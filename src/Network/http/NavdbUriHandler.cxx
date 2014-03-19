// NavdbUriHandler.cxx -- Access the nav database
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

#include "NavdbUriHandler.hxx"
#include <simgear/debug/logstream.hxx>
#include <Navaids/positioned.hxx>
#include <3rdparty/cjson/cJSON.h>
#include <boost/lexical_cast.hpp>

using std::string;

namespace flightgear {
namespace http {

//Ref: http://geojson.org/

static cJSON * createGeometryFor(FGPositionedRef positioned)
{
  // for now, we are lazy - everything is just a point.

  cJSON * geometry = cJSON_CreateObject();
  cJSON_AddItemToObject(geometry, "type", cJSON_CreateString("Point"));

  cJSON * coordinates = cJSON_CreateArray();
  cJSON_AddItemToObject(geometry, "coordinates", coordinates);

  cJSON_AddItemToArray(coordinates, cJSON_CreateNumber(positioned->longitude()));
  cJSON_AddItemToArray(coordinates, cJSON_CreateNumber(positioned->latitude()));

  return geometry;
}

static cJSON * createPropertiesFor(FGPositionedRef positioned)
{
  cJSON * properties = cJSON_CreateObject();

  cJSON_AddItemToObject(properties, "name", cJSON_CreateString(positioned->name().c_str()));
  cJSON_AddItemToObject(properties, "type", cJSON_CreateString(positioned->typeString()));
  cJSON_AddItemToObject(properties, "elevation-m", cJSON_CreateNumber(positioned->elevationM()));
  return properties;
}

static cJSON * createFeatureFor(FGPositionedRef positioned)
{
  cJSON * feature = cJSON_CreateObject();

  // A GeoJSON object with the type "Feature" is a feature object.
  cJSON_AddItemToObject(feature, "type", cJSON_CreateString("Feature"));

  // A feature object must have a member with the name "geometry".
  // The value of the geometry member is a geometry object as defined above or a JSON null value.
  cJSON_AddItemToObject(feature, "geometry", createGeometryFor(positioned));

  // A feature object must have a member with the name "properties".
  // The value of the properties member is an object (any JSON object or a JSON null value).
  cJSON_AddItemToObject(feature, "properties", createPropertiesFor(positioned));

  // If a feature has a commonly used identifier, that identifier should be included
  // as a member of the feature object with the name "id".
  cJSON_AddItemToObject(feature, "id", cJSON_CreateString(positioned->ident().c_str()));

  return feature;
}

bool NavdbUriHandler::handleRequest(const HTTPRequest & request, HTTPResponse & response)
{
  response.Header["Content-Type"] = "application/json; charset=UTF-8";

  bool indent = request.RequestVariables.get("i") == "y";
  string query = request.RequestVariables.get("q");
  FGPositionedList result;

  if (query == "findWithinRange") {
    // ?q=findWithinRange&lat=53.5&lon=10.0&range=100&type=vor,ils

    double lat, lon, range = -1;
    try {
      lat = boost::lexical_cast<double>(request.RequestVariables.get("lat"));
      lon = boost::lexical_cast<double>(request.RequestVariables.get("lon"));
      range = boost::lexical_cast<double>(request.RequestVariables.get("range"));
    }
    catch (...) {
      goto fail;
    }

    if (range <= 1.0) goto fail;
    // In remembrance of a famous bug

    SGGeod pos = SGGeod::fromDeg(lon, lat);
    FGPositioned::TypeFilter filter;
    try {
      filter = FGPositioned::TypeFilter::fromString(request.RequestVariables.get("type"));
    }
    catch (...) {
      goto fail;
    }

    result = FGPositioned::findWithinRange(pos, range, &filter);
  } else {
    goto fail;
  }

  { // create some GeoJSON from the result list
    // GeoJSON always consists of a single object.
    cJSON * geoJSON = cJSON_CreateObject();

    // The GeoJSON object must have a member with the name "type".
    // This member's value is a string that determines the type of the GeoJSON object.
    cJSON_AddItemToObject(geoJSON, "type", cJSON_CreateString("FeatureCollection"));

    // A GeoJSON object with the type "FeatureCollection" is a feature collection object.
    // An object of type "FeatureCollection" must have a member with the name "features".
    // The value corresponding to "features" is an array.
    cJSON * featureCollection = cJSON_CreateArray();
    cJSON_AddItemToObject(geoJSON, "features", featureCollection);

    for (FGPositionedList::iterator it = result.begin(); it != result.end(); ++it) {
      // Each element in the array is a feature object as defined above.
      cJSON_AddItemToArray(featureCollection, createFeatureFor(*it));
    }

    char * jsonString = indent ? cJSON_Print(geoJSON) : cJSON_PrintUnformatted(geoJSON);
    cJSON_Delete(geoJSON);
    response.Content = jsonString;
    free(jsonString);
  }

  return true;

  fail: response.StatusCode = 400;
  response.Content = "{ 'error': 'bad request' }";
  return true;
}

} // namespace http
} // namespace flightgear

