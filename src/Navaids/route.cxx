// route.cxx - classes supporting waypoints and route structures

// Written by James Turner, started 2009.
//
// Copyright (C) 2009  Curtis L. Olson
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "route.hxx"

// std
#include <map>
#include <fstream>

// Boost
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string.hpp>

// SimGear
#include <simgear/structure/exception.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

// FlightGear
#include <Main/globals.hxx>
#include <Navaids/procedure.hxx>
#include <Navaids/waypoint.hxx>
#include <Airports/simple.hxx>

using std::string;
using std::vector;
using std::endl;
using std::fstream;

namespace flightgear {

const double NO_MAG_VAR = -1000.0; // an impossible mag-var value

Waypt::Waypt(Route* aOwner) :
  _altitudeFt(0.0),
  _speed(0.0),
  _altRestrict(RESTRICT_NONE),
  _speedRestrict(RESTRICT_NONE),
  _owner(aOwner),
  _flags(0),
  _magVarDeg(NO_MAG_VAR)
{
}

Waypt::~Waypt()
{
}
  
std::string Waypt::ident() const
{
  return "";
}
  
bool Waypt::flag(WayptFlag aFlag) const
{
  return ((_flags & aFlag) != 0);
}
	
void Waypt::setFlag(WayptFlag aFlag, bool aV)
{
  _flags = (_flags & ~aFlag);
  if (aV) _flags |= aFlag;
}

bool Waypt::matches(Waypt* aOther) const
{
  assert(aOther);
  if (ident() != aOther->ident()) { // cheap check first
    return false;
  }
  
  return matches(aOther->position());
}


bool Waypt::matches(const SGGeod& aPos) const
{
  double d = SGGeodesy::distanceM(position(), aPos);
  return (d < 100.0); // 100 metres seems plenty
}

void Waypt::setAltitude(double aAlt, RouteRestriction aRestrict)
{
  _altitudeFt = aAlt;
  _altRestrict = aRestrict;
}

void Waypt::setSpeed(double aSpeed, RouteRestriction aRestrict)
{
  _speed = aSpeed;
  _speedRestrict = aRestrict;
}

double Waypt::speedKts() const
{
  assert(_speedRestrict != SPEED_RESTRICT_MACH);
  return speed();
}
  
double Waypt::speedMach() const
{
  assert(_speedRestrict == SPEED_RESTRICT_MACH);
  return speed();
}
  
std::pair<double, double>
Waypt::courseAndDistanceFrom(const SGGeod& aPos) const
{
  if (flag(WPT_DYNAMIC)) {
    return std::make_pair(0.0, 0.0);
  }
  
  double course, az2, distance;
  SGGeodesy::inverse(aPos, position(), course, az2, distance);
  return std::make_pair(course, distance);
}

double Waypt::magvarDeg() const
{
  if (_magVarDeg == NO_MAG_VAR) {
    // derived classes with a default pos must override this method
    assert(!(position() == SGGeod()));
    
    double jd = globals->get_time_params()->getJD();
    _magVarDeg = sgGetMagVar(position(), jd);
  }
  
  return _magVarDeg;
}
  
///////////////////////////////////////////////////////////////////////////
// persistence

static RouteRestriction restrictionFromString(const char* aStr)
{
  std::string l = boost::to_lower_copy(std::string(aStr));

  if (l == "at") return RESTRICT_AT;
  if (l == "above") return RESTRICT_ABOVE;
  if (l == "below") return RESTRICT_BELOW;
  if (l == "none") return RESTRICT_NONE;
  if (l == "mach") return SPEED_RESTRICT_MACH;
  
  if (l.empty()) return RESTRICT_NONE;
  throw sg_io_exception("unknown restriction specification:" + l, 
    "Route restrictFromString");
}

static const char* restrictionToString(RouteRestriction aRestrict)
{
  switch (aRestrict) {
  case RESTRICT_AT: return "at";
  case RESTRICT_BELOW: return "below";
  case RESTRICT_ABOVE: return "above";
  case RESTRICT_NONE: return "none";
  case SPEED_RESTRICT_MACH: return "mach";
  
  default:
    throw sg_exception("invalid route restriction",
      "Route restrictToString");
  }
}

Waypt* Waypt::createInstance(Route* aOwner, const std::string& aTypeName)
{
  Waypt* r = NULL;
  if (aTypeName == "basic") {
    r = new BasicWaypt(aOwner);
  } else if (aTypeName == "navaid") {
    r = new NavaidWaypoint(aOwner);
  } else if (aTypeName == "offset-navaid") {
    r = new OffsetNavaidWaypoint(aOwner);
  } else if (aTypeName == "hold") {
    r = new Hold(aOwner);
  } else if (aTypeName == "runway") {
    r = new RunwayWaypt(aOwner);
  } else if (aTypeName == "hdgToAlt") {
    r = new HeadingToAltitude(aOwner);
  } else if (aTypeName == "dmeIntercept") {
    r = new DMEIntercept(aOwner);
  } else if (aTypeName == "radialIntercept") {
    r = new RadialIntercept(aOwner);
  } else if (aTypeName == "vectors") {
    r = new ATCVectors(aOwner);
  } 

  if (!r || (r->type() != aTypeName)) {
    throw sg_exception("broken factory method for type:" + aTypeName,
      "Waypt::createInstance");
  }
  
  return r;
}

WayptRef Waypt::createFromProperties(Route* aOwner, SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("type")) {
    throw sg_io_exception("bad props node, no type provided", 
      "Waypt::createFromProperties");
  }
  
  WayptRef nd(createInstance(aOwner, aProp->getStringValue("type")));
  nd->initFromProperties(aProp);
  return nd;
}
  
void Waypt::saveAsNode(SGPropertyNode* n) const
{
  n->setStringValue("type", type());
  writeToProperties(n);
}

void Waypt::initFromProperties(SGPropertyNode_ptr aProp)
{
  if (aProp->hasChild("generated")) {
    setFlag(WPT_GENERATED, aProp->getBoolValue("generated")); 
  }
  
  if (aProp->hasChild("overflight")) {
    setFlag(WPT_OVERFLIGHT, aProp->getBoolValue("overflight")); 
  }
  
  if (aProp->hasChild("arrival")) {
    setFlag(WPT_ARRIVAL, aProp->getBoolValue("arrival")); 
  }
  
  if (aProp->hasChild("departure")) {
    setFlag(WPT_DEPARTURE, aProp->getBoolValue("departure")); 
  }
  
  if (aProp->hasChild("miss")) {
    setFlag(WPT_MISS, aProp->getBoolValue("miss")); 
  }
  
  if (aProp->hasChild("alt-restrict")) {
    _altRestrict = restrictionFromString(aProp->getStringValue("alt-restrict"));
    _altitudeFt = aProp->getDoubleValue("altitude-ft");
  }
  
  if (aProp->hasChild("speed-restrict")) {
    _speedRestrict = restrictionFromString(aProp->getStringValue("speed-restrict"));
    _speed = aProp->getDoubleValue("speed");
  }
  
  
}

void Waypt::writeToProperties(SGPropertyNode_ptr aProp) const
{
  if (flag(WPT_OVERFLIGHT)) {
    aProp->setBoolValue("overflight", true);
  }

  if (flag(WPT_DEPARTURE)) {
    aProp->setBoolValue("departure", true);
  }
  
  if (flag(WPT_ARRIVAL)) {
    aProp->setBoolValue("arrival", true);
  }
  
  if (flag(WPT_MISS)) {
    aProp->setBoolValue("miss", true);
  }
  
  if (flag(WPT_GENERATED)) {
    aProp->setBoolValue("generated", true);
  }
  
  if (_altRestrict != RESTRICT_NONE) {
    aProp->setStringValue("alt-restrict", restrictionToString(_altRestrict));
    aProp->setDoubleValue("altitude-ft", _altitudeFt);
  }
  
  if (_speedRestrict != RESTRICT_NONE) {
    aProp->setStringValue("speed-restrict", restrictionToString(_speedRestrict));
    aProp->setDoubleValue("speed", _speed);
  }
}

void Route::dumpRouteToFile(const WayptVec& aRoute, const std::string& aName)
{
  SGPath p = "/Users/jmt/Desktop/" + aName + ".kml";
  std::fstream f;
  f.open(p.str().c_str(), fstream::out | fstream::app);
  if (!f.is_open()) {
    SG_LOG(SG_GENERAL, SG_WARN, "unable to open:" << p.str());
    return;
  }
  
// pre-amble
  f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
    "<Document>\n";

  dumpRouteToLineString(aName, aRoute, f);
  
// post-amble
  f << "</Document>\n" 
    "</kml>" << endl;
  f.close();
}

void Route::dumpRouteToLineString(const std::string& aIdent,
  const WayptVec& aRoute, std::ostream& aStream)
{
  // preamble
  aStream << "<Placemark>\n";
  aStream << "<name>" << aIdent << "</name>\n";
  aStream << "<LineString>\n";
  aStream << "<tessellate>1</tessellate>\n";
  aStream << "<coordinates>\n";
  
  // waypoints
  for (unsigned int i=0; i<aRoute.size(); ++i) {
    SGGeod pos = aRoute[i]->position();
    aStream << pos.getLongitudeDeg() << "," << pos.getLatitudeDeg() << " " << endl;
  }
  
  // postable
  aStream << "</coordinates>\n"
    "</LineString>\n"
    "</Placemark>\n" << endl;
}

///////////////////////////////////////////////////////////////////////////

class NavdataVisitor : public XMLVisitor {
public:
  NavdataVisitor(FGAirport* aApt, const SGPath& aPath);

protected:
  virtual void startXML (); 
  virtual void endXML   ();
  virtual void startElement (const char * name, const XMLAttributes &atts);
  virtual void endElement (const char * name);
  virtual void data (const char * s, int len);
  virtual void pi (const char * target, const char * data);
  virtual void warning (const char * message, int line, int column);
  virtual void error (const char * message, int line, int column);

private:
  Waypt* buildWaypoint();
  void processRunways(ArrivalDeparture* aProc, const XMLAttributes &atts);
 
  void finishApproach();
  void finishSid();
  void finishStar();
  
  FGAirport* _airport;
  SGPath _path;
  string _text; ///< last element text value
  
  SID* _sid;
  STAR* _star;
  Approach* _approach;

  WayptVec _waypoints; ///< waypoint list for current approach/sid/star
  WayptVec _transWaypts; ///< waypoint list for current transition
  
  string _wayptName;
  string _wayptType;
  string _ident; // id of segment under construction
  string _transIdent;
  double _longitude, _latitude, _altitude, _speed;
  RouteRestriction _altRestrict;
  
  double _holdRadial; // inbound hold radial, or -1 if radial is 'inbound'
  double _holdTD; ///< hold time (seconds) or distance (nm), based on flag below
  bool _holdRighthanded;
  bool _holdDistance; // true, TD is distance in nm; false, TD is time in seconds
  
  double _course, _radial, _dmeDistance;
};

void Route::loadAirportProcedures(const SGPath& aPath, FGAirport* aApt)
{
  assert(aApt);
  try {
    NavdataVisitor visitor(aApt, aPath);
    readXML(aPath.str(), visitor);
  } catch (sg_io_exception& ex) {
    SG_LOG(SG_GENERAL, SG_WARN, "failured parsing procedures: " << aPath.str() <<
      "\n\t" << ex.getMessage() << "\n\tat:" << ex.getLocation().asString());
  } catch (sg_exception& ex) {
    SG_LOG(SG_GENERAL, SG_WARN, "failured parsing procedures: " << aPath.str() <<
      "\n\t" << ex.getMessage());
  }
}

NavdataVisitor::NavdataVisitor(FGAirport* aApt, const SGPath& aPath):
  _airport(aApt),
  _path(aPath),
  _sid(NULL),
  _star(NULL),
  _approach(NULL)
{
}

void NavdataVisitor::startXML()
{
}

void NavdataVisitor::endXML()
{
}

void NavdataVisitor::startElement(const char* name, const XMLAttributes &atts)
{
  _text.clear();
  string tag(name);
  if (tag == "Airport") {
    string icao(atts.getValue("ICAOcode"));
    if (_airport->ident() != icao) {
      throw sg_format_exception("Airport and ICAO mismatch", icao, _path.str());
    }
  } else if (tag == "Sid") {
    string ident(atts.getValue("Name"));
    _sid = new SID(ident);
    _waypoints.clear();
    processRunways(_sid, atts);
  } else if (tag == "Star") {
    string ident(atts.getValue("Name"));
    _star = new STAR(ident);
    _waypoints.clear();
    processRunways(_star, atts);
  } else if ((tag == "Sid_Waypoint") ||
      (tag == "App_Waypoint") ||
      (tag == "Star_Waypoint") ||
      (tag == "AppTr_Waypoint") ||
      (tag == "SidTr_Waypoint") ||
      (tag == "RwyTr_Waypoint"))
  {
    // reset waypoint data
    _speed = 0.0;
    _altRestrict = RESTRICT_NONE;
    _altitude = 0.0;
  } else if (tag == "Approach") {
    _ident = atts.getValue("Name");
    _waypoints.clear();
    _approach = new Approach(_ident);
  } else if ((tag == "Sid_Transition") || 
             (tag == "App_Transition") ||
             (tag == "Star_Transition")) {
    _transIdent = atts.getValue("Name");
    _transWaypts.clear();
  } else if (tag == "RunwayTransition") {
    _transIdent = atts.getValue("Runway");
    _transWaypts.clear();
  } else {
    
  }
}

void NavdataVisitor::processRunways(ArrivalDeparture* aProc, const XMLAttributes &atts)
{
  string v("All");
  if (atts.hasAttribute("Runways")) {
    v = atts.getValue("Runways");
  }
  
  if (v == "All") {
    for (unsigned int r=0; r<_airport->numRunways(); ++r) {
      aProc->addRunway(_airport->getRunwayByIndex(r));
    }
    return;
  }
  
  vector<string> rwys;
  boost::split(rwys, v, boost::is_any_of(" ,"));
  for (unsigned int r=0; r<rwys.size(); ++r) {
    FGRunway* rwy = _airport->getRunwayByIdent(rwys[r]);
    aProc->addRunway(rwy);
  }
}

void NavdataVisitor::endElement(const char* name)
{
  string tag(name);
  if ((tag == "Sid_Waypoint") ||
      (tag == "App_Waypoint") ||
      (tag == "Star_Waypoint"))
  {
    _waypoints.push_back(buildWaypoint());
  } else if ((tag == "AppTr_Waypoint") || 
             (tag == "SidTr_Waypoint") ||
             (tag == "RwyTr_Waypoint") ||
             (tag == "StarTr_Waypoint")) 
  {
    _transWaypts.push_back(buildWaypoint());
  } else if (tag == "Sid_Transition") {
    assert(_sid);
    // SID waypoints are stored backwards, to share code with STARs
    std::reverse(_transWaypts.begin(), _transWaypts.end());
    Transition* t = new Transition(_transIdent, _sid, _transWaypts);
    _sid->addTransition(t);
  } else if (tag == "Star_Transition") {
    assert(_star);
    Transition* t = new Transition(_transIdent, _star, _transWaypts);
    _star->addTransition(t);
  } else if (tag == "App_Transition") {
    assert(_approach);
    Transition* t = new Transition(_transIdent, _approach, _transWaypts);
    _approach->addTransition(t);
  } else if (tag == "RunwayTransition") {
    ArrivalDeparture* ad;
    if (_sid) {
      // SID waypoints are stored backwards, to share code with STARs
      std::reverse(_transWaypts.begin(), _transWaypts.end());
      ad = _sid;
    } else {
      ad = _star;
    }
    
    Transition* t = new Transition(_transIdent, ad, _transWaypts);
    FGRunwayRef rwy = _airport->getRunwayByIdent(_transIdent);
    ad->addRunwayTransition(rwy, t);
  } else if (tag == "Approach") {
    finishApproach();
  } else if (tag == "Sid") {
    finishSid();
  } else if (tag == "Star") {
    finishStar();  
  } else if (tag == "Longitude") {
    _longitude = atof(_text.c_str());
  } else if (tag == "Latitude") {
    _latitude = atof(_text.c_str());
  } else if (tag == "Name") {
    _wayptName = _text;
  } else if (tag == "Type") {
    _wayptType = _text;
  } else if (tag == "Speed") {
    _speed = atoi(_text.c_str());
  } else if (tag == "Altitude") {
    _altitude = atof(_text.c_str());
  } else if (tag == "AltitudeRestriction") {
    if (_text == "at") {
      _altRestrict = RESTRICT_AT;
    } else if (_text == "above") {
      _altRestrict = RESTRICT_ABOVE;
    } else if (_text == "below") {
      _altRestrict = RESTRICT_BELOW;
    } else {
      throw sg_format_exception("Unrecognized altitude restriction", _text);
    }
  } else if (tag == "Hld_Rad_or_Inbd") {
    if (_text == "Inbd") {
      _holdRadial = -1.0;
    }
  } else if (tag == "Hld_Time_or_Dist") {
    _holdDistance = (_text == "Dist");
  } else if (tag == "Hld_Rad_value") {
    _holdRadial = atof(_text.c_str());
  } else if (tag == "Hld_Turn") {
    _holdRighthanded = (_text == "Right");
  } else if (tag == "Hld_td_value") {
    _holdTD = atof(_text.c_str());
  } else if (tag == "Hdg_Crs_value") {
    _course = atof(_text.c_str());
  } else if (tag == "DMEtoIntercept") {
    _dmeDistance = atof(_text.c_str());
  } else if (tag == "RadialtoIntercept") {
    _radial = atof(_text.c_str());
  } else {
    
  }
}

Waypt* NavdataVisitor::buildWaypoint()
{
  Waypt* wp = NULL;
  if (_wayptType == "Normal") {
    // new LatLonWaypoint
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude));
    wp = new BasicWaypt(pos, _wayptName, NULL);
  } else if (_wayptType == "Runway") {
    string ident = _wayptName.substr(2);
    FGRunwayRef rwy = _airport->getRunwayByIdent(ident);
    wp = new RunwayWaypt(rwy, NULL);
  } else if (_wayptType == "Hold") {
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude));
    Hold* h = new Hold(pos, _wayptName, NULL);
    wp = h;
    if (_holdRighthanded) {
      h->setRightHanded();
    } else {
      h->setLeftHanded();
    }
    
    if (_holdDistance) {
      h->setHoldDistance(_holdTD);
    } else {
      h->setHoldTime(_holdTD * 60.0);
    }
    
    if (_holdRadial >= 0.0) {
      h->setHoldRadial(_holdRadial);
    }
  } else if (_wayptType == "Vectors") {
    wp = new ATCVectors(NULL, _airport);
  } else if ((_wayptType == "Intc") || (_wayptType == "VorRadialIntc")) {
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude));
    wp = new RadialIntercept(NULL, _wayptName, pos, _course, _radial);
  } else if (_wayptType == "DmeIntc") {
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude));
    wp = new DMEIntercept(NULL, _wayptName, pos, _course, _dmeDistance);
  } else if (_wayptType == "ConstHdgtoAlt") {
    wp = new HeadingToAltitude(NULL, _wayptName, _course);
  } else {
    SG_LOG(SG_GENERAL, SG_ALERT, "implement waypoint type:" << _wayptType);
    throw sg_format_exception("Unrecognized waypt type", _wayptType);
  }
  
  assert(wp);
  if ((_altitude > 0.0) && (_altRestrict != RESTRICT_NONE)) {
    wp->setAltitude(_altitude,_altRestrict);
  }
  
  if (_speed > 0.0) {
    wp->setSpeed(_speed, RESTRICT_AT); // or _BELOW?
  }
  
  return wp;
}

void NavdataVisitor::finishApproach()
{
  WayptVec::iterator it;
  FGRunwayRef rwy;
  
// find the runway node
  for (it = _waypoints.begin(); it != _waypoints.end(); ++it) {
    FGPositionedRef navid = (*it)->source();
    if (!navid) {
      continue;
    }
    
    if (navid->type() == FGPositioned::RUNWAY) {
      rwy = (FGRunway*) navid.get();
      break;
    }
  }
  
  if (!rwy) {
    throw sg_format_exception("Malformed approach, no runway waypt", _ident);
  }
  
  WayptVec primary(_waypoints.begin(), it);
  // erase all points up to and including the runway, to leave only the
  // missed segments
  _waypoints.erase(_waypoints.begin(), ++it);
  
  _approach->setRunway(rwy);
  _approach->setPrimaryAndMissed(primary, _waypoints);
  _airport->addApproach(_approach);
  _approach = NULL;
}

void NavdataVisitor::finishSid()
{
  // reverse order, because that's how we deal with commonality between
  // STARs and SIDs. SID::route undoes  this
  std::reverse(_waypoints.begin(), _waypoints.end());
  _sid->setCommon(_waypoints);
  _airport->addSID(_sid);
  _sid = NULL;
}

void NavdataVisitor::finishStar()
{
  _star->setCommon(_waypoints);
  _airport->addSTAR(_star);
  _star = NULL;
}

void NavdataVisitor::data (const char * s, int len)
{
  _text += string(s, len);
}


void NavdataVisitor::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void NavdataVisitor::warning (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_WARN, "Warning: " << message << " (" << line << ',' << column << ')');
}

void NavdataVisitor::error (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_ALERT, "Error: " << message << " (" << line << ',' << column << ')');
}

} // of namespace flightgear
