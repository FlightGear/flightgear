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

#include <string>
#include <cstdlib>
#include <cstring> // for strcmp

#include "dynamicloader.hxx"

#include <Navaids/NavDataCache.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/groundnetwork.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>

using std::string;

/*****************************************************************************
 * Helper function for parsing position string
 ****************************************************************************/
static double processPosition(const string &pos)
{
    string subs{pos};
    int sign = 1;
    const auto prefix = subs.substr(0, 1);
    if (prefix == "S" || (prefix == "W"))
        sign = -1;
    subs = subs.substr(1, subs.length()); // drop first character
    const auto spacePos = subs.find(" ", 0);
    const auto degree = subs.substr(0, spacePos);
    const auto decimal = subs.substr(spacePos, subs.length());

    return sign * (stoi(degree) + (stof(decimal) / 60.0));
}

FGGroundNetXMLLoader::FGGroundNetXMLLoader(FGGroundNetwork* net):
    XMLVisitor(),
    _groundNetwork(net)
{}

void  FGGroundNetXMLLoader::startXML () {
  //cout << "FGAirportDynamicsLoader::Start XML" << endl;
}

void  FGGroundNetXMLLoader::endXML ()
{
  ParkingPushbackIndex::const_iterator it;

  for (it = _parkingPushbacks.begin(); it != _parkingPushbacks.end(); ++it) {
    NodeIndexMap::const_iterator j = _indexMap.find(it->second);
    if (j == _indexMap.end()) {
      SG_LOG(SG_NAVAID, SG_DEV_WARN, "bad groundnet, no node for index:" << it->first);
      continue;
    }

    it->first->setPushBackPoint(j->second);

  }

  for (const FGTaxiNodeRef& node: _unreferencedNodes) {
    SG_LOG(SG_NAVAID, SG_DEV_WARN,
           "unreferenced groundnet node: " << node->getIndex());
  }

}

void FGGroundNetXMLLoader::startParking(const XMLAttributes &atts)
{
  string type;
  int index = 0;
  string gateName, gateNumber;
  string lat, lon;
  double heading = 0.0;
  double radius = 1.0;
  string airlineCodes;
  int pushBackRoute = -1;       // signals unseen attribute

  savePosition();               // allows to retrieve the line number

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
      const string attrVal = atts.getValue(i);
      try {
          if (attrVal.empty() || (attrVal == "None")) {
              pushBackRoute = -2;
          } else {
              pushBackRoute = simgear::strutils::readNonNegativeInt<int>(attrVal);
          }
      } catch (const sg_exception& e) {
        SG_LOG(SG_NAVAID, SG_DEV_WARN,
               getPath() << ":" << getLine() << ": " <<
               "invalid value for 'pushBackRoute': " << e.what());
        pushBackRoute = -2;
      }
    }
  }

  SGGeod pos(SGGeod::fromDeg(processPosition(lon), processPosition(lat)));

  FGParkingRef parking(new FGParking(index,
                                     pos, heading, radius,
                                     gateName + gateNumber,
                                     type, airlineCodes));
  if (pushBackRoute >= 0) {
    _parkingPushbacks[parking] = pushBackRoute;
  }

  _indexMap[index] = parking;
  _groundNetwork->addParking(parking);
}

void FGGroundNetXMLLoader::startNode(const XMLAttributes &atts)
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

  if (_indexMap.find(index) != _indexMap.end()) {
    SG_LOG(SG_NAVAID, SG_DEV_WARN, "duplicate ground-net index:" << index);
  }

  SGGeod pos(SGGeod::fromDeg(processPosition(lon), processPosition(lat)));
  FGTaxiNodeRef node(new FGTaxiNode(index, pos, onRunway, holdPointType));
  _indexMap[index] = node;
  _unreferencedNodes.insert(node);
}

void FGGroundNetXMLLoader::startArc(const XMLAttributes &atts)
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
    SG_LOG(SG_NAVAID, SG_DEV_WARN, _groundNetwork->airport()->ident() << " ground-net: skipping duplicate edge:" << begin << "->" << end);
    return;
  }

  NodeIndexMap::const_iterator it;
  FGTaxiNodeRef fromNode, toNode;
  it = _indexMap.find(begin);
  if (it == _indexMap.end()) {
      SG_LOG(SG_NAVAID, SG_DEV_WARN, "ground-net: bad edge:" << begin << "->" << end << ", begin index unknown");
      return;
  } else {
      _unreferencedNodes.erase(it->second);
      fromNode = it->second;
  }

  it = _indexMap.find(end);
  if (it == _indexMap.end()) {
      SG_LOG(SG_NAVAID, SG_DEV_WARN, "ground-net: bad edge:" << begin << "->" << end << ", end index unknown");
      return;
  } else {
      _unreferencedNodes.erase(it->second);
      toNode = it->second;
  }

  _arcSet.insert(e);
  _groundNetwork->addSegment(fromNode, toNode);
  if (isPushBackRoute) {
//    toNode->setIsPushback();
  }
}

void FGGroundNetXMLLoader::startElement (const char * name, const XMLAttributes &atts)
{
  if (!strcmp("Parking", name)) {
    startParking(atts);
  } else if (!strcmp("node", name)) {
    startNode(atts);
  } else if (!strcmp("arc", name)) {
    startArc(atts);
  }
}

void  FGGroundNetXMLLoader::endElement (const char * name)
{
  int valueAsInt = atoi(value.c_str());
  if (!strcmp("version", name)) {
    _groundNetwork->addVersion(valueAsInt);
  } else if (!strcmp("AWOS", name)) {
    _groundNetwork->addAwosFreq(valueAsInt);
  } else if (!strcmp("UNICOM", name)) {
    _groundNetwork->addUnicomFreq(valueAsInt);
  } else if (!strcmp("CLEARANCE", name)) {
    _groundNetwork->addClearanceFreq(valueAsInt);
  } else if (!strcmp("GROUND", name)) {
    _groundNetwork->addGroundFreq(valueAsInt);
  } else if (!strcmp("TOWER", name)) {
    _groundNetwork->addTowerFreq(valueAsInt);
  } else if (!strcmp("APPROACH", name)) {
    _groundNetwork->addApproachFreq(valueAsInt);
  }
}

void  FGGroundNetXMLLoader::data (const char * s, int len) {
  string token = string(s,len);
  //cout << "Character data " << string(s,len) << endl;
  if ((token.find(" ") == string::npos && (token.find('\n')) == string::npos))
    value += token;
  else
    value = string("");
}

void  FGGroundNetXMLLoader::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void  FGGroundNetXMLLoader::warning (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_WARN, "Warning: " << message << " (" << line << ',' << column << ')');
}

void  FGGroundNetXMLLoader::error (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_ALERT, "Error: " << message << " (" << line << ',' << column << ')');
}
