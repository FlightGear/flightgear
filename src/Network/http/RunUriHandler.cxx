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
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>
#include <Main/globals.hxx>
#include <3rdparty/cjson/cJSON.h>


using std::string;

namespace flightgear {
namespace http {

static void JsonToProp( cJSON * json, SGPropertyNode_ptr base )
{
  if( NULL == json ) return;

  cJSON * cj = cJSON_GetObjectItem( json, "name" );
  if( NULL == cj ) return; // a node with no name?
  char * name = cj->valuestring;
  if( NULL == name ) return; // still no name?

  //TODO: check valid name

  int index = 0;
  cj = cJSON_GetObjectItem( json, "index" );
  if( NULL != cj ) index = cj->valueint;
  if( index < 0 ) return;

  SGPropertyNode_ptr n = base->getNode( name, index, true );
  cJSON * children = cJSON_GetObjectItem( json, "children" );
  if( NULL != children ) {
    for( int i = 0; i < cJSON_GetArraySize( children ); i++ ) {
      JsonToProp( cJSON_GetArrayItem( children, i ), n );
    }
  } else {
    //TODO: set correct type
/*
    char * type = "";
    cj = cJSON_GetObjectItem( json, "type" );
    if( NULL != cj ) type = cj->valuestring;
*/
    char * value = NULL;
    cj = cJSON_GetObjectItem( json, "value" );
    if( NULL != cj ) value = cj->valuestring;

    if( NULL != value )
      n->setUnspecifiedValue( value );
  }
}

bool RunUriHandler::handleRequest( const HTTPRequest & request, HTTPResponse & response )
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
  JsonToProp( json, args );

  cJSON_Delete( json );
  if ( globals->get_commands()->execute(command.c_str(), args) ) {
    response.Content = "ok.";
    return true;
  } 

  response.Content = "command '" + command + "' failed.";
  SG_LOG( SG_NETWORK, SG_ALERT, response.Content );
  return true;
}

} // namespace http
} // namespace flightgear

