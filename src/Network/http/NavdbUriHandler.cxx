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
#include <Navaids/navrecord.hxx>
#include <Airports/airport.hxx>
#include <3rdparty/cjson/cJSON.h>
#include <boost/lexical_cast.hpp>

using std::string;

namespace flightgear {
namespace http {

static cJSON * createPositionArray(double x, double y)
{
  cJSON * p = cJSON_CreateArray();
  cJSON_AddItemToArray(p, cJSON_CreateNumber(x));
  cJSON_AddItemToArray(p, cJSON_CreateNumber(y));
  return p;
}

static cJSON * createLOCGeometry(FGNavRecord * navRecord)
{
  assert( navRecord != NULL );

  cJSON * geometry = cJSON_CreateObject();
  int range = navRecord->get_range();

  double width = navRecord->localizerWidth();
  double course = navRecord->get_multiuse();

  double px[4];
  double py[4];

  px[0] = navRecord->longitude();
  py[0] = navRecord->latitude();

  for (int i = -1; i <= +1; i++) {
    double c = SGMiscd::normalizeAngle((course + 180 + i * width / 2) * SG_DEGREES_TO_RADIANS);
    SGGeoc geoc = SGGeoc::fromGeod(navRecord->geod());
    SGGeod p2 = SGGeod::fromGeoc(geoc.advanceRadM(c, range * SG_NM_TO_METER));
    px[i + 2] = p2.getLongitudeDeg();
    py[i + 2] = p2.getLatitudeDeg();
  }
  // Add three lines: centerline, left and right edge
  cJSON_AddItemToObject(geometry, "type", cJSON_CreateString("MultiLineString"));
  cJSON * coordinates = cJSON_CreateArray();
  cJSON_AddItemToObject(geometry, "coordinates", coordinates);
  for (int i = 1; i < 4; i++) {
    cJSON * line = cJSON_CreateArray();
    cJSON_AddItemToArray(coordinates, line);
    cJSON_AddItemToArray(line, createPositionArray(px[0], py[0]));
    cJSON_AddItemToArray(line, createPositionArray(px[i], py[i]));
  }

  return geometry;
}

static cJSON * createPointGeometry(FGPositionedRef positioned )
{
  cJSON * geometry = cJSON_CreateObject();
  cJSON_AddItemToObject(geometry, "type", cJSON_CreateString("Point"));
  cJSON_AddItemToObject(geometry, "coordinates", createPositionArray(positioned ->longitude(), positioned->latitude()));
  return geometry;
}

static cJSON * createAirportGeometry(FGAirport * airport )
{
  assert( airport != NULL );
  FGRunwayList runways = airport->getRunwaysWithoutReciprocals();

  if( runways.empty() ) {
    // no runways? Create a Point geometry
    return createPointGeometry( airport );
  }

  cJSON * geometry = cJSON_CreateObject();

  // if there are runways, create a geometry collection
  cJSON_AddItemToObject(geometry, "type", cJSON_CreateString("GeometryCollection"));
  cJSON * geometryCollection = cJSON_CreateArray();
  cJSON_AddItemToObject(geometry, "geometries", geometryCollection);

  // the first item is the aerodrome reference point
  cJSON_AddItemToArray( geometryCollection, createPointGeometry(airport) );

  // followed by the runway polygons
  for( FGRunwayList::iterator it = runways.begin(); it != runways.end(); ++it ) {
    cJSON * polygon = cJSON_CreateObject(); 
    cJSON_AddItemToArray( geometryCollection, polygon );
    cJSON_AddItemToObject(polygon, "type", cJSON_CreateString("Polygon"));
    cJSON * coordinates = cJSON_CreateArray();
    cJSON_AddItemToObject(polygon, "coordinates", coordinates );
    cJSON * linearRing = cJSON_CreateArray();
    cJSON_AddItemToArray( coordinates, linearRing );

    // compute the four corners of the runway
    SGGeod p1 = (*it)->pointOffCenterline( 0.0, (*it)->widthM()/2 );
    SGGeod p2 = (*it)->pointOffCenterline( 0.0, -(*it)->widthM()/2 );
    SGGeod p3 = (*it)->pointOffCenterline( (*it)->lengthM(), -(*it)->widthM()/2 );
    SGGeod p4 = (*it)->pointOffCenterline( (*it)->lengthM(), (*it)->widthM()/2 );
    cJSON_AddItemToArray( linearRing, createPositionArray(p1.getLongitudeDeg(), p1.getLatitudeDeg()) );
    cJSON_AddItemToArray( linearRing, createPositionArray(p2.getLongitudeDeg(), p2.getLatitudeDeg()) );
    cJSON_AddItemToArray( linearRing, createPositionArray(p3.getLongitudeDeg(), p3.getLatitudeDeg()) );
    cJSON_AddItemToArray( linearRing, createPositionArray(p4.getLongitudeDeg(), p4.getLatitudeDeg()) );
    // close the ring
    cJSON_AddItemToArray( linearRing, createPositionArray(p1.getLongitudeDeg(), p1.getLatitudeDeg()) );
  }

  return geometry;
}

static cJSON * createGeometryFor(FGPositionedRef positioned)
{
  switch( positioned->type() ) {
    case FGPositioned::LOC:
    case FGPositioned::ILS:
      return createLOCGeometry( dynamic_cast<FGNavRecord*>(positioned.get()) );

    case FGPositioned::AIRPORT:
      return createAirportGeometry( dynamic_cast<FGAirport*>(positioned.get()) );

    default:
      return createPointGeometry( positioned );
  }
}

static void addAirportProperties(cJSON * json, FGAirport * airport )
{
  if( NULL == airport ) return;
  FGRunwayList runways = airport->getRunwaysWithoutReciprocals();
  double longestRunwayLength = 0.0;
  double longestRunwayHeading = 0.0;
  const char * longestRunwaySurface = "";
  for( FGRunwayList::iterator it = runways.begin(); it != runways.end(); ++it ) {
    FGRunwayRef runway = *it;
    if( runway->lengthM() > longestRunwayLength ) {
      longestRunwayLength = runway->lengthM();
      longestRunwayHeading = runway->headingDeg();
      longestRunwaySurface = runway->surfaceName();
    }
  }
  cJSON_AddItemToObject(json, "longestRwyLength_m", cJSON_CreateNumber(longestRunwayLength));
  cJSON_AddItemToObject(json, "longestRwyHeading_deg", cJSON_CreateNumber(longestRunwayHeading));
  cJSON_AddItemToObject(json, "longestRwySurface", cJSON_CreateString(longestRunwaySurface));
}

static void addNAVProperties(cJSON * json, FGNavRecord * navRecord )
{
  if( NULL == navRecord ) return;
  cJSON_AddItemToObject(json, "range_nm", cJSON_CreateNumber(navRecord->get_range()));
  cJSON_AddItemToObject(json, "frequency", cJSON_CreateNumber((double) navRecord->get_freq() / 100.0));
  switch (navRecord->type()) {
    case FGPositioned::ILS:
    case FGPositioned::LOC:
      cJSON_AddItemToObject(json, "localizer-course", cJSON_CreateNumber(navRecord->get_multiuse()));
      break;

    case FGPositioned::VOR:
      cJSON_AddItemToObject(json, "variation", cJSON_CreateNumber(navRecord->get_multiuse()));
      break;

    default:
      break;
  }
}

static cJSON * createPropertiesFor(FGPositionedRef positioned)
{
  cJSON * properties = cJSON_CreateObject();

  cJSON_AddItemToObject(properties, "name", cJSON_CreateString(positioned->name().c_str()));
  // also add id to properties
  cJSON_AddItemToObject(properties, "id", cJSON_CreateString(positioned->ident().c_str()));
  cJSON_AddItemToObject(properties, "type", cJSON_CreateString(positioned->typeString()));
  cJSON_AddItemToObject(properties, "elevation-m", cJSON_CreateNumber(positioned->elevationM()));
  addNAVProperties( properties, dynamic_cast<FGNavRecord*>(positioned.get()) );
  addAirportProperties( properties, dynamic_cast<FGAirport*>(positioned.get()) );
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

bool NavdbUriHandler::handleRequest(const HTTPRequest & request, HTTPResponse & response, Connection * connection)
{

  response.Header["Content-Type"] = "application/json; charset=UTF-8";
  response.Header["Access-Control-Allow-Origin"] = "*";
  response.Header["Access-Control-Allow-Methods"] = "OPTIONS, GET";
  response.Header["Access-Control-Allow-Headers"] = "Origin, Accept, Content-Type, X-Requested-With, X-CSRF-Token";

  if( request.Method == "OPTIONS" ){
      return true; // OPTIONS only needs the headers
  }

  if( request.Method != "GET" ){
    response.Header["Allow"] = "OPTIONS, GET";
    response.StatusCode = 405;
    response.Content = "{}";
    return true; 
  }

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

    // we send zero to many features - let's make it a FeatureCollection
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

