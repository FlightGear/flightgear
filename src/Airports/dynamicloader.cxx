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
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstdlib>
#include <cstring> // for strcmp

#include "dynamicloader.hxx"

/*****************************************************************************
 * Helper function for parsing position string
 ****************************************************************************/
static double processPosition(const string &pos)
{
  string prefix;
  string subs;
  string degree;
  string decimal;
  int sign = 1;
  double value;
  subs = pos;
  prefix= subs.substr(0,1);
  if (prefix == string("S") || (prefix == string("W")))
    sign = -1;
  subs    = subs.substr(1, subs.length());
  degree  = subs.substr(0, subs.find(" ",0));
  decimal = subs.substr(subs.find(" ",0), subs.length());
  
  value = sign * (atof(degree.c_str()) + atof(decimal.c_str())/60.0);
  return value;
}

FGAirportDynamicsXMLLoader::FGAirportDynamicsXMLLoader(FGAirportDynamics* dyn):
    XMLVisitor(), _dynamics(dyn)
{}

void  FGAirportDynamicsXMLLoader::startXML () {
  //cout << "FGAirportDynamicsLoader::Start XML" << endl;
}

void  FGAirportDynamicsXMLLoader::endXML () {
  //cout << "End XML" << endl;
}

void FGAirportDynamicsXMLLoader::startParking(const XMLAttributes &atts)
{
  string type;
  int index;
  string gateName, gateNumber;
  string lat, lon;
  double heading = 0.0;
  double radius = 1.0;
  string airlineCodes;
  int pushBackRoute = 0;
  
  for (int i = 0; i < atts.size(); i++)
	{
    string attname(atts.getName(i));
	  if (attname == "index") {
      index = std::atoi(atts.getValue(i));
    } else if (attname == "type")
	    type = atts.getValue(i);
    else if (attname == "name")
      gateName = atts.getValue(i);
	  else if (attname == "number")
	    gateNumber = atts.getValue(i);
	  else if (attname == "lat")
      lat = atts.getValue(i);
	  else if (attname == "lon")
	    lon = atts.getValue(i);
	  else if (attname == "heading")
	    heading = std::atof(atts.getValue(i));
	  else if (attname == "radius") {
	    string radiusStr = atts.getValue(i);
	    if (radiusStr.find("M") != string::npos)
	      radiusStr = radiusStr.substr(0, radiusStr.find("M",0));
	    radius = std::atof(radiusStr.c_str());
	  }
	  else if (attname == "airlineCodes")
      airlineCodes = atts.getValue(i);
    else if (attname == "pushBackRoute") {
      pushBackRoute = std::atoi(atts.getValue(i));      
    }
	}
 
  SGGeod pos(SGGeod::fromDeg(processPosition(lon), processPosition(lat)));
  
  FGParking* pk = new FGParking(0, index, pos, heading, radius,
                                gateName + gateNumber, type, airlineCodes);
  pk->setPushBackPoint(pushBackRoute);
  _dynamics->addParking(pk);
}

void FGAirportDynamicsXMLLoader::startNode(const XMLAttributes &atts)
{
  int index;
  string lat, lon;
  bool onRunway;
  int holdPointType;
  
  for (int i = 0; i < atts.size() ; i++)
	{
	  string attname(atts.getName(i));
	  if (attname == "index")
	    index = std::atoi(atts.getValue(i));
	  else if (attname == "lat")
      lat = atts.getValue(i);
	  else if (attname == "lon")
	    lon = atts.getValue(i);
    else if (attname == "isOnRunway")
      onRunway = (bool) std::atoi(atts.getValue(i));
	  else if (attname == "holdPointType") {
      string attval = atts.getValue(i);
      if (attval=="none") {
        holdPointType=0;
      } else if (attval=="normal") {
        holdPointType=1;
      } else if (attval=="CAT II/III") {
        holdPointType=3;
      } else if (attval=="PushBack") {
        holdPointType=3;
      } else {
        holdPointType=0;
      }
    }
	}
  
  SGGeod pos(SGGeod::fromDeg(processPosition(lon), processPosition(lat)));
  FGTaxiNode* taxiNode = new FGTaxiNode(0, index, pos, onRunway, holdPointType);
  _dynamics->getGroundNetwork()->addNode(taxiNode);
}

void FGAirportDynamicsXMLLoader::startArc(const XMLAttributes &atts)
{  
  int begin, end;
  bool isPushBackRoute = false;
  
  for (int i = 0; i < atts.size() ; i++)
	{
	  string attname = atts.getName(i);
	  if (attname == "begin")
	    begin = std::atoi(atts.getValue(i));
	  else if (attname == "end")
	    end = std::atoi(atts.getValue(i));
    else if (attname == "isPushBackRoute")
	    isPushBackRoute = (bool) std::atoi(atts.getValue(i));
	}
  
  _dynamics->getGroundNetwork()->addSegment(new FGTaxiSegment(begin, end, isPushBackRoute));
}

void FGAirportDynamicsXMLLoader::startElement (const char * name, const XMLAttributes &atts)
{
  if (!strcmp("Parking", name)) {
    startParking(atts);
  } else if (!strcmp("node", name)) {
    startNode(atts);
  } else if (!strcmp("arc", name)) {
    startArc(atts);
  }
}

void  FGAirportDynamicsXMLLoader::endElement (const char * name)
{
  int valueAsInt = atoi(value.c_str());
  if (!strcmp("version", name)) {
    _dynamics->getGroundNetwork()->addVersion(valueAsInt);
  } else if (!strcmp("AWOS", name)) {
    _dynamics->addAwosFreq(valueAsInt);
  } else if (!strcmp("UNICOM", name)) {
    _dynamics->addUnicomFreq(valueAsInt);
  } else if (!strcmp("CLEARANCE", name)) {
    _dynamics->addClearanceFreq(valueAsInt);
  } else if (!strcmp("GROUND", name)) {
    _dynamics->addGroundFreq(valueAsInt);
  } else if (!strcmp("TOWER", name)) {
    _dynamics->addTowerFreq(valueAsInt);
  } else if (!strcmp("APPROACH", name)) {
    _dynamics->addApproachFreq(valueAsInt);
  }
}

void  FGAirportDynamicsXMLLoader::data (const char * s, int len) {
  string token = string(s,len);
  //cout << "Character data " << string(s,len) << endl;
  if ((token.find(" ") == string::npos && (token.find('\n')) == string::npos))
    value += token;
  else
    value = string("");
}

void  FGAirportDynamicsXMLLoader::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void  FGAirportDynamicsXMLLoader::warning (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_WARN, "Warning: " << message << " (" << line << ',' << column << ')');
}

void  FGAirportDynamicsXMLLoader::error (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_ALERT, "Error: " << message << " (" << line << ',' << column << ')');
}
