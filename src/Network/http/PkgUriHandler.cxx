// PkgUriHandler.cxx -- service for the package system
//
// Written by Torsten Dreyer, started February 2015.
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


#include "PkgUriHandler.hxx"
#include <cJSON.h>

#include <simgear/package/Root.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Delegate.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Package.hxx>

#include <Main/fg_props.hxx>

using std::string;

namespace flightgear {
namespace http {

/*
url: /pkg/command/args

Examples:
/pkg/path

Input:
{
  command: "command",
  args: {
  }
}

Output:
{
}
*/

static cJSON * StringListToJson( const string_list & l )
{
  cJSON * jsonArray = cJSON_CreateArray();
  for( string_list::const_iterator it = l.begin(); it != l.end(); ++it )
      cJSON_AddItemToArray(jsonArray, cJSON_CreateString((*it).c_str()) );
  return jsonArray;
}

static cJSON * PackageToJson( simgear::pkg::Package * p )
{
  cJSON * json = cJSON_CreateObject();
  if( p ) {
    cJSON_AddItemToObject(json, "id", cJSON_CreateString( p->id().c_str() ));
    cJSON_AddItemToObject(json, "name", cJSON_CreateString( p->name().c_str() ));
    cJSON_AddItemToObject(json, "description", cJSON_CreateString( p->description().c_str() ));
    cJSON_AddItemToObject(json, "installed", cJSON_CreateBool( p->isInstalled() ));
    cJSON_AddItemToObject(json, "thumbnails", StringListToJson( p->thumbnailUrls() ));
    cJSON_AddItemToObject(json, "variants", StringListToJson( p->variants() ));
    cJSON_AddItemToObject(json, "revision", cJSON_CreateNumber( p->revision() ));
    cJSON_AddItemToObject(json, "fileSize", cJSON_CreateNumber( p->fileSizeBytes() ));
    cJSON_AddItemToObject(json, "author", cJSON_CreateString( p->getLocalisedProp("author").c_str() ));
    cJSON_AddItemToObject(json, "ratingFdm", cJSON_CreateString( p->getLocalisedProp("rating/FDM").c_str() ));
    cJSON_AddItemToObject(json, "ratingCockpit", cJSON_CreateString( p->getLocalisedProp("rating/cockpit").c_str() ));
    cJSON_AddItemToObject(json, "ratingModel", cJSON_CreateString( p->getLocalisedProp("rating/model").c_str() ));
    cJSON_AddItemToObject(json, "ratingSystems", cJSON_CreateString( p->getLocalisedProp("rating/systems").c_str() ));
  }
  return json;
}

static cJSON * PackageListToJson( const simgear::pkg::PackageList & l )
{
  cJSON * jsonArray = cJSON_CreateArray();
  for( simgear::pkg::PackageList::const_iterator it = l.begin(); it != l.end(); ++it ) {
    cJSON_AddItemToArray(jsonArray, PackageToJson(*it) );
  }
  return jsonArray;
}

static cJSON * CatalogToJson( simgear::pkg::Catalog * c )
{
  cJSON * json = cJSON_CreateObject();
  if( c ) {
    cJSON_AddItemToObject(json, "id", cJSON_CreateString( c->id().c_str() ));
    std::string s = c->installRoot().utf8Str();
    cJSON_AddItemToObject(json, "installRoot", cJSON_CreateString( s.c_str() ));
    cJSON_AddItemToObject(json, "url", cJSON_CreateString( c->url().c_str() ));
    cJSON_AddItemToObject(json, "description", cJSON_CreateString( c->description().c_str() ));
    cJSON_AddItemToObject(json, "packages", PackageListToJson(c->packages()) );
    cJSON_AddItemToObject(json, "needingUpdate", PackageListToJson(c->packagesNeedingUpdate()) );
    cJSON_AddItemToObject(json, "installed", PackageListToJson(c->installedPackages()) );
  }
  return json;
}


static string PackageRootCommand( simgear::pkg::Root* packageRoot, const string & command, const string & args )
{
  cJSON * json = cJSON_CreateObject();

  if( command == "path" ) {
    std::string p = packageRoot->path().utf8Str();
    cJSON_AddItemToObject(json, "path", cJSON_CreateString( p.c_str() ));

  } else if( command == "version" ) {

    cJSON_AddItemToObject(json, "version", cJSON_CreateString( packageRoot->applicationVersion().c_str() ));

  } else if( command == "refresh" ) {
    packageRoot->refresh(true);
    cJSON_AddItemToObject(json, "refresh", cJSON_CreateString( "OK" ));

  } else if( command == "catalogs" ) {

    cJSON * jsonArray = cJSON_CreateArray();
    simgear::pkg::CatalogList catalogList = packageRoot->catalogs();
    for( simgear::pkg::CatalogList::iterator it = catalogList.begin(); it != catalogList.end(); ++it ) {
      cJSON_AddItemToArray(jsonArray, CatalogToJson(*it) );
    }
    cJSON_AddItemToObject(json, "catalogs", jsonArray );

  } else if( command == "packageById" ) {

    simgear::pkg::PackageRef p = packageRoot->getPackageById(args);
    cJSON_AddItemToObject(json, "package", PackageToJson( p ));

  } else if( command == "catalogById" ) {

    simgear::pkg::CatalogRef p = packageRoot->getCatalogById(args);
    cJSON_AddItemToObject(json, "catalog", CatalogToJson( p ));

  } else if( command == "search" ) {

    SGPropertyNode_ptr query(new SGPropertyNode);
    simgear::pkg::PackageList packageList = packageRoot->packagesMatching(query);
    cJSON_AddItemToObject(json, "packages", PackageListToJson(packageList) );

  } else if( command == "install" ) {

	  simgear::pkg::PackageRef package = packageRoot->getPackageById(args);
	  if( NULL == package ) {
		  SG_LOG(SG_NETWORK,SG_WARN,"Can't install package '" << args << "', package not found" );
		    cJSON_Delete( json );
		    return string("");
	  }
	  package->existingInstall();

  } else {
    SG_LOG( SG_NETWORK,SG_WARN, "Unhandled pkg command : '" << command << "'" );
    cJSON_Delete( json );
    return string("");
  }

  char * jsonString = cJSON_PrintUnformatted( json );
  string reply(jsonString);
  free( jsonString );
  cJSON_Delete( json );
  return reply;
}

static  string findCommand( const string & uri, string & outArgs )
{
  size_t n = uri.find_first_of('/');
  if( n == string::npos ) outArgs = string("");
  else outArgs = uri.substr( n+1 );
  return uri.substr( 0, n );
}

bool PkgUriHandler::handleRequest( const HTTPRequest & request, HTTPResponse & response, Connection * connection )
{
  response.Header["Content-Type"] = "application/json; charset=UTF-8";
  response.Header["Access-Control-Allow-Origin"] = "*";
  response.Header["Access-Control-Allow-Methods"] = "OPTIONS, GET, POST";
  response.Header["Access-Control-Allow-Headers"] = "Origin, Accept, Content-Type, X-Requested-With, X-CSRF-Token";

  if( request.Method == "OPTIONS" ){
      return true; // OPTIONS only needs the headers
  }

  simgear::pkg::Root* packageRoot = globals->packageRoot();
  if( NULL == packageRoot ) {
    SG_LOG( SG_NETWORK,SG_WARN, "NO PackageRoot" );
    response.StatusCode = 500;
    response.Content = "{}";
    return true; 
  }

  string argString;
  string command = findCommand( string(request.Uri).substr(getUri().size()), argString );
  

  SG_LOG(SG_NETWORK,SG_INFO, "Request is for command '"  << command << "' with arg='" << argString << "'" );

  if( request.Method == "GET" ){
  } else if( request.Method == "POST" ) {
  } else {
    SG_LOG(SG_NETWORK,SG_INFO, "PkgUriHandler: invalid request method '" << request.Method << "'" );
    response.Header["Allow"] = "OPTIONS, GET, POST";
    response.StatusCode = 405;
    response.Content = "{}";
    return true; 
  }

  response.Content = PackageRootCommand( packageRoot, command, argString );
  if( response.Content.empty() ) {
    response.StatusCode = 404;
    response.Content = "{}";
  }
  return true; 
}

} // namespace http
} // namespace flightgear

