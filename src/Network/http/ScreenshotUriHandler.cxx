// ScreenshotUriHandler.cxx -- Provide screenshots via http
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


#include "ScreenshotUriHandler.hxx"

using std::string;

namespace flightgear {
namespace http {

bool ScreenshotUriHandler::handleRequest( const HTTPRequest & request, HTTPResponse & response )
{
  response.Header["Content-Type"] = "image/jpeg";
//  string response = "Screenshot - not yet implemented :-(";
//  mg_send_data( connection, response.c_str(), response.length() );
  return true;
}

} // namespace http
} // namespace flightgear

