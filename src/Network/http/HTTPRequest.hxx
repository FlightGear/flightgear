// HTTPRequest.hxx -- Wraps a http Request
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

#ifndef FG_HTTPREQUEST_HXX
#define FG_HTTPREQUEST_HXX

#include <string>
#include <map>

namespace flightgear {
namespace http {

class HTTPRequest 
{
public:
  HTTPRequest() {}
  virtual ~HTTPRequest() {}

  std::string Method;
  std::string Uri;
  std::string HttpVersion;
  std::string QueryString;

  std::string remoteAddress;
  int         remotePort;
  std::string localAddress;
  int         localPort;

  std::string Content;

  class StringMap : public std::map<std::string,std::string> {
    public:
      std::string get( const std::string & key ) const {
        const_iterator it = find( key );
        return it == end() ? "" : it->second;
      }
  };

  StringMap RequestVariables;

  StringMap HeaderVariables;

};

}
} // namespace flightgear

#endif // FG_HTTPREQUEST_HXX
