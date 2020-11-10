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

#include "config.h"

#include "route.hxx"

// std
#include <map>
#include <fstream>


// SimGear
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props_io.hxx>

// FlightGear
#include <Main/globals.hxx>
#include "Main/fg_props.hxx"
#include <Navaids/procedure.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/LevelDXML.hxx>
#include <Airports/airport.hxx>
#include <Navaids/airways.hxx>

using std::string;
using std::vector;
using std::endl;
using std::fstream;

namespace flightgear {

const double NO_MAG_VAR = -1000.0; // an impossible mag-var value

bool isMachRestrict(RouteRestriction rr)
{
  return (rr == SPEED_RESTRICT_MACH) || (rr == SPEED_COMPUTED_MACH);
}

Waypt::Waypt(RouteBase* aOwner) :
  _owner(aOwner),
  _magVarDeg(NO_MAG_VAR)
{
}

Waypt::~Waypt()
{
}

std::string Waypt::ident() const
{
  return {};
}

bool Waypt::flag(WayptFlag aFlag) const
{
  return ((_flags & aFlag) != 0);
}

void Waypt::setFlag(WayptFlag aFlag, bool aV)
{
    if (aFlag == 0) {
        throw sg_range_exception("invalid waypoint flag set");
    }

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
    
bool Waypt::matches(FGPositioned* aPos) const
{
    if (!aPos)
        return false;
    
    // if w ehave no source, match on position and ident
    if (!source()) {
        return (ident() == aPos->ident()) && matches(aPos->geod());
    }
    
    return (aPos == source());
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

double Waypt::magvarDeg() const
{
  if (_magVarDeg == NO_MAG_VAR) {
    // derived classes with a default pos must override this method
    assert(!(position() == SGGeod()));

    double jd = globals->get_time_params()->getJD();
    _magVarDeg = sgGetMagVar(position(), jd) * SG_RADIANS_TO_DEGREES;
  }

  return _magVarDeg;
}

double Waypt::headingRadialDeg() const
{
  return 0.0;
}

std::string Waypt::icaoDescription() const
{
    return ident();
}

///////////////////////////////////////////////////////////////////////////
// persistence

static RouteRestriction restrictionFromString(const char* aStr)
{
  std::string l = simgear::strutils::lowercase(aStr);

  if (l == "at") return RESTRICT_AT;
  if (l == "above") return RESTRICT_ABOVE;
  if (l == "below") return RESTRICT_BELOW;
  if (l == "none") return RESTRICT_NONE;
  if (l == "mach") return SPEED_RESTRICT_MACH;

  if (l.empty()) return RESTRICT_NONE;
  throw sg_io_exception("unknown restriction specification:" + l,
    "Route restrictFromString");
}

const char* restrictionToString(RouteRestriction aRestrict)
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

WayptRef Waypt::createInstance(RouteBase* aOwner, const std::string& aTypeName)
{
  WayptRef r;
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
  } else if (aTypeName == "discontinuity") {
    r = new Discontinuity(aOwner);
  } else if (aTypeName == "via") {
      r = new Via(aOwner);
  }

  if (!r || (r->type() != aTypeName)) {
    throw sg_exception("broken factory method for type:" + aTypeName,
      "Waypt::createInstance");
  }

  return r;
}

WayptRef Waypt::createFromProperties(RouteBase* aOwner, SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("type")) {
      SG_LOG(SG_GENERAL, SG_WARN, "Bad waypoint node: missing type");
      return {};
  }

  flightgear::AirwayRef via;
  if (aProp->hasChild("airway")) {
      const auto level = static_cast<flightgear::Airway::Level>(aProp->getIntValue("network"));
      via = flightgear::Airway::findByIdent(aProp->getStringValue("airway"), level);
      if (via) {
          // override owner if we are from an airway
          aOwner = via.get();
      }
  }

    WayptRef nd(createInstance(aOwner, aProp->getStringValue("type")));
    if (nd->initFromProperties(aProp)) {
        return nd;
    }
    SG_LOG(SG_GENERAL, SG_WARN, "failed to create waypoint, trying basic");


    // if we failed to make the waypoint, try again making a basic waypoint.
    // this handles the case where a navaid waypoint is missing, for example
    // we also reject navaids that don't look correct (too far form the specified
    // lat-lon, eg see https://sourceforge.net/p/flightgear/codetickets/1814/ )
    // and again fallback to here.
    WayptRef bw(new BasicWaypt(aOwner));
    if (bw->initFromProperties(aProp)) {
        return bw;
    }

    return {}; // total failure
}

void Waypt::saveAsNode(SGPropertyNode* n) const
{
  n->setStringValue("type", type());
  writeToProperties(n);
}

bool Waypt::initFromProperties(SGPropertyNode_ptr aProp)
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

  if (aProp->hasChild("approach")) {
    setFlag(WPT_APPROACH, aProp->getBoolValue("approach"));
  }

  if (aProp->hasChild("departure")) {
    setFlag(WPT_DEPARTURE, aProp->getBoolValue("departure"));
  }

  if (aProp->hasChild("miss")) {
    setFlag(WPT_MISS, aProp->getBoolValue("miss"));
  }

  if (aProp->hasChild("airway")) {
      setFlag(WPT_VIA, true);
  }

  if (aProp->hasChild("alt-restrict")) {
    _altRestrict = restrictionFromString(aProp->getStringValue("alt-restrict"));
    _altitudeFt = aProp->getDoubleValue("altitude-ft");
  }

  if (aProp->hasChild("speed-restrict")) {
    _speedRestrict = restrictionFromString(aProp->getStringValue("speed-restrict"));
    _speed = aProp->getDoubleValue("speed");
  }

  return true;
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

  if (flag(WPT_APPROACH)) {
    aProp->setBoolValue("approach", true);
  }

  if (flag(WPT_VIA) && _owner) {
      flightgear::AirwayRef awy = (flightgear::Airway*) _owner;
      aProp->setStringValue("airway", awy->ident());
      aProp->setIntValue("network", awy->level());
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

RouteBase::~RouteBase()
{
}
    
void RouteBase::dumpRouteToKML(const WayptVec& aRoute, const std::string& aName)
{
  SGPath p = "/Users/jmt/Desktop/" + aName + ".kml";
  sg_ofstream f(p);
  if (!f.is_open()) {
    SG_LOG(SG_NAVAID, SG_WARN, "unable to open:" << p);
    return;
  }

// pre-amble
  f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
    "<Document>\n";

  dumpRouteToKMLLineString(aName, aRoute, f);

// post-amble
  f << "</Document>\n"
    "</kml>" << endl;
  f.close();
}

void RouteBase::dumpRouteToKMLLineString(const std::string& aIdent,
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

void RouteBase::loadAirportProcedures(const SGPath& aPath, FGAirport* aApt)
{
  assert(aApt);
  try {
    NavdataVisitor visitor(aApt, aPath);
      readXML(aPath, visitor);
  } catch (sg_io_exception& ex) {
    SG_LOG(SG_NAVAID, SG_WARN, "failure parsing procedures: " << aPath <<
      "\n\t" << ex.getMessage() << "\n\tat:" << ex.getLocation().asString());
  } catch (sg_exception& ex) {
    SG_LOG(SG_NAVAID, SG_WARN, "failure parsing procedures: " << aPath <<
      "\n\t" << ex.getMessage());
  }
}

} // of namespace flightgear
