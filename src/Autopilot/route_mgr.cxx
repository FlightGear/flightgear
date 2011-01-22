// route_mgr.cxx - manage a route (i.e. a collection of waypoints)
//
// Written by Curtis Olson, started January 2004.
//            Norman Vine
//            Melchior FRANZ
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <time.h>
#endif

#include <simgear/compiler.h>

#include "route_mgr.hxx"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/tuple/tuple.hpp>

#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sgstream.hxx>

#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/route/route.hxx>
#include <simgear/sg_inlines.h>

#include "Main/fg_props.hxx"
#include "Navaids/positioned.hxx"
#include <Navaids/waypoint.hxx>
#include <Navaids/airways.hxx>
#include <Navaids/procedure.hxx>
#include "Airports/simple.hxx"
#include "Airports/runways.hxx"

#define RM "/autopilot/route-manager/"

#include <GUI/new_gui.hxx>
#include <GUI/dialog.hxx>

using namespace flightgear;

class PropertyWatcher : public SGPropertyChangeListener
{
public:
  void watch(SGPropertyNode* p)
  {
    p->addChangeListener(this, false);
  }

  virtual void valueChanged(SGPropertyNode*)
  {
    fire();
  }
protected:
  virtual void fire() = 0;
};

/**
 * Template adapter, created by convenience helper below
 */
template <class T>
class MethodPropertyWatcher : public PropertyWatcher
{
public:
  typedef void (T::*fire_method)();

  MethodPropertyWatcher(T* obj, fire_method m) :
    _object(obj),
    _method(m)
  { ; }
  
protected:
  virtual void fire()
  { // dispatch to the object method we're helping
    (_object->*_method)();
  }
  
private:
  T* _object;
  fire_method _method;
};

template <class T>
PropertyWatcher* createWatcher(T* obj, void (T::*m)())
{
  return new MethodPropertyWatcher<T>(obj, m);
}

FGRouteMgr::FGRouteMgr() :
  _currentIndex(0),
  input(fgGetNode( RM "input", true )),
  mirror(fgGetNode( RM "route", true ))
{
  listener = new InputListener(this);
  input->setStringValue("");
  input->addChangeListener(listener);
}


FGRouteMgr::~FGRouteMgr()
{
  input->removeChangeListener(listener);
  delete listener;
}


void FGRouteMgr::init() {
  SGPropertyNode_ptr rm(fgGetNode(RM));
  
  lon = fgGetNode( "/position/longitude-deg", true );
  lat = fgGetNode( "/position/latitude-deg", true );
  alt = fgGetNode( "/position/altitude-ft", true );
  magvar = fgGetNode("/environment/magnetic-variation-deg", true);
     
  departure = fgGetNode(RM "departure", true);
  departure->tie("airport", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDepartureICAO, &FGRouteMgr::setDepartureICAO));
  departure->tie("name", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDepartureName, NULL));
  departure->setStringValue("runway", "");
  
  _departureWatcher = createWatcher(this, &FGRouteMgr::departureChanged);
  _departureWatcher->watch(departure->getChild("runway"));
  
  departure->getChild("etd", 0, true);
  _departureWatcher->watch(departure->getChild("sid", 0, true));
  departure->getChild("takeoff-time", 0, true);

  destination = fgGetNode(RM "destination", true);
  destination->getChild("airport", 0, true);
  
  destination->tie("airport", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDestinationICAO, &FGRouteMgr::setDestinationICAO));
  destination->tie("name", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDestinationName, NULL));
  
  _arrivalWatcher = createWatcher(this, &FGRouteMgr::arrivalChanged);
  _arrivalWatcher->watch(destination->getChild("runway", 0, true));
  
  destination->getChild("eta", 0, true);
  _arrivalWatcher->watch(destination->getChild("star", 0, true));
  _arrivalWatcher->watch(destination->getChild("transition", 0, true));
  destination->getChild("touchdown-time", 0, true);

  alternate = fgGetNode(RM "alternate", true);
  alternate->getChild("airport", 0, true);
  alternate->getChild("runway", 0, true);
  
  cruise = fgGetNode(RM "cruise", true);
  cruise->getChild("altitude-ft", 0, true);
  cruise->setDoubleValue("altitude-ft", 10000.0);
  cruise->getChild("flight-level", 0, true);
  cruise->getChild("speed-kts", 0, true);
  cruise->setDoubleValue("speed-kts", 160.0);
  
  _routingType = cruise->getChild("routing", 0, true);
  _routingType->setIntValue(ROUTE_HIGH_AIRWAYS);
  
  totalDistance = fgGetNode(RM "total-distance", true);
  totalDistance->setDoubleValue(0.0);
  
  ete = fgGetNode(RM "ete", true);
  ete->setDoubleValue(0.0);
  
  elapsedFlightTime = fgGetNode(RM "flight-time", true);
  elapsedFlightTime->setDoubleValue(0.0);
  
  active = fgGetNode(RM "active", true);
  active->setBoolValue(false);
  
  airborne = fgGetNode(RM "airborne", true);
  airborne->setBoolValue(false);
    
  _edited = fgGetNode(RM "signals/edited", true);
  _finished = fgGetNode(RM "signals/finished", true);
  
  _currentWpt = fgGetNode(RM "current-wp", true);
  _currentWpt->tie(SGRawValueMethods<FGRouteMgr, int>
    (*this, &FGRouteMgr::currentIndex, &FGRouteMgr::jumpToIndex));
      
  // temporary distance / eta calculations, for backward-compatability
  wp0 = fgGetNode(RM "wp", 0, true);
  wp0->getChild("id", 0, true);
  wp0->getChild("dist", 0, true);
  wp0->getChild("eta", 0, true);
  wp0->getChild("bearing-deg", 0, true);
  
  wp1 = fgGetNode(RM "wp", 1, true);
  wp1->getChild("id", 0, true);
  wp1->getChild("dist", 0, true);
  wp1->getChild("eta", 0, true);
  
  wpn = fgGetNode(RM "wp-last", 0, true);
  wpn->getChild("dist", 0, true);
  wpn->getChild("eta", 0, true);
  
  update_mirror();
  _pathNode = fgGetNode(RM "file-path", 0, true);
}


void FGRouteMgr::postinit()
{
  string_list *waypoints = globals->get_initial_waypoints();
  if (waypoints) {
    string_list::iterator it;
    for (it = waypoints->begin(); it != waypoints->end(); ++it) {
      WayptRef w = waypointFromString(*it);
      if (w) {
        _route.push_back(w);
      }
    }
    
    SG_LOG(SG_AUTOPILOT, SG_INFO, "loaded initial waypoints:" << _route.size());
  }

  weightOnWheels = fgGetNode("/gear/gear[0]/wow", true);
  // check airbone flag agrees with presets
}

void FGRouteMgr::bind() { }
void FGRouteMgr::unbind() { }

bool FGRouteMgr::isRouteActive() const
{
  return active->getBoolValue();
}

void FGRouteMgr::update( double dt )
{
  if (dt <= 0.0) {
    return; // paused, nothing to do here
  }
  
  double groundSpeed = fgGetDouble("/velocities/groundspeed-kt", 0.0);
  if (airborne->getBoolValue()) {
    time_t now = time(NULL);
    elapsedFlightTime->setDoubleValue(difftime(now, _takeoffTime));
  } else { // not airborne
    if (weightOnWheels->getBoolValue() || (groundSpeed < 40)) {
      return;
    }
    
    airborne->setBoolValue(true);
    _takeoffTime = time(NULL); // start the clock
    departure->setIntValue("takeoff-time", _takeoffTime);
  }
  
  if (!active->getBoolValue()) {
    return;
  }

// basic course/distance information
  SGGeod currentPos = SGGeod::fromDegFt(lon->getDoubleValue(), 
    lat->getDoubleValue(),alt->getDoubleValue());

  Waypt* curWpt = currentWaypt();
  if (!curWpt) {
    return;
  }
  
  double courseDeg;
  double distanceM;
  boost::tie(courseDeg, distanceM) = curWpt->courseAndDistanceFrom(currentPos);
  
// update wp0 / wp1 / wp-last for legacy users
  wp0->setDoubleValue("dist", distanceM * SG_METER_TO_NM);
  courseDeg -= magvar->getDoubleValue(); // expose magnetic bearing
  wp0->setDoubleValue("bearing-deg", courseDeg);
  setETAPropertyFromDistance(wp0->getChild("eta"), distanceM);
  
  double totalDistanceRemaining = distanceM; // distance to current waypoint
  
  Waypt* nextWpt = nextWaypt();
  if (nextWpt) {
    boost::tie(courseDeg, distanceM) = nextWpt->courseAndDistanceFrom(currentPos);
     
    wp1->setDoubleValue("dist", distanceM * SG_METER_TO_NM);
    courseDeg -= magvar->getDoubleValue(); // expose magnetic bearing
    wp1->setDoubleValue("bearing-deg", courseDeg);
    setETAPropertyFromDistance(wp1->getChild("eta"), distanceM);
  }
  
  Waypt* prev = curWpt;
  for (unsigned int i=_currentIndex + 1; i<_route.size(); ++i) {
    Waypt* w = _route[i];
    if (w->flag(WPT_DYNAMIC)) continue;
    totalDistanceRemaining += SGGeodesy::distanceM(prev->position(), w->position());
    prev = w;
  }
  
  wpn->setDoubleValue("dist", totalDistanceRemaining * SG_METER_TO_NM);
  ete->setDoubleValue(totalDistanceRemaining * SG_METER_TO_NM / groundSpeed * 3600.0);
  setETAPropertyFromDistance(wpn->getChild("eta"), totalDistanceRemaining);
}

void FGRouteMgr::setETAPropertyFromDistance(SGPropertyNode_ptr aProp, double aDistance)
{
  double speed = fgGetDouble("/velocities/groundspeed-kt", 0.0);
  if (speed < 1.0) {
    aProp->setStringValue("--:--");
    return;
  }

  char eta_str[64];
  double eta = aDistance * SG_METER_TO_NM / speed;
  if ( eta >= 100.0 ) { 
      eta = 99.999; // clamp
  }
  
  if ( eta < (1.0/6.0) ) {
    eta *= 60.0; // within 10 minutes, bump up to min/secs
  }
  
  int major = (int)eta, 
      minor = (int)((eta - (int)eta) * 60.0);
  snprintf( eta_str, 64, "%d:%02d", major, minor );
  aProp->setStringValue( eta_str );
}

flightgear::WayptRef FGRouteMgr::removeWayptAtIndex(int aIndex)
{
  int index = aIndex;
  if (aIndex < 0) { // negative indices count the the end
    index = _route.size() + index;
  }
  
  if ((index < 0) || (index >= numWaypts())) {
    SG_LOG(SG_AUTOPILOT, SG_WARN, "removeWayptAtIndex with invalid index:" << aIndex);
    return NULL;
  }
  WayptVec::iterator it = _route.begin();
  it += index;
  
  WayptRef w = *it; // hold a ref now, in case _route is the only other owner
  _route.erase(it);
  
  update_mirror();
  
  if (_currentIndex == index) {
    currentWaypointChanged(); // current waypoint was removed
  }
  else
  if (_currentIndex > index) {
    --_currentIndex; // shift current index down if necessary
  }

  _edited->fireValueChanged();
  checkFinished();
  
  return w;
}
  
void FGRouteMgr::clearRoute()
{
  _route.clear();
  _currentIndex = -1;
  
  update_mirror();
  active->setBoolValue(false);
  _edited->fireValueChanged();
}

/**
 * route between index-1 and index, using airways.
 */
bool FGRouteMgr::routeToIndex(int index, RouteType aRouteType)
{
  WayptRef wp1;
  WayptRef wp2;
  
  if (index == -1) {
    index = _route.size(); // can still be zero, of course
  }
  
  if (index == 0) {
    if (!_departure) {
      SG_LOG(SG_AUTOPILOT, SG_WARN, "routeToIndex: no departure set");
      return false;
    }
    
    wp1 = new NavaidWaypoint(_departure.get(), NULL);
  } else {
    wp1 = wayptAtIndex(index - 1);
  }
  
  if (index >= numWaypts()) {
    if (!_destination) {
      SG_LOG(SG_AUTOPILOT, SG_WARN, "routeToIndex: no destination set");
      return false;
    }
    
    wp2 = new NavaidWaypoint(_destination.get(), NULL);
  } else {
    wp2 = wayptAtIndex(index);
  }
  
  double distNm = SGGeodesy::distanceNm(wp1->position(), wp2->position());
  if (distNm < 100.0) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "routeToIndex: existing waypoints are nearby, direct route");
    return true;
  }
  
  WayptVec r;
  switch (aRouteType) {
  case ROUTE_HIGH_AIRWAYS:
    Airway::highLevel()->route(wp1, wp2, r);
    break;
    
  case ROUTE_LOW_AIRWAYS:
    Airway::lowLevel()->route(wp1, wp2, r);
    break;
    
  case ROUTE_VOR:
    throw sg_exception("VOR routing not supported yet");
  }
  
  if (r.empty()) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "routeToIndex: no route found");
    return false;
  }

  WayptVec::iterator it = _route.begin();
  it += index;
  _route.insert(it, r.begin(), r.end());

  update_mirror();
  _edited->fireValueChanged();
  return true;
}

void FGRouteMgr::autoRoute()
{
  if (!_departure || !_destination) {
    return;
  }
  
  string runwayId(departure->getStringValue("runway"));
  FGRunway* runway = NULL;
  if (_departure->hasRunwayWithIdent(runwayId)) {
    runway = _departure->getRunwayByIdent(runwayId);
  }
  
  FGRunway* dstRunway = NULL;
  runwayId = destination->getStringValue("runway");
  if (_destination->hasRunwayWithIdent(runwayId)) {
    dstRunway = _destination->getRunwayByIdent(runwayId);
  }
    
  _route.clear(); // clear out the existing, first
// SID
  flightgear::SID* sid;
  WayptRef sidTrans;
  
  boost::tie(sid, sidTrans) = _departure->selectSID(_destination->geod(), runway);
  if (sid) { 
    SG_LOG(SG_AUTOPILOT, SG_INFO, "selected SID " << sid->ident());
    if (sidTrans) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "\tvia " << sidTrans->ident() << " transition");
    }
    
    sid->route(runway, sidTrans, _route);
    departure->setStringValue("sid", sid->ident());
  } else {
    // use airport location for airway search
    sidTrans = new NavaidWaypoint(_departure.get(), NULL);
    departure->setStringValue("sid", "");
  }
  
// STAR
  destination->setStringValue("transition", "");
  destination->setStringValue("star", "");
  
  STAR* star;
  WayptRef starTrans;
  boost::tie(star, starTrans) = _destination->selectSTAR(_departure->geod(), dstRunway);
  if (star) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "selected STAR " << star->ident());
    if (starTrans) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "\tvia " << starTrans->ident() << " transition");
      destination->setStringValue("transition", starTrans->ident());
    }    
    destination->setStringValue("star", star->ident());
  } else {
    // use airport location for search
    starTrans = new NavaidWaypoint(_destination.get(), NULL);
  }
  
// route between them
  WayptVec airwayRoute;
  if (Airway::highLevel()->route(sidTrans, starTrans, airwayRoute)) {
    _route.insert(_route.end(), airwayRoute.begin(), airwayRoute.end());
  }
  
// add the STAR if we have one
  if (star) {
    _destination->buildApproach(starTrans, star, dstRunway, _route);
  }

  update_mirror();
  _edited->fireValueChanged();
}

void FGRouteMgr::departureChanged()
{
// remove existing departure waypoints
  WayptVec::iterator it = _route.begin();
  for (; it != _route.end(); ++it) {
    if (!(*it)->flag(WPT_DEPARTURE)) {
      break;
    }
  }
  
  // erase() invalidates iterators, so grab now
  WayptRef enroute;
  if (it == _route.end()) {
    if (_destination) {
      enroute = new NavaidWaypoint(_destination.get(), NULL);
    }
  } else {
    enroute = *it;
  }

  _route.erase(_route.begin(), it);
  if (!_departure) {
    waypointsChanged();
    return;
  }
  
  WayptVec wps;
  buildDeparture(enroute, wps);
  for (it = wps.begin(); it != wps.end(); ++it) {
    (*it)->setFlag(WPT_DEPARTURE);
    (*it)->setFlag(WPT_GENERATED);
  }
  _route.insert(_route.begin(), wps.begin(), wps.end());
  
  update_mirror();
  waypointsChanged();
}

void FGRouteMgr::buildDeparture(WayptRef enroute, WayptVec& wps)
{
  string runwayId(departure->getStringValue("runway"));
  if (!_departure->hasRunwayWithIdent(runwayId)) {
// valid airport, but no runway selected, so just the airport noide itself
    wps.push_back(new NavaidWaypoint(_departure.get(), NULL));
    return;
  }
  
  FGRunway* r = _departure->getRunwayByIdent(runwayId);
  string sidId = departure->getStringValue("sid");
  flightgear::SID* sid = _departure->findSIDWithIdent(sidId);
  if (!sid) {
// valid runway, but no SID selected/found, so just the runway node for now
    if (!sidId.empty() && (sidId != "(none)")) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "SID not found:" << sidId);
    }
    
    wps.push_back(new RunwayWaypt(r, NULL));
    return;
  }
  
// we have a valid SID, awesome
  string trans(departure->getStringValue("transition"));
  WayptRef t = sid->findTransitionByName(trans);
  if (!t && enroute) {
    t = sid->findBestTransition(enroute->position());
  }

  sid->route(r, t, wps);
  if (!wps.empty() && wps.front()->flag(WPT_DYNAMIC)) {
    // ensure first waypoint is static, to simplify other computations
    wps.insert(wps.begin(), new RunwayWaypt(r, NULL));
  }
}

void FGRouteMgr::arrivalChanged()
{  
  // remove existing arrival waypoints
  WayptVec::reverse_iterator rit = _route.rbegin();
  for (; rit != _route.rend(); ++rit) {
    if (!(*rit)->flag(WPT_ARRIVAL)) {
      break;
    }
  }
  
  // erase() invalidates iterators, so grab now
  WayptRef enroute;
  WayptVec::iterator it;
  
  if (rit != _route.rend()) {
    enroute = *rit;
    it = rit.base(); // convert to fwd iterator
  } else {
    it = _route.begin();
  }

  _route.erase(it, _route.end());
  
  WayptVec wps;
  buildArrival(enroute, wps);
  for (it = wps.begin(); it != wps.end(); ++it) {
    (*it)->setFlag(WPT_ARRIVAL);
    (*it)->setFlag(WPT_GENERATED);
  }
  _route.insert(_route.end(), wps.begin(), wps.end());
  
  update_mirror();
  waypointsChanged();
}

void FGRouteMgr::buildArrival(WayptRef enroute, WayptVec& wps)
{
  if (!_destination) {
    return;
  }
  
  string runwayId(destination->getStringValue("runway"));
  if (!_destination->hasRunwayWithIdent(runwayId)) {
// valid airport, but no runway selected, so just the airport node itself
    wps.push_back(new NavaidWaypoint(_destination.get(), NULL));
    return;
  }
  
  FGRunway* r = _destination->getRunwayByIdent(runwayId);
  string starId = destination->getStringValue("star");
  STAR* star = _destination->findSTARWithIdent(starId);
  if (!star) {
// valid runway, but no STAR selected/found, so just the runway node for now
    wps.push_back(new RunwayWaypt(r, NULL));
    return;
  }
  
// we have a valid STAR
  string trans(destination->getStringValue("transition"));
  WayptRef t = star->findTransitionByName(trans);
  if (!t && enroute) {
    t = star->findBestTransition(enroute->position());
  }
  
  _destination->buildApproach(t, star, r, wps);
}

void FGRouteMgr::waypointsChanged()
{

}

void FGRouteMgr::insertWayptAtIndex(Waypt* aWpt, int aIndex)
{
  if (!aWpt) {
    return;
  }
  
  int index = aIndex;
  if ((aIndex == -1) || (aIndex > (int) _route.size())) {
    index = _route.size();
  }
  
  WayptVec::iterator it = _route.begin();
  it += index;
      
  if (_currentIndex >= index) {
    ++_currentIndex;
  }
  
  _route.insert(it, aWpt);
  
  update_mirror();
  _edited->fireValueChanged();
}

WayptRef FGRouteMgr::waypointFromString(const string& tgt )
{
  string target(boost::to_upper_copy(tgt));
  WayptRef wpt;
  
// extract altitude
  double altFt = cruise->getDoubleValue("altitude-ft");
  RouteRestriction altSetting = RESTRICT_NONE;
    
  size_t pos = target.find( '@' );
  if ( pos != string::npos ) {
    altFt = atof( target.c_str() + pos + 1 );
    target = target.substr( 0, pos );
    if ( !strcmp(fgGetString("/sim/startup/units"), "meter") )
      altFt *= SG_METER_TO_FEET;
    altSetting = RESTRICT_AT;
  }

// check for lon,lat
  pos = target.find( ',' );
  if ( pos != string::npos ) {
    double lon = atof( target.substr(0, pos).c_str());
    double lat = atof( target.c_str() + pos + 1);
    char buf[32];
    char ew = (lon < 0.0) ? 'W' : 'E';
    char ns = (lat < 0.0) ? 'S' : 'N';
    snprintf(buf, 32, "%c%03d%c%03d", ew, (int) fabs(lon), ns, (int)fabs(lat));
    
    wpt = new BasicWaypt(SGGeod::fromDeg(lon, lat), buf, NULL);
    if (altSetting != RESTRICT_NONE) {
      wpt->setAltitude(altFt, altSetting);
    }
    return wpt;
  }

  SGGeod basePosition;
  if (_route.empty()) {
    // route is empty, use current position
    basePosition = SGGeod::fromDeg(lon->getDoubleValue(), lat->getDoubleValue());
  } else {
    basePosition = _route.back()->position();
  }
    
  string_list pieces(simgear::strutils::split(target, "/"));
  FGPositionedRef p = FGPositioned::findClosestWithIdent(pieces.front(), basePosition);
  if (!p) {
    SG_LOG( SG_AUTOPILOT, SG_INFO, "Unable to find FGPositioned with ident:" << pieces.front());
    return NULL;
  }

  if (pieces.size() == 1) {
    wpt = new NavaidWaypoint(p, NULL);
  } else if (pieces.size() == 3) {
    // navaid/radial/distance-nm notation
    double radial = atof(pieces[1].c_str()),
      distanceNm = atof(pieces[2].c_str());
    radial += magvar->getDoubleValue(); // convert to true bearing
    wpt = new OffsetNavaidWaypoint(p, NULL, radial, distanceNm);
  } else if (pieces.size() == 2) {
    FGAirport* apt = dynamic_cast<FGAirport*>(p.ptr());
    if (!apt) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "Waypoint is not an airport:" << pieces.front());
      return NULL;
    }
    
    if (!apt->hasRunwayWithIdent(pieces[1])) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "No runway: " << pieces[1] << " at " << pieces[0]);
      return NULL;
    }
      
    FGRunway* runway = apt->getRunwayByIdent(pieces[1]);
    wpt = new NavaidWaypoint(runway, NULL);
  } else if (pieces.size() == 4) {
    // navid/radial/navid/radial notation     
    FGPositionedRef p2 = FGPositioned::findClosestWithIdent(pieces[2], basePosition);
    if (!p2) {
      SG_LOG( SG_AUTOPILOT, SG_INFO, "Unable to find FGPositioned with ident:" << pieces[2]);
      return NULL;
    }

    double r1 = atof(pieces[1].c_str()),
      r2 = atof(pieces[3].c_str());
    r1 += magvar->getDoubleValue();
    r2 += magvar->getDoubleValue();
    
    SGGeod intersection;
    bool ok = SGGeodesy::radialIntersection(p->geod(), r1, p2->geod(), r2, intersection);
    if (!ok) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "no valid intersection for:" << target);
      return NULL;
    }
    
    std::string name = p->ident() + "-" + p2->ident();
    wpt = new BasicWaypt(intersection, name, NULL);
  }
  
  if (!wpt) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "Unable to parse waypoint:" << target);
    return NULL;
  }
  
  if (altSetting != RESTRICT_NONE) {
    wpt->setAltitude(altFt, altSetting);
  }
  return wpt;
}

// mirror internal route to the property system for inspection by other subsystems
void FGRouteMgr::update_mirror()
{
  mirror->removeChildren("wp");
  for (int i = 0; i < numWaypts(); i++) {
    Waypt* wp = _route[i];
    SGPropertyNode *prop = mirror->getChild("wp", i, 1);

    const SGGeod& pos(wp->position());
    prop->setStringValue("id", wp->ident().c_str());
    //prop->setStringValue("name", wp.get_name().c_str());
    prop->setDoubleValue("longitude-deg", pos.getLongitudeDeg());
    prop->setDoubleValue("latitude-deg",pos.getLatitudeDeg());
   
    if (wp->altitudeRestriction() != RESTRICT_NONE) {
      double ft = wp->altitudeFt();
      prop->setDoubleValue("altitude-m", ft * SG_FEET_TO_METER);
      prop->setDoubleValue("altitude-ft", ft);
    } else {
      prop->setDoubleValue("altitude-m", -9999.9);
      prop->setDoubleValue("altitude-ft", -9999.9);
    }
    
    if (wp->speedRestriction() != RESTRICT_NONE) {
      prop->setDoubleValue("speed-kts", wp->speedKts());
    }
    
    if (wp->flag(WPT_ARRIVAL)) {
      prop->setBoolValue("arrival", true);
    }
    
    if (wp->flag(WPT_DEPARTURE)) {
      prop->setBoolValue("departure", true);
    }
    
    if (wp->flag(WPT_MISS)) {
      prop->setBoolValue("missed-approach", true);
    }
    
    prop->setBoolValue("generated", wp->flag(WPT_GENERATED));
  } // of waypoint iteration
  
  // set number as listener attachment point
  mirror->setIntValue("num", _route.size());
    
  NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
  FGDialog* rmDlg = gui->getDialog("route-manager");
  if (rmDlg) {
    rmDlg->updateValues();
  }
}

// command interface /autopilot/route-manager/input:
//
//   @CLEAR             ... clear route
//   @POP               ... remove first entry
//   @DELETE3           ... delete 4th entry
//   @INSERT2:KSFO@900  ... insert "KSFO@900" as 3rd entry
//   KSFO@900           ... append "KSFO@900"
//
void FGRouteMgr::InputListener::valueChanged(SGPropertyNode *prop)
{
    const char *s = prop->getStringValue();
    if (strlen(s) == 0) {
      return;
    }
    
    if (!strcmp(s, "@CLEAR"))
        mgr->clearRoute();
    else if (!strcmp(s, "@ACTIVATE"))
        mgr->activate();
    else if (!strcmp(s, "@LOAD")) {
      mgr->loadRoute();
    } else if (!strcmp(s, "@SAVE")) {
      mgr->saveRoute();
    } else if (!strcmp(s, "@POP")) {
      SG_LOG(SG_AUTOPILOT, SG_WARN, "route-manager @POP command is deprecated");
    } else if (!strcmp(s, "@NEXT")) {
      mgr->jumpToIndex(mgr->_currentIndex + 1);
    } else if (!strcmp(s, "@PREVIOUS")) {
      mgr->jumpToIndex(mgr->_currentIndex - 1);
    } else if (!strncmp(s, "@JUMP", 5)) {
      mgr->jumpToIndex(atoi(s + 5));
    } else if (!strncmp(s, "@DELETE", 7))
        mgr->removeWayptAtIndex(atoi(s + 7));
    else if (!strncmp(s, "@INSERT", 7)) {
        char *r;
        int pos = strtol(s + 7, &r, 10);
        if (*r++ != ':')
            return;
        while (isspace(*r))
            r++;
        if (*r)
            mgr->insertWayptAtIndex(mgr->waypointFromString(r), pos);
    } else if (!strncmp(s, "@ROUTE", 6)) {
      char* r;
      int endIndex = strtol(s + 6, &r, 10);
      RouteType rt = (RouteType) mgr->_routingType->getIntValue();
      mgr->routeToIndex(endIndex, rt);
    } else if (!strcmp(s, "@AUTOROUTE")) {
      mgr->autoRoute();
    } else if (!strcmp(s, "@POSINIT")) {
      mgr->initAtPosition();
    } else
      mgr->insertWayptAtIndex(mgr->waypointFromString(s), -1);
}

void FGRouteMgr::initAtPosition()
{
  if (isRouteActive()) {
    return; // don't mess with the active route
  }
  
  if (haveUserWaypoints()) {
    // user has already defined, loaded or entered a route, again
    // don't interfere with it
    return; 
  }
  
  if (airborne->getBoolValue()) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "initAtPosition: airborne, clearing departure info");
    _departure = NULL;
    departure->setStringValue("runway", "");
    return;
  }
  
// on the ground
  SGGeod pos = SGGeod::fromDegFt(lon->getDoubleValue(), lat->getDoubleValue(), alt->getDoubleValue());
  _departure = FGAirport::findClosest(pos, 20.0);
  if (!_departure) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "initAtPosition: couldn't find an airport within 20nm");
    departure->setStringValue("runway", "");
    return;
  }
  
  FGRunway* r = _departure->findBestRunwayForPos(pos);
  if (!r) {
    return;
  }
  
  departure->setStringValue("runway", r->ident().c_str());
  SG_LOG(SG_AUTOPILOT, SG_INFO, "initAtPosition: starting at " 
    << _departure->ident() << " on runway " << r->ident());
}

bool FGRouteMgr::haveUserWaypoints() const
{
  for (int i = 0; i < numWaypts(); i++) {
    if (!_route[i]->flag(WPT_GENERATED)) {
      // have a non-generated waypoint, we're done
      return true;
    }
  }
  
  // all waypoints are generated
  return false;
}

bool FGRouteMgr::activate()
{
  if (isRouteActive()) {
    SG_LOG(SG_AUTOPILOT, SG_WARN, "duplicate route-activation, no-op");
    return false;
  }
 
  _currentIndex = 0;
  currentWaypointChanged();
  
 /* double routeDistanceNm = _route->total_distance() * SG_METER_TO_NM;
  totalDistance->setDoubleValue(routeDistanceNm);
  double cruiseSpeedKts = cruise->getDoubleValue("speed", 0.0);
  if (cruiseSpeedKts > 1.0) {
    // very very crude approximation, doesn't allow for climb / descent
    // performance or anything else at all
    ete->setDoubleValue(routeDistanceNm / cruiseSpeedKts * (60.0 * 60.0));
  }
  */
  active->setBoolValue(true);
  SG_LOG(SG_AUTOPILOT, SG_INFO, "route-manager, activate route ok");
  return true;
}


void FGRouteMgr::sequence()
{
  if (!active->getBoolValue()) {
    SG_LOG(SG_AUTOPILOT, SG_ALERT, "trying to sequence waypoints with no active route");
    return;
  }
  
  if (checkFinished()) {
    return;
  }
  
  _currentIndex++;
  currentWaypointChanged();
}

bool FGRouteMgr::checkFinished()
{
  if (_currentIndex < (int) _route.size()) {
    return false;
  }
  
  SG_LOG(SG_AUTOPILOT, SG_INFO, "reached end of active route");
  _finished->fireValueChanged();
  active->setBoolValue(false);
  return true;
}

void FGRouteMgr::jumpToIndex(int index)
{
  if ((index < 0) || (index >= (int) _route.size())) {
    SG_LOG(SG_AUTOPILOT, SG_ALERT, "passed invalid index (" << 
      index << ") to FGRouteMgr::jumpToIndex");
    return;
  }

  if (_currentIndex == index) {
    return; // no-op
  }
  
// all the checks out the way, go ahead and update state
  _currentIndex = index;
  currentWaypointChanged();
  _currentWpt->fireValueChanged();
}

void FGRouteMgr::currentWaypointChanged()
{
  Waypt* cur = currentWaypt();
  Waypt* next = nextWaypt();

  wp0->getChild("id")->setStringValue(cur ? cur->ident() : "");
  wp1->getChild("id")->setStringValue(next ? next->ident() : "");
  
  _currentWpt->fireValueChanged();
  SG_LOG(SG_AUTOPILOT, SG_INFO, "route manager, current-wp is now " << _currentIndex);
}

int FGRouteMgr::findWayptIndex(const SGGeod& aPos) const
{  
  for (int i=0; i<numWaypts(); ++i) {
    if (_route[i]->matches(aPos)) {
      return i;
    }
  }
  
  return -1;
}

Waypt* FGRouteMgr::currentWaypt() const
{
  if ((_currentIndex < 0) || (_currentIndex >= numWaypts()))
      return NULL;
  return wayptAtIndex(_currentIndex);
}

Waypt* FGRouteMgr::previousWaypt() const
{
  if (_currentIndex == 0) {
    return NULL;
  }
  
  return wayptAtIndex(_currentIndex - 1);
}

Waypt* FGRouteMgr::nextWaypt() const
{
  if ((_currentIndex < 0) || ((_currentIndex + 1) >= numWaypts())) {
    return NULL;
  }
  
  return wayptAtIndex(_currentIndex + 1);
}

Waypt* FGRouteMgr::wayptAtIndex(int index) const
{
  if ((index < 0) || (index >= numWaypts())) {
    throw sg_range_exception("waypt index out of range", "FGRouteMgr::wayptAtIndex");
  }
  
  return _route[index];
}

void FGRouteMgr::saveRoute()
{
  SGPath path(_pathNode->getStringValue());
  SG_LOG(SG_IO, SG_INFO, "Saving route to " << path.str());
  try {
    SGPropertyNode_ptr d(new SGPropertyNode);
    SGPath path(_pathNode->getStringValue());
    d->setIntValue("version", 2);
    
    if (_departure) {
      d->setStringValue("departure/airport", _departure->ident());
      d->setStringValue("departure/sid", departure->getStringValue("sid"));
      d->setStringValue("departure/runway", departure->getStringValue("runway"));
    }
    
    if (_destination) {
      d->setStringValue("destination/airport", _destination->ident());
      d->setStringValue("destination/star", destination->getStringValue("star"));
      d->setStringValue("destination/transition", destination->getStringValue("transition"));
      d->setStringValue("destination/runway", destination->getStringValue("runway"));
    }
    
  // route nodes
    SGPropertyNode* routeNode = d->getChild("route", 0, true);
    for (unsigned int i=0; i<_route.size(); ++i) {
      Waypt* wpt = _route[i];
      wpt->saveAsNode(routeNode->getChild("wp", i, true));
    } // of waypoint iteration
    writeProperties(path.str(), d, true /* write-all */);
  } catch (sg_exception& e) {
    SG_LOG(SG_IO, SG_WARN, "failed to save flight-plan:" << e.getMessage());
  }
}

void FGRouteMgr::loadRoute()
{
  // deactivate route first
  active->setBoolValue(false);
  
  SGPropertyNode_ptr routeData(new SGPropertyNode);
  SGPath path(_pathNode->getStringValue());
  
  SG_LOG(SG_IO, SG_INFO, "going to read flight-plan from:" << path.str());
    
  try {
    readProperties(path.str(), routeData);
  } catch (sg_exception& ) {
    // if XML parsing fails, the file might be simple textual list of waypoints
    loadPlainTextRoute(path);
    return;
  }
  
  try {
    int version = routeData->getIntValue("version", 1);
    if (version == 1) {
      loadVersion1XMLRoute(routeData);
    } else if (version == 2) {
      loadVersion2XMLRoute(routeData);
    } else {
      throw sg_io_exception("unsupported XML route version");
    }
  } catch (sg_exception& e) {
    SG_LOG(SG_IO, SG_WARN, "failed to load flight-plan (from '" << e.getOrigin()
      << "'):" << e.getMessage());
  }
}

void FGRouteMgr::loadXMLRouteHeader(SGPropertyNode_ptr routeData)
{
  // departure nodes
  SGPropertyNode* dep = routeData->getChild("departure");
  if (dep) {
    string depIdent = dep->getStringValue("airport");
    _departure = (FGAirport*) fgFindAirportID(depIdent);
    departure->setStringValue("runway", dep->getStringValue("runway"));
    departure->setStringValue("sid", dep->getStringValue("sid"));
    departure->setStringValue("transition", dep->getStringValue("transition"));
  }
  
// destination
  SGPropertyNode* dst = routeData->getChild("destination");
  if (dst) {
    _destination = (FGAirport*) fgFindAirportID(dst->getStringValue("airport"));
    destination->setStringValue("runway", dst->getStringValue("runway"));
    destination->setStringValue("star", dst->getStringValue("star"));
    destination->setStringValue("transition", dst->getStringValue("transition"));
  }

// alternate
  SGPropertyNode* alt = routeData->getChild("alternate");
  if (alt) {
    alternate->setStringValue(alt->getStringValue("airport"));
  } // of cruise data loading
  
// cruise
  SGPropertyNode* crs = routeData->getChild("cruise");
  if (crs) {
    cruise->setDoubleValue("speed-kts", crs->getDoubleValue("speed-kts"));
    cruise->setDoubleValue("mach", crs->getDoubleValue("mach"));
    cruise->setDoubleValue("altitude-ft", crs->getDoubleValue("altitude-ft"));
  } // of cruise data loading

}

void FGRouteMgr::loadVersion2XMLRoute(SGPropertyNode_ptr routeData)
{
  loadXMLRouteHeader(routeData);
  
// route nodes
  WayptVec wpts;
  SGPropertyNode_ptr routeNode = routeData->getChild("route", 0);    
  for (int i=0; i<routeNode->nChildren(); ++i) {
    SGPropertyNode_ptr wpNode = routeNode->getChild("wp", i);
    WayptRef wpt = Waypt::createFromProperties(NULL, wpNode);
    wpts.push_back(wpt);
  } // of route iteration
  
  _route = wpts;
}

void FGRouteMgr::loadVersion1XMLRoute(SGPropertyNode_ptr routeData)
{
  loadXMLRouteHeader(routeData);

// route nodes
  WayptVec wpts;
  SGPropertyNode_ptr routeNode = routeData->getChild("route", 0);    
  for (int i=0; i<routeNode->nChildren(); ++i) {
    SGPropertyNode_ptr wpNode = routeNode->getChild("wp", i);
    WayptRef wpt = parseVersion1XMLWaypt(wpNode);
    wpts.push_back(wpt);
  } // of route iteration
  
  _route = wpts;
}

WayptRef FGRouteMgr::parseVersion1XMLWaypt(SGPropertyNode* aWP)
{
  SGGeod lastPos;
  if (!_route.empty()) {
    lastPos = _route.back()->position();
  } else if (_departure) {
    lastPos = _departure->geod();
  }

  WayptRef w;
  string ident(aWP->getStringValue("ident"));
  if (aWP->hasChild("longitude-deg")) {
    // explicit longitude/latitude
    w = new BasicWaypt(SGGeod::fromDeg(aWP->getDoubleValue("longitude-deg"), 
      aWP->getDoubleValue("latitude-deg")), ident, NULL);
    
  } else {
    string nid = aWP->getStringValue("navid", ident.c_str());
    FGPositionedRef p = FGPositioned::findClosestWithIdent(nid, lastPos);
    if (!p) {
      throw sg_io_exception("bad route file, unknown navid:" + nid);
    }
      
    SGGeod pos(p->geod());
    if (aWP->hasChild("offset-nm") && aWP->hasChild("offset-radial")) {
      double radialDeg = aWP->getDoubleValue("offset-radial");
      // convert magnetic radial to a true radial!
      radialDeg += magvar->getDoubleValue();
      double offsetNm = aWP->getDoubleValue("offset-nm");
      double az2;
      SGGeodesy::direct(p->geod(), radialDeg, offsetNm * SG_NM_TO_METER, pos, az2);
    }

    w = new BasicWaypt(pos, ident, NULL);
  }
  
  double altFt = aWP->getDoubleValue("altitude-ft", -9999.9);
  if (altFt > -9990.0) {
    w->setAltitude(altFt, RESTRICT_AT);
  }

  return w;
}

void FGRouteMgr::loadPlainTextRoute(const SGPath& path)
{
  sg_gzifstream in(path.str().c_str());
  if (!in.is_open()) {
    return;
  }
  
  try {
    WayptVec wpts;
    while (!in.eof()) {
      string line;
      getline(in, line, '\n');
    // trim CR from end of line, if found
      if (line[line.size() - 1] == '\r') {
        line.erase(line.size() - 1, 1);
      }
      
      line = simgear::strutils::strip(line);
      if (line.empty() || (line[0] == '#')) {
        continue; // ignore empty/comment lines
      }
      
      WayptRef w = waypointFromString(line);
      if (!w) {
        throw sg_io_exception("failed to create waypoint from line:" + line);
      }
      
      wpts.push_back(w);
    } // of line iteration
  
    _route = wpts;
  } catch (sg_exception& e) {
    SG_LOG(SG_IO, SG_WARN, "failed to load route from:" << path.str() << ":" << e.getMessage());
  }
}

const char* FGRouteMgr::getDepartureICAO() const
{
  if (!_departure) {
    return "";
  }
  
  return _departure->ident().c_str();
}

const char* FGRouteMgr::getDepartureName() const
{
  if (!_departure) {
    return "";
  }
  
  return _departure->name().c_str();
}

void FGRouteMgr::setDepartureICAO(const char* aIdent)
{
  if ((aIdent == NULL) || (strlen(aIdent) < 4)) {
    _departure = NULL;
  } else {
    _departure = FGAirport::findByIdent(aIdent);
  }
  
  departureChanged();
}

const char* FGRouteMgr::getDestinationICAO() const
{
  if (!_destination) {
    return "";
  }
  
  return _destination->ident().c_str();
}

const char* FGRouteMgr::getDestinationName() const
{
  if (!_destination) {
    return "";
  }
  
  return _destination->name().c_str();
}

void FGRouteMgr::setDestinationICAO(const char* aIdent)
{
  if ((aIdent == NULL) || (strlen(aIdent) < 4)) {
    _destination = NULL;
  } else {
    _destination = FGAirport::findByIdent(aIdent);
  }
  
  arrivalChanged();
}
