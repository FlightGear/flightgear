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
#include "jsonprops.hxx"
#include <Main/fg_props.hxx>
#include <simgear/props/props.hxx>

using std::string;

namespace flightgear {
namespace http {

bool JsonUriHandler::handleRequest( const HTTPRequest & request, HTTPResponse & response, Connection * connection )
{
  response.Header["Content-Type"] = "application/json; charset=UTF-8";
  response.Header["Access-Control-Allow-Origin"] = "*";
  response.Header["Access-Control-Allow-Methods"] = "OPTIONS, GET, POST";
  response.Header["Access-Control-Allow-Headers"] = "Origin, Accept, Content-Type, X-Requested-With, X-CSRF-Token";

  if( request.Method == "OPTIONS" ){
      return true; // OPTIONS only needs the headers
  }

  if( request.Method == "GET" ){
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
    bool timestamp = request.RequestVariables.get("t") == "y";

    SGPropertyNode_ptr node = fgGetNode( string("/") + propertyPath );
    if( false == node.valid() ) {
      response.StatusCode = 404;
      response.Content = "{}";
      SG_LOG(SG_NETWORK,SG_WARN, "Node not found: '" << propertyPath << "'");
      return true;

    } 

    response.Content = JSON::toJsonString( indent, node, depth, timestamp ? fgGetDouble("/sim/time/elapsed-sec") : -1.0 );

    return true;
  }

  if( request.Method == "POST" ) {
    SG_LOG(SG_NETWORK,SG_INFO, "Setting properties from JSON: " << request.Content );
    string propertyPath = request.Uri;
    propertyPath = propertyPath.substr( getUri().size() );

    // strip the querystring
    size_t pos = propertyPath.find( '?' );
    if( pos != string::npos ) {
      propertyPath = propertyPath.substr( 0, pos-1 );
    }

    // skip trailing '/' - not very efficient but shouldn't happen too often
    while( false == propertyPath.empty() && propertyPath[ propertyPath.length()-1 ] == '/' )
      propertyPath = propertyPath.substr(0,propertyPath.length()-1);

    SGPropertyNode_ptr node = fgGetNode( string("/") + propertyPath );
    if( false == node.valid() ) {
      response.StatusCode = 404;
      response.Content = "{}";
      SG_LOG(SG_NETWORK,SG_WARN, "Node not found: '" << propertyPath << "'");
      return true;
    } 

    cJSON * json = cJSON_Parse( request.Content.c_str() );
    if( NULL != json ) {
      JSON::toProp( json, node );
      cJSON_Delete(json);
    }

    response.Content = "{}";
    return true;
  }

  response.Header["Allow"] = "OPTIONS, GET";
  response.StatusCode = 405;
  response.Content = "{}";
  return true; 

}

} // namespace http
} // namespace flightgear

