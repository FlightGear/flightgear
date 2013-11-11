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
#include <boost/foreach.hpp>

#include "dynamicloader.hxx"

#include <Navaids/NavDataCache.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/airport.hxx>

using std::string;

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

void  FGAirportDynamicsXMLLoader::endXML ()
{
  std::map<PositionedID, int>::iterator it;
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  
  for (it = _parkingPushbacks.begin(); it != _parkingPushbacks.end(); ++it) {
    std::map<int, PositionedID>::iterator j = _idMap.find(it->second);
    if (j == _idMap.end()) {
      SG_LOG(SG_NAVAID, SG_WARN, "bad groundnet, no node for index:" << it->first);
      continue;
    }
    
    cache->setParkingPushBackRoute(it->first, j->second);
  }
  
  BOOST_FOREACH(PositionedID id, _unreferencedNodes) {
    SG_LOG(SG_NAVAID, SG_WARN, "unreferenced groundnet node:" << id);
  }
  
}

void FGAirportDynamicsXMLLoader::startParking(const XMLAttributes &atts)
{
  string type;
  int index = 0;
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
  
  PositionedID guid = flightgear::NavDataCache::instance()->insertParking(gateName + gateNumber, pos,
                                                      _dynamics->parent()->guid(),
                                                      heading, radius, type, airlineCodes);
  if (pushBackRoute > 0) {
    _parkingPushbacks[guid] = pushBackRoute;
  }
  
  _idMap[index] = guid;
}

void FGAirportDynamicsXMLLoader::startNode(const XMLAttributes &atts)
{
  int index = 0;
  string lat, lon;
  bool onRunway = false;
  int holdPointType = 0;
  
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
      onRunway = std::atoi(atts.getValue(i)) != 0;
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
  
  if (_idMap.find(index) != _idMap.end()) {
    SG_LOG(SG_NAVAID, SG_WARN, "duplicate ground-net index:" << index);
  }
  
  SGGeod pos(SGGeod::fromDeg(processPosition(lon), processPosition(lat)));
  PositionedID guid = flightgear::NavDataCache::instance()->insertTaxiNode(pos,
    _dynamics->parent()->guid(), holdPointType, onRunway);
  _idMap[index] = guid;
  _unreferencedNodes.insert(guid);
}

void FGAirportDynamicsXMLLoader::startArc(const XMLAttributes &atts)
{  
  int begin = 0, end = 0;
  bool isPushBackRoute = false;
  
  for (int i = 0; i < atts.size() ; i++)
	{
	  string attname = atts.getName(i);
	  if (attname == "begin")
	    begin = std::atoi(atts.getValue(i));
	  else if (attname == "end")
	    end = std::atoi(atts.getValue(i));
    else if (attname == "isPushBackRoute")
	    isPushBackRoute = std::atoi(atts.getValue(i)) != 0;
	}
  
  IntPair e(begin, end);
  if (_arcSet.find(e) != _arcSet.end()) {
    SG_LOG(SG_NAVAID, SG_WARN, _dynamics->parent()->ident() << " ground-net: skipping duplicate edge:" << begin << "->" << end);
    return;
  }
  
  _arcSet.insert(e);
  flightgear::NavDataCache::instance()->insertGroundnetEdge(_dynamics->parent()->guid(),
                                                            _idMap[begin], _idMap[end]);
  
  _unreferencedNodes.erase(_idMap[begin]);
  _unreferencedNodes.erase(_idMap[end]);
  
  if (isPushBackRoute) {
    flightgear::NavDataCache::instance()->markGroundnetAsPushback(_idMap[end]);
  }
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
