// JsonUriHandler.cxx -- json interface to the property tree
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


#include "JsonUriHandler.hxx"
#include <Main/fg_props.hxx>
#include <simgear/props/props.hxx>
#include <3rdparty/cjson/cJSON.h>

using std::string;

namespace flightgear {
namespace http {

static const char * getPropertyTypeString( simgear::props::Type type )
{
  switch( type ) {
    case simgear::props::NONE: return "-";
    case simgear::props::ALIAS: return "alias";
    case simgear::props::BOOL: return "bool";
    case simgear::props::INT: return "int";
    case simgear::props::LONG: return "long";
    case simgear::props::FLOAT: return "float";
    case simgear::props::DOUBLE: return "double";
    case simgear::props::STRING: return "string";
    case simgear::props::UNSPECIFIED: return "unspecified";
    case simgear::props::EXTENDED: return "extended";
    case simgear::props::VEC3D: return "vec3d";
    case simgear::props::VEC4D: return "vec4d";
    default: return "?";
  }
}

static cJSON * PropToJson( SGPropertyNode_ptr n, int depth )
{
  cJSON * json = cJSON_CreateObject();
  cJSON_AddItemToObject(json, "path", cJSON_CreateString(n->getPath(true).c_str()));
  cJSON_AddItemToObject(json, "name", cJSON_CreateString(n->getName()));
  cJSON_AddItemToObject(json, "value", cJSON_CreateString(n->getStringValue()));
  cJSON_AddItemToObject(json, "type", cJSON_CreateString(getPropertyTypeString(n->getType())));
  cJSON_AddItemToObject(json, "index", cJSON_CreateNumber(n->getIndex()));

  if( depth > 0 && n->nChildren() > 0 ) {
    cJSON * jsonArray = cJSON_CreateArray();
    for( int i = 0; i < n->nChildren(); i++ )
      cJSON_AddItemToArray( jsonArray, PropToJson( n->getChild(i), depth-1 ) );
    cJSON_AddItemToObject( json, "children", jsonArray );
  }
  return json;
}

bool JsonUriHandler::handleGetRequest( const HTTPRequest & request, HTTPResponse & response )
{
  response.Header["Content-Type"] = "application/json; charset=UTF-8";

  string propertyPath = request.Uri;

  // strip the uri prefix of our handler
  propertyPath = propertyPath.substr( getUri().size() );

  // strip the querystring
  size_t pos = propertyPath.find( '?' );
  if( pos != string::npos ) {
    propertyPath = propertyPath.substr( 0, pos-1 );
  }

  // skip trailing '/' - not very efficient but shouldn't happen too often
  while( false == propertyPath.empty() && propertyPath[ propertyPath.length()-1 ] == '/' )
    propertyPath = propertyPath.substr(0,propertyPath.length()-1);

  // max recursion depth
  int  depth = atoi(request.RequestVariables.get("d").c_str());
  if( depth < 1 ) depth = 1; // at least one level 

  // pretty print (y) or compact print (default)
  bool indent = request.RequestVariables.get("i") == "y";

  SGPropertyNode_ptr node = fgGetNode( string("/") + propertyPath );
  if( false == node.valid() ) {
    response.StatusCode = 400;
    response.Content = "Node not found: " + propertyPath;
    SG_LOG(SG_NETWORK,SG_ALERT, response.Content );
    return true;

  } 

  cJSON * json = PropToJson( node, depth );

  char * jsonString = indent ? cJSON_Print( json ) : cJSON_PrintUnformatted( json );
  response.Content = jsonString;
  free( jsonString );
  cJSON_Delete( json );


  return true;

}

} // namespace http
} // namespace flightgear

