// waypoint.cxx - waypoints that can occur in routes/procedures
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

#include "waypoint.hxx"

#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>

#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Navaids/airways.hxx>

using std::string;

namespace flightgear
{

BasicWaypt::BasicWaypt(const SGGeod& aPos, const string& aIdent, RouteBase* aOwner) :
  Waypt(aOwner),
  _pos(aPos),
  _ident(aIdent)
{
}

BasicWaypt::BasicWaypt(RouteBase* aOwner) :
  Waypt(aOwner)
{
}

void BasicWaypt::initFromProperties(SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("lon") || !aProp->hasChild("lat")) {
    throw sg_io_exception("missing lon/lat properties", 
      "BasicWaypt::initFromProperties");
  }

  Waypt::initFromProperties(aProp);

  _pos = SGGeod::fromDeg(aProp->getDoubleValue("lon"), 
    aProp->getDoubleValue("lat"));
  _ident = aProp->getStringValue("ident");
}

void BasicWaypt::writeToProperties(SGPropertyNode_ptr aProp) const
{
  Waypt::writeToProperties(aProp);
  
  aProp->setStringValue("ident", _ident);
  aProp->setDoubleValue("lon", _pos.getLongitudeDeg());
  aProp->setDoubleValue("lat", _pos.getLatitudeDeg());
}

std::string BasicWaypt::icaoDescription() const
{
    return simgear::strutils::formatGeodAsString(_pos, simgear::strutils::LatLonFormat::ICAO_ROUTE_DEGREES);
}
    
//////////////////////////////////////////////////////////////////////////////

NavaidWaypoint::NavaidWaypoint(FGPositioned* aPos, RouteBase* aOwner) :
  Waypt(aOwner),
  _navaid(aPos)
{
  if (aPos->type() == FGPositioned::RUNWAY) {
      SG_LOG(SG_NAVAID, SG_WARN, "sure you don't want to be building a runway waypt here?");
  }
}

NavaidWaypoint::NavaidWaypoint(RouteBase* aOwner) :
  Waypt(aOwner)
{
}


SGGeod NavaidWaypoint::position() const
{
  return SGGeod::fromGeodFt(_navaid->geod(), _altitudeFt);
}  
 
std::string NavaidWaypoint::ident() const
{
  return _navaid->ident();
}

void NavaidWaypoint::initFromProperties(SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("ident")) {
    throw sg_io_exception("missing navaid value", 
      "NavaidWaypoint::initFromProperties");
  }
  
  Waypt::initFromProperties(aProp);
  
  std::string idn(aProp->getStringValue("ident"));
  SGGeod p;
  if (aProp->hasChild("lon")) {
    p = SGGeod::fromDeg(aProp->getDoubleValue("lon"), aProp->getDoubleValue("lat"));
  }
  
  // FIXME - resolve co-located DME, etc
  // is it sufficent just to ignore DMEs, actually?
  FGPositionedRef nav = FGPositioned::findClosestWithIdent(idn, p, nullptr);
  if (!nav) {
    throw sg_io_exception("unknown navaid ident:" + idn, 
      "NavaidWaypoint::initFromProperties");
  }

  if (p.isValid() && (SGGeodesy::distanceM(nav->geod(), p) > 4000)) {
      // the looked up navaid was more than 4000 metres from the lat/lon.
      // in this case, throw an exception here so we fall back to using
      // a basic waypoint
      // see https://sourceforge.net/p/flightgear/codetickets/1814/
      throw sg_io_exception("Waypoint navaid for ident:" + idn +
                            " is too far from specified lat/lon in flight-plan",
                            "NavaidWaypoint::initFromProperties");
  }
  
  _navaid = nav;
}

void NavaidWaypoint::writeToProperties(SGPropertyNode_ptr aProp) const
{
  Waypt::writeToProperties(aProp);
  
  aProp->setStringValue("ident", _navaid->ident());
  // write lon/lat to disambiguate
  aProp->setDoubleValue("lon", _navaid->geod().getLongitudeDeg());
  aProp->setDoubleValue("lat", _navaid->geod().getLatitudeDeg());
}

OffsetNavaidWaypoint::OffsetNavaidWaypoint(FGPositioned* aPos, RouteBase* aOwner,
  double aRadial, double aDistNm) :
  NavaidWaypoint(aPos, aOwner),
  _radial(aRadial),
  _distanceNm(aDistNm)
{
  init();
}

OffsetNavaidWaypoint::OffsetNavaidWaypoint(RouteBase* aOwner) :
  NavaidWaypoint(aOwner)
{
}

void OffsetNavaidWaypoint::init()
{
  SGGeod offset;
  double az2;
  SGGeodesy::direct(_navaid->geod(), _radial, _distanceNm * SG_NM_TO_METER, offset, az2);
  _geod = SGGeod::fromGeodFt(offset, _altitudeFt);
}

void OffsetNavaidWaypoint::initFromProperties(SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("radial-deg") || !aProp->hasChild("distance-nm")) {
    throw sg_io_exception("missing radial/offset distance",
      "OffsetNavaidWaypoint::initFromProperties");
  }
  
  NavaidWaypoint::initFromProperties(aProp);
  _radial = aProp->getDoubleValue("radial-deg");
  _distanceNm = aProp->getDoubleValue("distance-nm");
  init();
}

void OffsetNavaidWaypoint::writeToProperties(SGPropertyNode_ptr aProp) const
{
  NavaidWaypoint::writeToProperties(aProp);
  aProp->setDoubleValue("radial-deg", _radial);
  aProp->setDoubleValue("distance-nm", _distanceNm);
}

/////////////////////////////////////////////////////////////////////////////

RunwayWaypt::RunwayWaypt(FGRunway* aPos, RouteBase* aOwner) :
  Waypt(aOwner),
  _runway(aPos)
{
}

RunwayWaypt::RunwayWaypt(RouteBase* aOwner) :
  Waypt(aOwner)
{
}

SGGeod RunwayWaypt::position() const
{
  return _runway->threshold();
}
  
std::string RunwayWaypt::ident() const
{
  return _runway->airport()->ident() + "-" + _runway->ident();
}

FGPositioned* RunwayWaypt::source() const
{
  return _runway;
}
  
double RunwayWaypt::headingRadialDeg() const
{
  return _runway->headingDeg();
}

void RunwayWaypt::initFromProperties(SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("icao") || !aProp->hasChild("ident")) {
    throw sg_io_exception("missing values: icao or ident", 
      "RunwayWaypoint::initFromProperties");
  }
  
  Waypt::initFromProperties(aProp);
  std::string idn(aProp->getStringValue("ident"));
  const FGAirport* apt = FGAirport::getByIdent(aProp->getStringValue("icao"));
  _runway = apt->getRunwayByIdent(aProp->getStringValue("ident"));
}

void RunwayWaypt::writeToProperties(SGPropertyNode_ptr aProp) const
{
  Waypt::writeToProperties(aProp);
  aProp->setStringValue("ident", _runway->ident());
  aProp->setStringValue("icao", _runway->airport()->ident());
}

/////////////////////////////////////////////////////////////////////////////

Hold::Hold(const SGGeod& aPos, const string& aIdent, RouteBase* aOwner) :
  BasicWaypt(aPos, aIdent, aOwner),
  _righthanded(true),
  _isDistance(false)
{
}

Hold::Hold(RouteBase* aOwner) :
  BasicWaypt(aOwner),
  _righthanded(true),
  _isDistance(false)
{
}

void Hold::setHoldRadial(double aInboundRadial)
{
  _bearing = aInboundRadial;
}

void Hold::setHoldDistance(double aDistanceNm)
{
  _isDistance = true;
  _holdTD = aDistanceNm;
}

void Hold::setHoldTime(double aTimeSec)
{
  _isDistance = false;
  _holdTD = aTimeSec;
}

void Hold::setRightHanded()
{
  _righthanded = true;
}

void Hold::setLeftHanded()
{
  _righthanded = false;
}
  
void Hold::initFromProperties(SGPropertyNode_ptr aProp)
{
  BasicWaypt::initFromProperties(aProp);
  
  if (!aProp->hasChild("lon") || !aProp->hasChild("lat")) {
    throw sg_io_exception("missing lon/lat properties", 
      "Hold::initFromProperties");
  }

  _righthanded = aProp->getBoolValue("right-handed");
  _isDistance = aProp->getBoolValue("is-distance");
  _bearing = aProp->getDoubleValue("inbound-radial-deg");
  _holdTD = aProp->getDoubleValue("td");
}

void Hold::writeToProperties(SGPropertyNode_ptr aProp) const
{
  BasicWaypt::writeToProperties(aProp);

  aProp->setBoolValue("right-handed", _righthanded);
  aProp->setBoolValue("is-distance", _isDistance);
  aProp->setDoubleValue("inbound-radial-deg", _bearing);
  aProp->setDoubleValue("td", _holdTD);
}

/////////////////////////////////////////////////////////////////////////////

HeadingToAltitude::HeadingToAltitude(RouteBase* aOwner, const string& aIdent, 
  double aMagHdg) :
  Waypt(aOwner),
  _ident(aIdent),
  _magHeading(aMagHdg)
{
  setFlag(WPT_DYNAMIC);
}

HeadingToAltitude::HeadingToAltitude(RouteBase* aOwner) :
  Waypt(aOwner)
{
}

void HeadingToAltitude::initFromProperties(SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("heading-deg")) {
    throw sg_io_exception("missing heading/alt properties", 
      "HeadingToAltitude::initFromProperties");
  }

  Waypt::initFromProperties(aProp);
  _magHeading = aProp->getDoubleValue("heading-deg");
  _ident = aProp->getStringValue("ident");
}

void HeadingToAltitude::writeToProperties(SGPropertyNode_ptr aProp) const
{
  Waypt::writeToProperties(aProp);
  aProp->setStringValue("ident", _ident);
  aProp->setDoubleValue("heading-deg", _magHeading);
}

/////////////////////////////////////////////////////////////////////////////

DMEIntercept::DMEIntercept(RouteBase* aOwner, const string& aIdent, const SGGeod& aPos,
    double aCourseDeg, double aDistanceNm) :
  Waypt(aOwner),
  _ident(aIdent),
  _pos(aPos),
  _magCourse(aCourseDeg),
  _dmeDistanceNm(aDistanceNm)
{
  setFlag(WPT_DYNAMIC);
}

DMEIntercept::DMEIntercept(RouteBase* aOwner) :
  Waypt(aOwner)
{
}

void DMEIntercept::initFromProperties(SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("lon") || !aProp->hasChild("lat")) {
    throw sg_io_exception("missing lon/lat properties", 
      "DMEIntercept::initFromProperties");
  }

  Waypt::initFromProperties(aProp);
  _pos = SGGeod::fromDeg(aProp->getDoubleValue("lon"), aProp->getDoubleValue("lat"));
  _ident = aProp->getStringValue("ident");
// check it's a real DME?
  _magCourse = aProp->getDoubleValue("course-deg");
  _dmeDistanceNm = aProp->getDoubleValue("dme-distance-nm");
  
}

void DMEIntercept::writeToProperties(SGPropertyNode_ptr aProp) const
{
  Waypt::writeToProperties(aProp);
  
  aProp->setStringValue("ident", _ident);
  aProp->setDoubleValue("lon", _pos.getLongitudeDeg());
  aProp->setDoubleValue("lat", _pos.getLatitudeDeg());
  aProp->setDoubleValue("course-deg", _magCourse);
  aProp->setDoubleValue("dme-distance-nm", _dmeDistanceNm);
}

/////////////////////////////////////////////////////////////////////////////

RadialIntercept::RadialIntercept(RouteBase* aOwner, const string& aIdent, const SGGeod& aPos,
    double aCourseDeg, double aRadial) :
  Waypt(aOwner),
  _ident(aIdent),
  _pos(aPos),
  _magCourse(aCourseDeg),
  _radial(aRadial)
{
  setFlag(WPT_DYNAMIC);
}

RadialIntercept::RadialIntercept(RouteBase* aOwner) :
  Waypt(aOwner)
{
}
  
void RadialIntercept::initFromProperties(SGPropertyNode_ptr aProp)
{
  if (!aProp->hasChild("lon") || !aProp->hasChild("lat")) {
    throw sg_io_exception("missing lon/lat properties", 
      "RadialIntercept::initFromProperties");
  }

  Waypt::initFromProperties(aProp);
  _pos = SGGeod::fromDeg(aProp->getDoubleValue("lon"), aProp->getDoubleValue("lat"));
  _ident = aProp->getStringValue("ident");
// check it's a real VOR?
  _magCourse = aProp->getDoubleValue("course-deg");
  _radial = aProp->getDoubleValue("radial-deg");
  
}

void RadialIntercept::writeToProperties(SGPropertyNode_ptr aProp) const
{
  Waypt::writeToProperties(aProp);
  
  aProp->setStringValue("ident", _ident);
  aProp->setDoubleValue("lon", _pos.getLongitudeDeg());
  aProp->setDoubleValue("lat", _pos.getLatitudeDeg());
  aProp->setDoubleValue("course-deg", _magCourse);
  aProp->setDoubleValue("radial-deg", _radial);
}

/////////////////////////////////////////////////////////////////////////////

ATCVectors::ATCVectors(RouteBase* aOwner, FGAirport* aFacility) :
  Waypt(aOwner),
  _facility(aFacility)
{
  setFlag(WPT_DYNAMIC);
}

ATCVectors::~ATCVectors()
{
}

ATCVectors::ATCVectors(RouteBase* aOwner) :
  Waypt(aOwner)
{
}

SGGeod ATCVectors::position() const
{
  return _facility->geod();
}
    
string ATCVectors::ident() const
{
  return "VECTORS-" + _facility->ident();
}

void ATCVectors::initFromProperties(SGPropertyNode_ptr aProp)
{  
  if (!aProp->hasChild("icao")) {
    throw sg_io_exception("missing icao propertie", 
      "ATCVectors::initFromProperties");
  }

  Waypt::initFromProperties(aProp);
  _facility = FGAirport::getByIdent(aProp->getStringValue("icao"));
}

void ATCVectors::writeToProperties(SGPropertyNode_ptr aProp) const
{
  Waypt::writeToProperties(aProp);
  aProp->setStringValue("icao", _facility->ident());
}

/////////////////////////////////////////////////////////////////////////////

Discontinuity::Discontinuity(RouteBase* aOwner) :
    Waypt(aOwner)
{
    setFlag(WPT_DYNAMIC);
    setFlag(WPT_GENERATED); // prevent drag, delete, etc
}

Discontinuity::~Discontinuity()
{
}

SGGeod Discontinuity::position() const
{
    return SGGeod(); // deliberately invalid of course
}

string Discontinuity::ident() const
{
    return "DISCONTINUITY";
}

void Discontinuity::initFromProperties(SGPropertyNode_ptr aProp)
{
}

void Discontinuity::writeToProperties(SGPropertyNode_ptr aProp) const
{
    Waypt::writeToProperties(aProp);
}

/////////////////////////////////////////////////////////////////////////////

SGGeod Via::position() const
{
    return _to->geod();
}

string Via::ident() const
{
    return "VIA " + _airway->ident() + " TO " + _to->ident();
}

Via::Via(RouteBase *aOwner) :
    Waypt(aOwner)
{
}

Via::Via(RouteBase *aOwner, AirwayRef awy, FGPositionedRef to) :
    Waypt(aOwner),
    _airway(awy),
    _to(to)
{
}

Via::~Via()
{
}

void Via::initFromProperties(SGPropertyNode_ptr aProp)
{
    if (!aProp->hasChild("airway") || !aProp->hasChild("to")) {
        throw sg_io_exception("missing airway/to properties",
                              "Via::initFromProperties");
    }

    Waypt::initFromProperties(aProp);

    const std::string ident = aProp->getStringValue("airway");
    const Airway::Level level = static_cast<Airway::Level>(aProp->getIntValue("level", Airway::Both));
    _airway = Airway::findByIdent(ident, level);
    if (!_airway) {
        throw sg_io_exception("unknown airway ident: '" + ident + "'",
                              "Via::initFromProperties");
    }

    std::string idn(aProp->getStringValue("to"));
    SGGeod p;
    if (aProp->hasChild("lon")) {
        p = SGGeod::fromDeg(aProp->getDoubleValue("lon"),
                            aProp->getDoubleValue("lat"));
    }

    FGPositionedRef nav = FGPositioned::findClosestWithIdent(idn, p, nullptr);
    if (!nav) {
        throw sg_io_exception("unknown navaid ident:" + idn,
                              "Via::initFromProperties");
    }
    
    if (!_airway->containsNavaid(nav)) {
        // warn but don't block this
        SG_LOG(SG_AUTOPILOT, SG_WARN, "VIA TO navaid: " << idn << " not found on airway " << _airway);
    }

    _to = nav;
}

void Via::writeToProperties(SGPropertyNode_ptr aProp) const
{
    Waypt::writeToProperties(aProp);
    aProp->setStringValue("airway", _airway->ident());
    aProp->setIntValue("level", _airway->level());
    
    aProp->setStringValue("to", _to->ident());
    // write lon/lat to disambiguate
    aProp->setDoubleValue("lon", _to->geod().getLongitudeDeg());
    aProp->setDoubleValue("lat", _to->geod().getLatitudeDeg());
}

WayptVec Via::expandToWaypoints(WayptRef aPreceeding) const
{
    if (!aPreceeding) {
        throw sg_exception("invalid preceeding waypoint");
    }

    WayptRef toWp = new NavaidWaypoint(_to, nullptr);
    return _airway->via(aPreceeding, toWp);
}

} // of namespace
