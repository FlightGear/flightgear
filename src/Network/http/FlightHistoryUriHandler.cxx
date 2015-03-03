// FlightHistoryUriHandler.cxx -- FlightHistory service
//
// Written by Torsten Dreyer, started February 2015.
//
// Copyright (C) 2015  Torsten Dreyer
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

#include "FlightHistoryUriHandler.hxx"
#include "SimpleDOM.hxx"
#include <3rdparty/cjson/cJSON.h>

#include <Aircraft/FlightHistory.hxx>
#include <Main/fg_props.hxx>
#include <sstream>
#include <boost/lexical_cast.hpp>

using std::string;
using std::stringstream;

namespace flightgear {
namespace http {

/*
{
  type: "Feature",
  geometry: {
    type: "LineString",
    coordinates: [lon,lat,alt],..]
  },
  properties: {
    type: "FlightHistory"
  },
}
*/

static const char * errorPage =
		"<html><head><title>Flight History</title></head><body>"
		"<h1>Flight History</h1>"
		"Supported formats:"
		"<ul>"
		"<li><a href='track.json'>JSON</a></li>"
		"<li><a href='track.kml'>KML</a></li>"
		"</ul>"
		"</body></html>";

static string FlightHistoryToJson(const SGGeodVec & history, size_t last_seen ) {
	cJSON * feature = cJSON_CreateObject();
	cJSON_AddItemToObject(feature, "type", cJSON_CreateString("Feature"));

	cJSON * lineString = cJSON_CreateObject();
	cJSON_AddItemToObject(feature, "geometry", lineString );

	cJSON * properties = cJSON_CreateObject();
	cJSON_AddItemToObject(feature, "properties", properties );
	cJSON_AddItemToObject(properties, "type", cJSON_CreateString("FlightHistory"));
	cJSON_AddItemToObject(properties, "last", cJSON_CreateNumber(last_seen));

	cJSON_AddItemToObject(lineString, "type", cJSON_CreateString("LineString"));
	cJSON * coordinates = cJSON_CreateArray();
	cJSON_AddItemToObject(lineString, "coordinates", coordinates);
	for (SGGeodVec::const_iterator it = history.begin(); it != history.end();
			++it) {
		cJSON * coordinate = cJSON_CreateArray();
		cJSON_AddItemToArray(coordinates, coordinate);

		cJSON_AddItemToArray(coordinate, cJSON_CreateNumber(it->getLongitudeDeg()));
		cJSON_AddItemToArray(coordinate, cJSON_CreateNumber(it->getLatitudeDeg()));
		cJSON_AddItemToArray(coordinate, cJSON_CreateNumber(it->getElevationM()));

	}

	char * jsonString = cJSON_PrintUnformatted(feature);
	string reply(jsonString);
	free(jsonString);
	cJSON_Delete(lineString);
	return reply;
}

static string AutoUpdateResponse(const HTTPRequest & request,
		const string & base, const string & interval) {

	string url = "http://";
	url.append(request.HeaderVariables.get("Host")).append(base);

	std::string reply("<?xml version='1.0' encoding='UTF-8'?>");
	DOMNode * kml = new DOMNode("kml");
	kml->setAttribute("xmlns", "http://www.opengis.net/kml/2.2");

	DOMNode * document = kml->addChild(new DOMNode("Document"));

	DOMNode * networkLink = document->addChild(new DOMNode("NetworkLink"));
	DOMNode * link = networkLink->addChild(new DOMNode("Link"));
	link->addChild(new DOMNode("href"))->addChild(new DOMTextElement(url));
	link->addChild(new DOMNode("refreshMode"))->addChild(
			new DOMTextElement("onInterval"));
	link->addChild(new DOMNode("refreshInterval"))->addChild(
			new DOMTextElement(interval.empty() ? "10" : interval));

	reply.append(kml->render());
	delete kml;
	return reply;
}

static string FlightHistoryToKml(const SGGeodVec & history,
		const HTTPRequest & request) {
	string interval = request.RequestVariables.get("interval");
	if (false == interval.empty()) {
		return AutoUpdateResponse(request, "/flighthistory/track.kml", interval);
	}

	std::string reply("<?xml version='1.0' encoding='UTF-8'?>");
	DOMNode * kml = new DOMNode("kml");
	kml->setAttribute("xmlns", "http://www.opengis.net/kml/2.2");

	DOMNode * document = kml->addChild(new DOMNode("Document"));

	document->addChild(new DOMNode("name"))->addChild(
			new DOMTextElement("FlightGear"));
	document->addChild(new DOMNode("description"))->addChild(
			new DOMTextElement("FlightGear Flight History"));

	DOMNode * style = document->addChild(new DOMNode("Style"))->setAttribute(
			"id", "flight-history");
	DOMNode * lineStyle = style->addChild(new DOMNode("LineStyle"));

	string lineColor = request.RequestVariables.get("LineColor");
	string lineWidth = request.RequestVariables.get("LineWidth");
	string polyColor = request.RequestVariables.get("PolyColor");

	lineStyle->addChild(new DOMNode("color"))->addChild(
			new DOMTextElement(lineColor.empty() ? "427ebfff" : lineColor));
	lineStyle->addChild(new DOMNode("width"))->addChild(
			new DOMTextElement(lineWidth.empty() ? "4" : lineWidth));

	lineStyle = style->addChild(new DOMNode("PolyStyle"));
	lineStyle->addChild(new DOMNode("color"))->addChild(
			new DOMTextElement(polyColor.empty() ? "fbfc4600" : polyColor));

	DOMNode * placemark = document->addChild(new DOMNode("Placemark"));
	placemark->addChild(new DOMNode("name"))->addChild(
			new DOMTextElement("Flight Path"));
	placemark->addChild(new DOMNode("styleUrl"))->addChild(
			new DOMTextElement("#flight-history"));

	DOMNode * linestring = placemark->addChild(new DOMNode("LineString"));
	linestring->addChild(new DOMNode("extrude"))->addChild(
			new DOMTextElement("1"));
	linestring->addChild(new DOMNode("tessalate"))->addChild(
			new DOMTextElement("1"));
	linestring->addChild(new DOMNode("altitudeMode"))->addChild(
			new DOMTextElement("absolute"));

	stringstream ss;

	for (SGGeodVec::const_iterator it = history.begin(); it != history.end();
			++it) {
		ss << (*it).getLongitudeDeg() << "," << (*it).getLatitudeDeg() << ","
				<< it->getElevationM() << " ";
	}

	linestring->addChild(new DOMNode("coordinates"))->addChild(
			new DOMTextElement(ss.str()));

	reply.append(kml->render());
	delete kml;
	return reply;
}

static bool GetJsonDouble(cJSON * json, const char * item, double & out) {
	cJSON * cj = cJSON_GetObjectItem(json, item);
	if (NULL == cj)
		return false;

	if (cj->type != cJSON_Number)
		return false;

	out = cj->valuedouble;

	return true;
}

static bool GetJsonBool(cJSON * json, const char * item, bool & out) {
	cJSON * cj = cJSON_GetObjectItem(json, item);
	if (NULL == cj)
		return false;

	if (cj->type == cJSON_True) {
		out = true;
		return true;
	}
	if (cj->type == cJSON_False) {
		out = true;
		return true;

	}
	return false;
}

bool FlightHistoryUriHandler::handleRequest(const HTTPRequest & request,
		HTTPResponse & response, Connection * connection) {
	response.Header["Access-Control-Allow-Origin"] = "*";
	response.Header["Access-Control-Allow-Methods"] = "OPTIONS, GET, POST";
	response.Header["Access-Control-Allow-Headers"] =
			"Origin, Accept, Content-Type, X-Requested-With, X-CSRF-Token";

	if (request.Method == "OPTIONS") {
		return true; // OPTIONS only needs the headers
	}

	FGFlightHistory* history =
			static_cast<FGFlightHistory*>(globals->get_subsystem("history"));

	double minEdgeLengthM = 50;
	string requestPath = request.Uri.substr(getUri().length());

	if (request.Method == "GET") {
	} else if (request.Method == "POST") {
		/*
		 * {
		 *   sampleIntervalSec: (number),
		 *   maxMemoryUseBytes: (number),
		 *   clearOnTakeoff: (bool),
		 *   enabled: (bool),
		 * }
		 */
		cJSON * json = cJSON_Parse(request.Content.c_str());
		if ( NULL != json) {
			double d = .0;
			bool b = false;
			bool doReinit = false;
			if (GetJsonDouble(json, "sampleIntervalSec", d)) {
				fgSetDouble("/sim/history/sample-interval-sec", d);
				doReinit = true;
			}
			if (GetJsonDouble(json, "maxMemoryUseBytes", d)) {
				fgSetDouble("/sim/history/max-memory-use-bytes", d);
				doReinit = true;
			}

			if (GetJsonBool(json, "clearOnTakeoff", b)) {
				fgSetBool("/sim/history/clear-on-takeoff", b);
				doReinit = true;
			}
			if (GetJsonBool(json, "enabled", b)) {
				fgSetBool("/sim/history/enabled", b);
			}

			if (doReinit) {
				history->reinit();
			}

			cJSON_Delete(json);
		}

		response.Content = "{}";
		return true;

	} else {
		SG_LOG(SG_NETWORK, SG_INFO,
				"PkgUriHandler: invalid request method '" << request.Method << "'");
		response.Header["Allow"] = "OPTIONS, GET, POST";
		response.StatusCode = 405;
		response.Content = "{}";
		return true;
	}

	if (requestPath == "track.kml") {
		response.Header["Content-Type"] =
				"application/vnd.google-earth.kml+xml; charset=UTF-8";

		response.Content = FlightHistoryToKml(
				history->pathForHistory(minEdgeLengthM), request);

	} else if (requestPath == "track.json") {
		size_t count = -1;
		try {
		  count = boost::lexical_cast<size_t>(request.RequestVariables.get("count"));
		}
		catch( ... ) {
		}
		size_t last = 0;
		try {
			last = boost::lexical_cast<size_t>(request.RequestVariables.get("last"));
		}
		catch( ... ) {
		}

		response.Header["Content-Type"] = "application/json; charset=UTF-8";
		PagedPathForHistory_ptr h = history->pagedPathForHistory( count, last );
		response.Content = FlightHistoryToJson( h->path, h->last_seen );

	} else {
		response.Header["Content-Type"] = "text/html";
		response.Content = errorPage;
	}

	return true;
}

} // namespace http
} // namespace flightgear

