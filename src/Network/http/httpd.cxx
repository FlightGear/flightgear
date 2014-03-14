// httpd.cxx -- a http daemon subsystem based on Mongoose http
//
// Written by Torsten Dreyer, started April 2014.
//
// Copyright (C) 2014  Torsten Dreyer
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "httpd.hxx"
#include "HTTPRequest.hxx"
#include "PropertyChangeWebsocket.hxx"
#include "ScreenshotUriHandler.hxx"
#include "PropertyUriHandler.hxx"
#include "JsonUriHandler.hxx"
#include "RunUriHandler.hxx"
#include "PropertyChangeObserver.hxx"
#include <Main/fg_props.hxx>
#include <Include/version.h>
#include <3rdparty/mongoose/mongoose.h>
#include <3rdparty/cjson/cJSON.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace flightgear {
namespace http {

const char * PROPERTY_ROOT = "/sim/http";

class MongooseHttpd: public FGHttpd {
public:
  MongooseHttpd(SGPropertyNode_ptr);
  ~MongooseHttpd();
  void init();
  void bind();
  void unbind();
  void shutdown();
  void update(double dt);

private:
  int requestHandler(struct mg_connection *);
  int websocketHandler(struct mg_connection *);
  void closeWebsocket(struct mg_connection *);
  int iterateCallback(struct mg_connection *, mg_event event);

  static int staticRequestHandler(struct mg_connection *, mg_event event);
  static int staticIterateCallback(struct mg_connection *, mg_event event);

  struct mg_server *_server;
  SGPropertyNode_ptr _configNode;

  typedef int (MongooseHttpd::*handler_t)(struct mg_connection *);
  typedef vector<SGSharedPtr<URIHandler> > URIHandlerMap;
  URIHandlerMap _uriHandlers;

  PropertyChangeObserver _propertyChangeObserver;
};

MongooseHttpd::MongooseHttpd(SGPropertyNode_ptr configNode)
    : _server(NULL), _configNode(configNode)
{
}

MongooseHttpd::~MongooseHttpd()
{
  mg_destroy_server(&_server);
}

void MongooseHttpd::init()
{
  SGPropertyNode_ptr n = _configNode->getNode("uri-handler");
  if (n.valid()) {
    const char * uri;

    if ((uri = n->getStringValue("screenshot"))[0] != 0) {
      SG_LOG(SG_NETWORK, SG_INFO, "httpd: adding screenshot uri handler at " << uri);
      _uriHandlers.push_back(new flightgear::http::ScreenshotUriHandler(uri));
    }

    if ((uri = n->getStringValue("property"))[0] != 0) {
      SG_LOG(SG_NETWORK, SG_INFO, "httpd: adding screenshot property handler at " << uri);
      _uriHandlers.push_back(new flightgear::http::PropertyUriHandler(uri));
    }

    if ((uri = n->getStringValue("json"))[0] != 0) {
      SG_LOG(SG_NETWORK, SG_INFO, "httpd: adding json property handler at " << uri);
      _uriHandlers.push_back(new flightgear::http::JsonUriHandler(uri));
    }

    if ((uri = n->getStringValue("run"))[0] != 0) {
      SG_LOG(SG_NETWORK, SG_INFO, "httpd: adding run handler at " << uri);
      _uriHandlers.push_back(new flightgear::http::RunUriHandler(uri));
    }
  }

  _server = mg_create_server(this, MongooseHttpd::staticRequestHandler);

  n = _configNode->getNode("options");
  if (n.valid()) {

    const string fgRoot = fgGetString("/sim/fg-root");
    string docRoot = n->getStringValue("document-root", fgRoot.c_str() );
    if (docRoot[0] != '/') docRoot.insert(0, "/").insert(0, fgRoot );

    mg_set_option(_server, "document_root", docRoot.c_str());

    mg_set_option(_server, "listening_port", n->getStringValue("listening-port", "8080"));
    {
      // build url rewrites relative to fg-root
      string rewrites = n->getStringValue( "url-rewrites", "" );
      string_list rwl = simgear::strutils::split( rewrites, "," );
      rewrites.clear();
      for( string_list::iterator it = rwl.begin(); it != rwl.end(); ++it ) {
        string_list rw_entries = simgear::strutils::split( *it, "=" );
        if( rw_entries.size() != 2 ) {
          SG_LOG( SG_NETWORK,SG_WARN, "invalid entry '" << *it << "' in url-rewrites ignored." );
          continue;
        }
        string & lhs = rw_entries[0];
        string & rhs = rw_entries[1];
        if( false == rewrites.empty()) rewrites.append(1,',');
        rewrites.append( lhs ).append(1,'=');
        if( rhs[0] == '/' ) {
          rewrites.append( rhs );
        } else {
          rewrites.append( fgRoot ).append( 1, '/' ).append( rhs );
        }
      }
      SG_LOG(SG_NETWORK,SG_ALERT,"url-rewrites='" << rewrites << "'" );
      if( false == rewrites.empty() )
        mg_set_option(_server, "url_rewrites", rewrites.c_str() );
    }
    mg_set_option(_server, "enable_directory_listing", n->getStringValue("enable-directory-listing", "yes"));
    mg_set_option(_server, "idle_timeout_ms", n->getStringValue("idle-timeout-ms", "30000"));
    mg_set_option(_server, "index_files", n->getStringValue("index-files", "index.html"));

  }

}

void MongooseHttpd::bind()
{
}

void MongooseHttpd::unbind()
{
  mg_destroy_server(&_server);
  _uriHandlers.clear();
}

void MongooseHttpd::shutdown()
{
}

void MongooseHttpd::update(double dt)
{
  _propertyChangeObserver.check();
  mg_poll_server(_server, 0);
  mg_iterate_over_connections(_server, &MongooseHttpd::staticIterateCallback);
}

class MongooseHTTPRequest: public HTTPRequest {
private:
  inline string NotNull(const char * cp, size_t n = string::npos)
  {
    if ( NULL == cp || 0 == n) return string("");
    if (string::npos == n) return string(cp);
    return string(cp, n);
  }

public:
  MongooseHTTPRequest(struct mg_connection * connection)
  {
    Method = NotNull(connection->request_method);
    Uri = urlDecode(NotNull(connection->uri));
    HttpVersion = NotNull(connection->http_version);
    QueryString = urlDecode(NotNull(connection->query_string));

    remoteAddress = NotNull(connection->remote_ip);
    remotePort = connection->remote_port;
    localAddress = NotNull(connection->local_ip);
    localPort = connection->local_port;

    using namespace simgear::strutils;
    string_list pairs = split(string(QueryString), "&");
    for (string_list::iterator it = pairs.begin(); it != pairs.end(); ++it) {
      string_list nvp = split(*it, "=");
      if (nvp.size() != 2) continue;
      RequestVariables.insert(make_pair(nvp[0], nvp[1]));
    }

    for (int i = 0; i < connection->num_headers; i++)
      HeaderVariables[connection->http_headers[i].name] = connection->http_headers[i].value;

    Content = NotNull(connection->content, connection->content_len);

  }

  string urlDecode(const string & s)
  {
    string r = "";
    int max = s.length();
    int a, b;
    for (int i = 0; i < max; i++) {
      if (s[i] == '+') {
        r += ' ';
      } else if (s[i] == '%' && i + 2 < max && isxdigit(s[i + 1]) && isxdigit(s[i + 2])) {
        i++;
        a = isdigit(s[i]) ? s[i] - '0' : toupper(s[i]) - 'A' + 10;
        i++;
        b = isdigit(s[i]) ? s[i] - '0' : toupper(s[i]) - 'A' + 10;
        r += (char) (a * 16 + b);
      } else {
        r += s[i];
      }
    }
    return r;
  }

};

void MongooseHttpd::closeWebsocket(struct mg_connection * connection)
{
  Websocket * websocket = static_cast<Websocket*>(connection->connection_param);
  if ( NULL != websocket) websocket->close();
  delete websocket;
}

class MongooseWebsocketWriter: public WebsocketWriter {
public:
  MongooseWebsocketWriter(struct mg_connection * connection)
      : _connection(connection)
  {
  }

  virtual int writeToWebsocket(int opcode, const char * data, size_t len)
  {
    return mg_websocket_write( _connection, opcode, data, len );
  }
private:
  struct mg_connection * _connection;
};

int MongooseHttpd::websocketHandler(struct mg_connection * connection)
{
  MongooseHTTPRequest request(connection);
  MongooseWebsocketWriter writer(connection);

  Websocket * websocket = static_cast<Websocket*>(connection->connection_param);
  if ( NULL == websocket) {
    if (request.Uri.find("/PropertyListener") == 0) {
      SG_LOG(SG_ALL, SG_ALERT, "new PropertyChangeWebsocket for: " << request.Uri);
      websocket = new PropertyChangeWebsocket(&_propertyChangeObserver);
      connection->connection_param = websocket;
    } else {
      SG_LOG(SG_ALL, SG_ALERT, "httpd: unhandled websocket uri: " << request.Uri);
      return MG_FALSE;
    }
  }

  websocket->handleRequest(request, writer);
  return MG_TRUE;

}

int MongooseHttpd::requestHandler(struct mg_connection * connection)
{
  MongooseHTTPRequest request(connection);
  HTTPResponse response;
  response.Header["Server"] = "FlightGear/" FLIGHTGEAR_VERSION " Mongoose/" MONGOOSE_VERSION;
  response.Header["Connection"] = "close";
  response.Header["Cache-Control"] = "no-cache";
  {
    char buf[64];
    time_t now = time(NULL);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    response.Header["Date"] = buf;
  }

  bool processed = false;
  for (URIHandlerMap::iterator it = _uriHandlers.begin(); it != _uriHandlers.end(); ++it) {
    URIHandler * handler = *it;
    if (request.Uri.find(handler->getUri()) == 0) {
      processed = handler->handleRequest(request, response);
      break;
    }
  }

  if (processed) {
    for (HTTPResponse::Header_t::const_iterator it = response.Header.begin(); it != response.Header.end(); ++it) {
      const string name = it->first;
      const string value = it->second;
      if (name.empty() || value.empty()) continue;
      mg_send_header(connection, name.c_str(), value.c_str());
    }
    mg_send_status(connection, response.StatusCode);
    mg_send_data(connection, response.Content.c_str(), response.Content.length());
  }

  return processed ? MG_TRUE : MG_FALSE;
}

int MongooseHttpd::iterateCallback(struct mg_connection * connection, mg_event event)
{
  if (connection->is_websocket && event == MG_POLL) {
    Websocket * websocket = static_cast<Websocket*>(connection->connection_param);
    if ( NULL == websocket) return MG_TRUE;
    MongooseWebsocketWriter writer( connection );
    websocket->update( writer );
  }
  return MG_TRUE; // return value is ignored
}

int MongooseHttpd::staticIterateCallback(struct mg_connection * connection, mg_event event)
{
  return static_cast<MongooseHttpd*>(connection->server_param)->iterateCallback(connection, event);
}

int MongooseHttpd::staticRequestHandler(struct mg_connection * connection, mg_event event)
{
  switch (event) {
    case MG_REQUEST:
      return
          connection->is_websocket ?
              static_cast<MongooseHttpd*>(connection->server_param)->websocketHandler(connection) :
              static_cast<MongooseHttpd*>(connection->server_param)->requestHandler(connection);

    case MG_CLOSE:
      if (connection->is_websocket) static_cast<MongooseHttpd*>(connection->server_param)->closeWebsocket(connection);
      return MG_TRUE;

    case MG_AUTH:
      return MG_TRUE; // allow anybody (for now)

    default:
      return MG_FALSE;

  }
}

FGHttpd * FGHttpd::createInstance(SGPropertyNode_ptr configNode)
{
// only create a server if a port has been configured
  if (false == configNode.valid()) return NULL;
  string port = configNode->getStringValue("options/listening-port", "");
  if (port.empty()) return NULL;
  return new MongooseHttpd(configNode);
}

} // namespace http
} // namespace flightgear
