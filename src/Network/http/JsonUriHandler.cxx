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
    SG_LOG(SG_NETWORK,SG_WARN, "Node not found: '" << response.Content << "'");
    return true;

  } 

  response.Content = JSON::toJsonString( indent, node, depth );

  return true;

}

} // namespace http
} // namespace flightgear

