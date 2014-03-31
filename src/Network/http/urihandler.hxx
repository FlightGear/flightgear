// urihandler.hxx -- a base class for URI handler
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

#ifndef __FG_URI_HANDLER_HXX
#define __FG_URI_HANDLER_HXX

#include "HTTPRequest.hxx"
#include "HTTPResponse.hxx"
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <string>
#include <map>

namespace flightgear {
namespace http {

class ConnectionData : public SGReferenced {
public:
  // make this polymorphic
  virtual ~ConnectionData() {}
};

class Connection {
public:
  void put( const std::string & key, SGSharedPtr<ConnectionData> value ) {
    connectionData[key] = value;
  }
  SGSharedPtr<ConnectionData> get(const std::string & key ) {
    return connectionData[key];
  }

  void remove( const std::string & key ) {
    connectionData.erase(key);
  }

  virtual void write(const char * data, size_t len) = 0;

private:
  std::map<std::string,SGSharedPtr<ConnectionData> > connectionData;
};

/**
 * A Base class for URI Handlers.
 * Provides methods for handling a request and handling subsequent polls.
 * All methods are implemented as noops and should be overridden by deriving classes
 */
class URIHandler : public SGReferenced {
public:
  URIHandler( const char * uri ) : _uri(uri) {}
  virtual ~URIHandler()  {}

  /**
   * This method gets called from the httpd if a request has been detected on the connection
   * @param request The HTTP Request filled in by the httpd
   * @param response the HTTP Response to be filled in by the hander
   * @param connection Connection specific information, can be used to store persistent state
   * @return true if the request has been completely answered, false to get poll()ed
   */
  virtual bool handleRequest( const HTTPRequest & request, HTTPResponse & response, Connection * connection = NULL ) {
    if( request.Method == "GET" ) return handleGetRequest( request, response, connection );
    if( request.Method == "PUT" ) return handlePutRequest( request, response, connection );
    return true;
  }

  /**
   * Convenience method for GET Requests, gets called by handleRequest if not overridden
   * @param request @see handleRequest()
   * @param response @see handleRequest()
   * @param connection @see handleRequest()
   * @return @see handleRequest()
   *
   */
  virtual bool handleGetRequest( const HTTPRequest & request, HTTPResponse & response, Connection * connection ) {
    return true;
  }

  /**
   * Convenience method for PUT Requests, gets called by handleRequest if not overridden
   * @param request @see handleRequest()
   * @param response @see handleRequest()
   * @param connection @see handleRequest()
   * @return @see handleRequest()
   *
   */
  virtual bool handlePutRequest( const HTTPRequest & request, HTTPResponse & response, Connection * connection ) {
    return true;
  }

  /**
   * This method gets called from the httpd if the preceding handleRequest() or poll() method returned false.
   * @param connection @see handleRequest()
   * @return @see handleRequest()
   */
  virtual bool poll( Connection * connection ) { return false; }

  /**
   * Getter for the URI this handler serves
   *
   * @return the URI this handler serves
   */
  const std::string & getUri() const { return _uri; }

private:
  std::string _uri;
};

} // namespace http
} // namespace flightgear

#endif //#define __FG_URI_HANDLER_HXX
