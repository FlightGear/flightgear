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
#include "NavdbUriHandler.hxx"
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

/**
 * A Helper class for URI Handlers
 *
 * This class stores a list of URI Handlers and provides a lookup
 * method for find the handler by it's URI prefix
 */
class URIHandlerMap: public vector<SGSharedPtr<URIHandler> > {
public:
  /**
   * Find a URI Handler for a given URI
   *
   * Look for the first handler with a uri matching the beginning
   * of the given uri parameter.
   *
   * @param uri The uri to find the handler for
   * @return a SGSharedPtr of the URIHandler or an invalid SGSharedPtr if not found
   */
  SGSharedPtr<URIHandler> findHandler(const std::string & uri)
  {
    for (iterator it = begin(); it != end(); ++it) {
      SGSharedPtr<URIHandler> handler = *it;
      // check if the request-uri starts with the registered uri-string
      if (0 == uri.find(handler->getUri())) return handler;
    }
    return SGSharedPtr<URIHandler>();
  }
};

/**
 * A Helper class to create a HTTPRequest from a mongoose connection struct
 */
class MongooseHTTPRequest: public HTTPRequest {
private:
  /**
   * Creates a std::string from a char pointer and an optionally given length
   * If the pointer is NULL or the length is zero, return an empty string
   * If no length is given, create a std::string from a c-string (up to the /0 terminator)
   * If length is given, use as many chars as given in length (can exceed the /0 terminator)
   *
   * @param cp Points to the source of the string
   * @param len The number of chars to copy to the new string (optional)
   * @return a std::string containing a copy of the source
   */
  static inline string NotNull(const char * cp, size_t len = string::npos)
  {
    if ( NULL == cp || 0 == len) return string("");
    if (string::npos == len) return string(cp);
    return string(cp, len);
  }

public:
  /**
   * Constructs a HTTPRequest from a mongoose connection struct
   * Copies all fields into STL compatible local elements, performs urlDecode etc.
   *
   * @param connection the mongoose connection struct with the source data
   */
  MongooseHTTPRequest(struct mg_connection * connection)
  {
    Method = NotNull(connection->request_method);
    Uri = urlDecode(NotNull(connection->uri));
    HttpVersion = NotNull(connection->http_version);
    QueryString = NotNull(connection->query_string);

    remoteAddress = NotNull(connection->remote_ip);
    remotePort = connection->remote_port;
    localAddress = NotNull(connection->local_ip);
    localPort = connection->local_port;

    using namespace simgear::strutils;
    string_list pairs = split(string(QueryString), "&");
    for (string_list::iterator it = pairs.begin(); it != pairs.end(); ++it) {
      string_list nvp = split(*it, "=");
      if (nvp.size() != 2) continue;
      RequestVariables.insert(make_pair(urlDecode(nvp[0]), urlDecode(nvp[1])));
    }

    for (int i = 0; i < connection->num_headers; i++)
      HeaderVariables[connection->http_headers[i].name] = connection->http_headers[i].value;

    Content = NotNull(connection->content, connection->content_len);

  }

  /**
   * Decodes a URL encoded string
   * replaces '+' by ' '
   * replaces %nn hexdigits
   *
   * @param s The source string do decode
   * @return The decoded String
   */
  static string urlDecode(const string & s)
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

/**
 * A FGHttpd implementation based on mongoose httpd
 *
 * Mongoose API is documented here: http://cesanta.com/docs/API.shtml
 */
class MongooseHttpd: public FGHttpd {
public:

  /**
   * Construct a MongooseHttpd object from options in a PropertyNode
   */
  MongooseHttpd(SGPropertyNode_ptr);

  /**
   * Cleanup et.al.
   */
  ~MongooseHttpd();

  /**
   * override SGSubsystem::init()
   *
   * Reads the configuration PropertyNode, installs URIHandlers and configures mongoose
   */
  void init();

  /**
   * override SGSubsystem::bind()
   *
   * Currently a noop
   */
  void bind();

  /**
   * override SGSubsystem::unbind()
   * shutdown of mongoose, clear connections, unregister URIHandlers
   */
  void unbind();

  /**
   * overrride SGSubsystem::update()
   * poll connections, check for changed properties
   */
  void update(double dt);

  /**
   * Returns a URIHandler for the given uri
   *
   * @see URIHandlerMap::findHandler( const std::string & uri )
   */
  SGSharedPtr<URIHandler> findHandler(const std::string & uri)
  {
    return _uriHandler.findHandler(uri);
  }

  Websocket * newWebsocket(const string & uri);

private:
  int poll(struct mg_connection * connection);
  int auth(struct mg_connection * connection);
  int request(struct mg_connection * connection);
  void close(struct mg_connection * connection);

  static int staticRequestHandler(struct mg_connection *, mg_event event);

  struct mg_server *_server;
  SGPropertyNode_ptr _configNode;

  typedef int (MongooseHttpd::*handler_t)(struct mg_connection *);
  URIHandlerMap _uriHandler;

  PropertyChangeObserver _propertyChangeObserver;
};

class MongooseConnection: public Connection {
public:
  MongooseConnection(MongooseHttpd * httpd)
      : _httpd(httpd)
  {
  }
  virtual ~MongooseConnection();

  virtual void close(struct mg_connection * connection) = 0;
  virtual int poll(struct mg_connection * connection) = 0;
  virtual int request(struct mg_connection * connection) = 0;
  virtual void write(const char * data, size_t len)
  {
    if (_connection) mg_send_data(_connection, data, len);
  }

  static MongooseConnection * getConnection(MongooseHttpd * httpd, struct mg_connection * connection);

protected:
  void setConnection(struct mg_connection * connection)
  {
    _connection = connection;
  }
  MongooseHttpd * _httpd;
  struct mg_connection * _connection;

};

MongooseConnection::~MongooseConnection()
{
}

class RegularConnection: public MongooseConnection {
public:
  RegularConnection(MongooseHttpd * httpd)
      : MongooseConnection(httpd)
  {
  }
  virtual ~RegularConnection()
  {
  }

  virtual void close(struct mg_connection * connection);
  virtual int poll(struct mg_connection * connection);
  virtual int request(struct mg_connection * connection);

private:
  SGSharedPtr<URIHandler> _handler;
};

class WebsocketConnection: public MongooseConnection {
public:
  WebsocketConnection(MongooseHttpd * httpd)
      : MongooseConnection(httpd), _websocket(NULL)
  {
  }
  virtual ~WebsocketConnection()
  {
    delete _websocket;
  }
  virtual void close(struct mg_connection * connection);
  virtual int poll(struct mg_connection * connection);
  virtual int request(struct mg_connection * connection);

private:
  class MongooseWebsocketWriter: public WebsocketWriter {
  public:
    MongooseWebsocketWriter(struct mg_connection * connection)
        : _connection(connection)
    {
    }

    virtual int writeToWebsocket(int opcode, const char * data, size_t len)
    {
      return mg_websocket_write(_connection, opcode, data, len);
    }
  private:
    struct mg_connection * _connection;
  };
  Websocket * _websocket;
};

MongooseConnection * MongooseConnection::getConnection(MongooseHttpd * httpd, struct mg_connection * connection)
{
  if (connection->connection_param) return static_cast<MongooseConnection*>(connection->connection_param);
  MongooseConnection * c;
  if (connection->is_websocket) c = new WebsocketConnection(httpd);
  else c = new RegularConnection(httpd);

  connection->connection_param = c;
  return c;
}

int RegularConnection::request(struct mg_connection * connection)
{
  setConnection(connection);
  MongooseHTTPRequest request(connection);
  SG_LOG(SG_NETWORK, SG_INFO, "RegularConnection::request for " << request.Uri);

  // find a handler for the uri and remember it for possible polls on this connection
  _handler = _httpd->findHandler(request.Uri);
  if (false == _handler.valid()) {
    // uri not registered - pass false to indicate we have not processed the request
    return MG_FALSE;
  }

  // We handle this URI, prepare the response
  HTTPResponse response;
  response.Header["Server"] = "FlightGear/" FLIGHTGEAR_VERSION " Mongoose/" MONGOOSE_VERSION;
  response.Header["Connection"] = "keep-alive";
  response.Header["Cache-Control"] = "no-cache";
  {
    char buf[64];
    time_t now = time(NULL);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    response.Header["Date"] = buf;
  }

  // hand the request over to the handler, returns true if request is finished, 
  // false the handler wants to get polled again (calling handlePoll() next time)
  bool done = _handler->handleRequest(request, response, this);
  // fill in the response header
  mg_send_status(connection, response.StatusCode);
  for (HTTPResponse::Header_t::const_iterator it = response.Header.begin(); it != response.Header.end(); ++it) {
    const string name = it->first;
    const string value = it->second;
    if (name.empty() || value.empty()) continue;
    mg_send_header(connection, name.c_str(), value.c_str());
  }
  if (done || false == response.Content.empty()) {
    SG_LOG(SG_NETWORK, SG_INFO,
        "RegularConnection::request() responding " << response.Content.length() << " Bytes, done=" << done);
    mg_send_data(connection, response.Content.c_str(), response.Content.length());
  }
  return done ? MG_TRUE : MG_MORE;
}

int RegularConnection::poll(struct mg_connection * connection)
{
  setConnection(connection);
  if (false == _handler.valid()) return MG_FALSE;
  // only return MG_TRUE if we handle this request
  return _handler->poll(this) ? MG_TRUE : MG_MORE;
}

void RegularConnection::close(struct mg_connection * connection)
{
  setConnection(connection);
  // nothing to close
}

void WebsocketConnection::close(struct mg_connection * connection)
{
  setConnection(connection);
  if ( NULL != _websocket) _websocket->close();
  delete _websocket;
  _websocket = NULL;
}

int WebsocketConnection::poll(struct mg_connection * connection)
{
  setConnection(connection);
  // we get polled before the first request came in but we know 
  // nothing about how to handle that before we know the URI.
  // so simply ignore that poll
  if ( NULL != _websocket) {
    MongooseWebsocketWriter writer(connection);
    _websocket->poll(writer);
  }
  return MG_MORE;
}

int WebsocketConnection::request(struct mg_connection * connection)
{
  setConnection(connection);
  MongooseHTTPRequest request(connection);
  SG_LOG(SG_NETWORK, SG_INFO, "WebsocketConnection::request for " << request.Uri);

  if ( NULL == _websocket) _websocket = _httpd->newWebsocket(request.Uri);
  if ( NULL == _websocket) {
    SG_LOG(SG_NETWORK, SG_WARN, "httpd: unhandled websocket uri: " << request.Uri);
    return MG_TRUE; // close connection - good bye
  }

  MongooseWebsocketWriter writer(connection);
  _websocket->handleRequest(request, writer);
  return MG_MORE;
}

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
      _uriHandler.push_back(new flightgear::http::ScreenshotUriHandler(uri));
    }

    if ((uri = n->getStringValue("property"))[0] != 0) {
      SG_LOG(SG_NETWORK, SG_INFO, "httpd: adding property uri handler at " << uri);
      _uriHandler.push_back(new flightgear::http::PropertyUriHandler(uri));
    }

    if ((uri = n->getStringValue("json"))[0] != 0) {
      SG_LOG(SG_NETWORK, SG_INFO, "httpd: adding json uri handler at " << uri);
      _uriHandler.push_back(new flightgear::http::JsonUriHandler(uri));
    }

    if ((uri = n->getStringValue("run"))[0] != 0) {
      SG_LOG(SG_NETWORK, SG_INFO, "httpd: adding run uri handler at " << uri);
      _uriHandler.push_back(new flightgear::http::RunUriHandler(uri));
    }

    if ((uri = n->getStringValue("navdb"))[0] != 0) {
      SG_LOG(SG_NETWORK, SG_INFO, "httpd: adding navdb uri handler at " << uri);
      _uriHandler.push_back(new flightgear::http::NavdbUriHandler(uri));
    }
  }

  _server = mg_create_server(this, MongooseHttpd::staticRequestHandler);

  n = _configNode->getNode("options");
  if (n.valid()) {

    const string fgRoot = fgGetString("/sim/fg-root");
    string docRoot = n->getStringValue("document-root", fgRoot.c_str());
    if (docRoot[0] != '/') docRoot.insert(0, "/").insert(0, fgRoot);

    mg_set_option(_server, "document_root", docRoot.c_str());

    mg_set_option(_server, "listening_port", n->getStringValue("listening-port", "8080"));
    {
      // build url rewrites relative to fg-root
      string rewrites = n->getStringValue("url-rewrites", "");
      string_list rwl = simgear::strutils::split(rewrites, ",");
      rwl.push_back(string("/aircraft-dir/=") + fgGetString("/sim/aircraft-dir") + "/" );
      rewrites.clear();
      for (string_list::iterator it = rwl.begin(); it != rwl.end(); ++it) {
        string_list rw_entries = simgear::strutils::split(*it, "=");
        if (rw_entries.size() != 2) {
          SG_LOG(SG_NETWORK, SG_WARN, "invalid entry '" << *it << "' in url-rewrites ignored.");
          continue;
        }
        string & lhs = rw_entries[0];
        string & rhs = rw_entries[1];
        if (false == rewrites.empty()) rewrites.append(1, ',');
        rewrites.append(lhs).append(1, '=');
        SGPath targetPath(rhs);
        if (targetPath.isAbsolute() ) {
          rewrites.append(rhs);
        } else {
          // don't use targetPath here because SGPath strips trailing '/'
          rewrites.append(fgRoot).append(1, '/').append(rhs);
        }
      }
      if (false == rewrites.empty()) mg_set_option(_server, "url_rewrites", rewrites.c_str());
    }
    mg_set_option(_server, "enable_directory_listing", n->getStringValue("enable-directory-listing", "yes"));
    mg_set_option(_server, "idle_timeout_ms", n->getStringValue("idle-timeout-ms", "30000"));
    mg_set_option(_server, "index_files", n->getStringValue("index-files", "index.html"));
    mg_set_option(_server, "extra_mime_types", n->getStringValue("extra-mime-types", ""));
    mg_set_option(_server, "access_log_file", n->getStringValue("access-log-file", ""));

    if( sglog().would_log(SG_NETWORK,SG_INFO) ) {
      SG_LOG(SG_NETWORK,SG_INFO,"starting mongoose with these options: ");
      const char ** optionNames = mg_get_valid_option_names();
      for( int i = 0; optionNames[i] != NULL; i+= 2 ) {
        SG_LOG(SG_NETWORK,SG_INFO, "  > " << optionNames[i] << ": '" << mg_get_option(_server, optionNames[i]) << "'" );
      }
      SG_LOG(SG_NETWORK,SG_INFO,"end of mongoose options.");
    }

  }

  _configNode->setBoolValue("running",true);

}

void MongooseHttpd::bind()
{
}

void MongooseHttpd::unbind()
{
  _configNode->setBoolValue("running",false);
  mg_destroy_server(&_server);
  _uriHandler.clear();
  _propertyChangeObserver.clear();
}

void MongooseHttpd::update(double dt)
{
  _propertyChangeObserver.check();
  mg_poll_server(_server, 0);
  _propertyChangeObserver.uncheck();
}

int MongooseHttpd::poll(struct mg_connection * connection)
{
  if ( NULL == connection->connection_param) return MG_FALSE; // connection not yet set up - ignore poll

  return MongooseConnection::getConnection(this, connection)->poll(connection);
}

int MongooseHttpd::auth(struct mg_connection * connection)
{
  // auth preceeds request for websockets and regular connections,
  // and eventually the websocket has been already set up by mongoose
  // use this to choose the connection type
  MongooseConnection::getConnection(this, connection);
  //return MongooseConnection::getConnection(this,connection)->auth(connection);
  return MG_TRUE; // unrestricted access for now
}

int MongooseHttpd::request(struct mg_connection * connection)
{
  return MongooseConnection::getConnection(this, connection)->request(connection);
}

void MongooseHttpd::close(struct mg_connection * connection)
{
  MongooseConnection * c = MongooseConnection::getConnection(this, connection);
  c->close(connection);
  delete c;
}
Websocket * MongooseHttpd::newWebsocket(const string & uri)
{
  if (uri.find("/PropertyListener") == 0) {
    SG_LOG(SG_NETWORK, SG_INFO, "new PropertyChangeWebsocket for: " << uri);
    return new PropertyChangeWebsocket(&_propertyChangeObserver);
  }
  return NULL;
}

int MongooseHttpd::staticRequestHandler(struct mg_connection * connection, mg_event event)
{
  switch (event) {
    case MG_POLL:        // MG_TRUE: finished sending data, MG_MORE, call again
      return static_cast<MongooseHttpd*>(connection->server_param)->poll(connection);

    case MG_AUTH:        // If callback returns MG_FALSE, authentication fails
      return static_cast<MongooseHttpd*>(connection->server_param)->auth(connection);

    case MG_REQUEST:     // If callback returns MG_FALSE, Mongoose continues with req
      return static_cast<MongooseHttpd*>(connection->server_param)->request(connection);

    case MG_CLOSE:       // Connection is closed, callback return value is ignored
      static_cast<MongooseHttpd*>(connection->server_param)->close(connection);
      return MG_TRUE;

    case MG_HTTP_ERROR:  // If callback returns MG_FALSE, Mongoose continues with err 
      return MG_FALSE;   // we don't handle errors - let mongoose do the work

      // client services not used/implemented. Signal 'close connection' to be sure
    case MG_CONNECT:     // If callback returns MG_FALSE, connect fails
    case MG_REPLY:       // If callback returns MG_FALSE, Mongoose closes connection
      return MG_FALSE;

    default:
      return MG_FALSE; // keep compiler happy..
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
