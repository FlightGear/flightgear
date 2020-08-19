// RunUriHandler.cxx -- Run a flightgear command
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


#include "RunUriHandler.hxx"
#include "jsonprops.hxx"
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>
#include <Main/globals.hxx>
#include <cJSON.h>


using std::string;

namespace flightgear {
namespace http {


bool RunUriHandler::handleRequest( const HTTPRequest & request, HTTPResponse & response, Connection * connection )
{
  response.Header["Content-Type"] = "text/plain";
  string command = request.RequestVariables.get("value");
  if( command.empty() ) {
    response.StatusCode = 400;
    response.Content = "command not specified";
    return true;
  }

  SGPropertyNode_ptr args = new SGPropertyNode();
  cJSON * json = cJSON_Parse( request.Content.c_str() );
  JSON::toProp( json, args );

  SG_LOG( SG_NETWORK, SG_INFO, "RunUriHandler("<< request.Content << "): command='" << command << "', arg='" << JSON::toJsonString(false,args,5) << "'");

  cJSON_Delete( json );
  if ( globals->get_commands()->execute(command.c_str(), args, nullptr) ) {
    response.Content = "ok.";
    return true;
  } 

  response.Content = "command '" + command + "' failed.";
  response.StatusCode = 501; // Not implemented probably suits best
  SG_LOG( SG_NETWORK, SG_WARN, response.Content );
  return true;
}

} // namespace http
} // namespace flightgear

