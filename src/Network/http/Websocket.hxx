// Websocket.cxx -- a base class for websockets
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

#ifndef WEBSOCKET_HXX_
#define WEBSOCKET_HXX_

#include "HTTPRequest.hxx"
#include <string>

namespace flightgear {
namespace http {

class WebsocketWriter {
public:
  virtual ~WebsocketWriter()
  {
  }

  virtual int writeToWebsocket(int opcode, const char * data, size_t len) = 0;

  // ref: http://tools.ietf.org/html/rfc6455#section-5.2
  int writeContinuation(const char * data, size_t len); // { return writeToWebsocket( 0, data, len ); }
  int writeText(const char * data, size_t len)
  {
    return writeToWebsocket(1, data, len);
  }
  inline int writeText(const std::string & text)
  {
    return writeText(text.c_str(), text.length());
  }
  inline int writeBinary(const char * data, size_t len)
  {
    return writeToWebsocket(2, data, len);
  }
  inline int writeConnectionClose(const char * data, size_t len)
  {
    return writeToWebsocket(8, data, len);
  }
  inline int writePing(const char * data, size_t len)
  {
    return writeToWebsocket(9, data, len);
  }
  inline int writePong(const char * data, size_t len)
  {
    return writeToWebsocket(0xa, data, len);
  }
};

class Websocket {
public:
  virtual ~Websocket()
  {
  }
  virtual void close() = 0;
  virtual void handleRequest(const HTTPRequest & request, WebsocketWriter & writer) = 0;
  virtual void poll(WebsocketWriter & writer) = 0;

};

} // namespace http
} // namespace flightgear

#endif /* WEBSOCKET_HXX_ */
