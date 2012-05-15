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
#include <simgear/structure/commands.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sg_inlines.h>

#include "Main/fg_props.hxx"
#include "Navaids/positioned.hxx"
#include <Navaids/waypoint.hxx>
#include <Navaids/procedure.hxx>
#include "Airports/simple.hxx"
#include "Airports/runways.hxx"
#include <GUI/new_gui.hxx>
#include <GUI/dialog.hxx>

#define RM "/autopilot/route-manager/"

using namespace flightgear;

static bool commandLoadFlightPlan(const SGPropertyNode* arg)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  SGPath path(arg->getStringValue("path"));
  return self->loadRoute(path);
}

static bool commandSaveFlightPlan(const SGPropertyNode* arg)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  SGPath path(arg->getStringValue("path"));
  return self->saveRoute(path);
}

static bool commandActivateFlightPlan(const SGPropertyNode* arg)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  bool activate = arg->getBoolValue("activate", true);
  if (activate) {
    self->activate();
  } else {
    
  }
  
  return true;
}

static bool commandClearFlightPlan(const SGPropertyNode*)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  self->clearRoute();
  return true;
}

static bool commandSetActiveWaypt(const SGPropertyNode* arg)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  int index = arg->getIntValue("index");
  if ((index < 0) || (index >= self->numLegs())) {
    return false;
  }
  
  self->jumpToIndex(index);
  return true;
}

static bool commandInsertWaypt(const SGPropertyNode* arg)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  int index = arg->getIntValue("index");
  std::string ident(arg->getStringValue("id"));
  int alt = arg->getIntValue("altitude-ft", -999);
  int ias = arg->getIntValue("speed-knots", -999);
  
  WayptRef wp;
// lat/lon may be supplied to narrow down navaid search, or to specify
// a raw waypoint
  SGGeod pos;
  if (arg->hasChild("longitude-deg")) {
    pos = SGGeod::fromDeg(arg->getDoubleValue("longitude-deg"),
                               arg->getDoubleValue("latitude-deg"));
  }
  
  if (arg->hasChild("navaid")) {
    FGPositionedRef p = FGPositioned::findClosestWithIdent(arg->getStringValue("navaid"), pos);
    
    if (arg->hasChild("navaid", 1)) {
      // intersection of two radials
      FGPositionedRef p2 = FGPositioned::findClosestWithIdent(arg->getStringValue("navaid[1]"), pos);
      if (!p2) {
        SG_LOG( SG_AUTOPILOT, SG_INFO, "Unable to find FGPositioned with ident:" << arg->getStringValue("navaid[1]"));
        return false;
      }
      
      double r1 = arg->getDoubleValue("radial"),
        r2 = arg->getDoubleValue("radial[1]");
      
      SGGeod intersection;
      bool ok = SGGeodesy::radialIntersection(p->geod(), r1, p2->geod(), r2, intersection);
      if (!ok) {
        SG_LOG(SG_AUTOPILOT, SG_INFO, "no valid intersection for:" << p->ident() 
                << "," << p2->ident());
        return false;
      }
      
      std::string name = p->ident() + "-" + p2->ident();
      wp = new BasicWaypt(intersection, name, NULL);
    } else if (arg->hasChild("offset-nm") && arg->hasChild("radial")) {
      // offset radial from navaid
      double radial = arg->getDoubleValue("radial");
      double distanceNm = arg->getDoubleValue("offset-nm");
      //radial += magvar->getDoubleValue(); // convert to true bearing
      wp = new OffsetNavaidWaypoint(p, NULL, radial, distanceNm);
    } else {
      wp = new NavaidWaypoint(p, NULL);
    }
  } else if (arg->hasChild("airport")) {
    const FGAirport* apt = fgFindAirportID(arg->getStringValue("airport"));
    if (!apt) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "no such airport" << arg->getStringValue("airport"));
      return false;
    }
    
    if (arg->hasChild("runway")) {
      if (!apt->hasRunwayWithIdent(arg->getStringValue("runway"))) {
        SG_LOG(SG_AUTOPILOT, SG_INFO, "No runway: " << arg->getStringValue("runway") << " at " << apt->ident());
        return false;
      }
      
      FGRunway* runway = apt->getRunwayByIdent(arg->getStringValue("runway"));
      wp = new RunwayWaypt(runway, NULL);
    } else {
      wp = new NavaidWaypoint((FGAirport*) apt, NULL);
    }
  } else if (arg->hasChild("text")) {
    wp = self->waypointFromString(arg->getStringValue("text"));
  } else if (!(pos == SGGeod())) {
    // just a raw lat/lon
    wp = new BasicWaypt(pos, ident, NULL);
  } else {
    return false; // failed to build waypoint
  }

  FlightPlan::Leg* leg = self->flightPlan()->insertWayptAtIndex(wp, index);
  if (alt >= 0) {
    leg->setAltitude(RESTRICT_AT, alt);
  }
  
  if (ias > 0) {
    leg->setSpeed(RESTRICT_AT, ias);
  }
  
  return true;
}

static bool commandDeleteWaypt(const SGPropertyNode* arg)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  int index = arg->getIntValue("index");
  self->removeLegAtIndex(index);
  return true;
}

/////////////////////////////////////////////////////////////////////////////

FGRouteMgr::FGRouteMgr() :
  _plan(NULL),
  input(fgGetNode( RM "input", true )),
  mirror(fgGetNode( RM "route", true ))
{
  listener = new InputListener(this);
  input->setStringValue("");
  input->addChangeListener(listener);
  
  SGCommandMgr::instance()->addCommand("load-flightplan", commandLoadFlightPlan);
  SGCommandMgr::instance()->addCommand("save-flightplan", commandSaveFlightPlan);
  SGCommandMgr::instance()->addCommand("activate-flightplan", commandActivateFlightPlan);
  SGCommandMgr::instance()->addCommand("clear-flightplan", commandClearFlightPlan);
  SGCommandMgr::instance()->addCommand("set-active-waypt", commandSetActiveWaypt);
  SGCommandMgr::instance()->addCommand("insert-waypt", commandInsertWaypt);
  SGCommandMgr::instance()->addCommand("delete-waypt", commandDeleteWaypt);
}


FGRouteMgr::~FGRouteMgr()
{
  input->removeChangeListener(listener);
  delete listener;
}


void FGRouteMgr::init() {
  SGPropertyNode_ptr rm(fgGetNode(RM));
  
  magvar = fgGetNode("/environment/magnetic-variation-deg", true);
     
  departure = fgGetNode(RM "departure", true);
  departure->tie("airport", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDepartureICAO, &FGRouteMgr::setDepartureICAO));
  departure->tie("runway", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
                                                                       &FGRouteMgr::getDepartureRunway, 
                                                                      &FGRouteMgr::setDepartureRunway));
  departure->tie("sid", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
                                                                      &FGRouteMgr::getSID, 
                                                                      &FGRouteMgr::setSID));
  
  departure->tie("name", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDepartureName, NULL));
  departure->tie("field-elevation-ft", SGRawValueMethods<FGRouteMgr, double>(*this, 
                                                                               &FGRouteMgr::getDestinationFieldElevation, NULL));
  departure->getChild("etd", 0, true);
  departure->getChild("takeoff-time", 0, true);

  destination = fgGetNode(RM "destination", true);
  destination->getChild("airport", 0, true);
  
  destination->tie("airport", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDestinationICAO, &FGRouteMgr::setDestinationICAO));
  destination->tie("runway", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
                             &FGRouteMgr::getDestinationRunway, 
                            &FGRouteMgr::setDestinationRunway));
  destination->tie("star", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
                                                                        &FGRouteMgr::getSTAR, 
                                                                        &FGRouteMgr::setSTAR));
  destination->tie("approach", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
                                                                        &FGRouteMgr::getApproach, 
                                                                        &FGRouteMgr::setApproach));
  
  destination->tie("name", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDestinationName, NULL));
  destination->tie("field-elevation-ft", SGRawValueMethods<FGRouteMgr, double>(*this, 
                                                                      &FGRouteMgr::getDestinationFieldElevation, NULL));
  
  destination->getChild("eta", 0, true);
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
  
  totalDistance = fgGetNode(RM "total-distance", true);
  totalDistance->setDoubleValue(0.0);
  distanceToGo = fgGetNode(RM "distance-remaining-nm", true);
  distanceToGo->setDoubleValue(0.0);
  
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
  _flightplanChanged = fgGetNode(RM "signals/flightplan-changed", true);
  
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
  
  _pathNode = fgGetNode(RM "file-path", 0, true);
}


void FGRouteMgr::postinit()
{
  setFlightPlan(new FlightPlan());
  
  SGPath path(_pathNode->getStringValue());
  if (!path.isNull()) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "loading flight-plan from: " << path.str());
    loadRoute(path);
  }
  
// this code only matters for the --wp option now - perhaps the option
// should be deprecated in favour of an explicit flight-plan file?
// then the global initial waypoint list could die.
  string_list *waypoints = globals->get_initial_waypoints();
  if (waypoints) {
    string_list::iterator it;
    for (it = waypoints->begin(); it != waypoints->end(); ++it) {
      WayptRef w = waypointFromString(*it);
      if (w) {
        _plan->insertWayptAtIndex(w, -1);
      }
    }
    
    SG_LOG(SG_AUTOPILOT, SG_INFO, "loaded initial waypoints:" << numLegs());
    update_mirror();
  }

  weightOnWheels = fgGetNode("/gear/gear[0]/wow", true);
  groundSpeed = fgGetNode("/velocities/groundspeed-kt", true);
  
  // check airbone flag agrees with presets
}

void FGRouteMgr::bind() { }
void FGRouteMgr::unbind() { }

bool FGRouteMgr::isRouteActive() const
{
  return active->getBoolValue();
}

bool FGRouteMgr::saveRoute(const SGPath& p)
{
  if (!_plan) {
    return false;
  }
  
  return _plan->save(p);
}

bool FGRouteMgr::loadRoute(const SGPath& p)
{
  FlightPlan* fp = new FlightPlan;
  if (!fp->load(p)) {
    delete fp;
    return false;
  }
  
  setFlightPlan(fp);
  return true;
}

FlightPlan* FGRouteMgr::flightPlan() const
{
  return _plan;
}

void FGRouteMgr::setFlightPlan(FlightPlan* plan)
{
  if (plan == _plan) {
    return;
  }
  
  if (_plan) {
    _plan->removeDelegate(this);
    delete _plan;
    active->setBoolValue(false);
  }
  
  _plan = plan;
  _plan->addDelegate(this);
  
  _flightplanChanged->fireValueChanged();
  
// fire all the callbacks!
  departureChanged();
  arrivalChanged();
  waypointsChanged();
  currentWaypointChanged();
}

void FGRouteMgr::update( double dt )
{
  if (dt <= 0.0) {
    return; // paused, nothing to do here
  }
  
  double gs = groundSpeed->getDoubleValue();
  if (airborne->getBoolValue()) {
    time_t now = time(NULL);
    elapsedFlightTime->setDoubleValue(difftime(now, _takeoffTime));
    
    if (weightOnWheels->getBoolValue()) {
      // touch down
      destination->setIntValue("touchdown-time", time(NULL));
      airborne->setBoolValue(false);
    }
  } else { // not airborne
    if (weightOnWheels->getBoolValue() || (gs < 40)) {
      // either taking-off or rolling-out after touchdown
    } else {
      airborne->setBoolValue(true);
      _takeoffTime = time(NULL); // start the clock
      departure->setIntValue("takeoff-time", _takeoffTime);
    }
  }
  
  if (!active->getBoolValue()) {
    return;
  }

  if (checkFinished()) {
    // maybe we're done
  }
  
// basic course/distance information
  SGGeod currentPos = globals->get_aircraft_position();

  FlightPlan::Leg* leg = _plan ? _plan->currentLeg() : NULL;
  if (!leg) {
    return;
  }
  
  double courseDeg;
  double distanceM;
  boost::tie(courseDeg, distanceM) = leg->waypoint()->courseAndDistanceFrom(currentPos);
  
// update wp0 / wp1 / wp-last
  wp0->setDoubleValue("dist", distanceM * SG_METER_TO_NM);
  wp0->setDoubleValue("true-bearing-deg", courseDeg);
  courseDeg -= magvar->getDoubleValue(); // expose magnetic bearing
  wp0->setDoubleValue("bearing-deg", courseDeg);
  setETAPropertyFromDistance(wp0->getChild("eta"), distanceM);
  
  double totalPathDistanceNm = _plan->totalDistanceNm();
  double totalDistanceRemaining = distanceM * SG_METER_TO_NM; // distance to current waypoint
  
// total distance to go, is direct distance to wp0, plus the remaining
// path distance from wp0
  totalDistanceRemaining += (totalPathDistanceNm - leg->distanceAlongRoute());
  
  wp0->setDoubleValue("distance-along-route-nm", 
                      leg->distanceAlongRoute());
  wp0->setDoubleValue("remaining-distance-nm", 
                      totalPathDistanceNm - leg->distanceAlongRoute());
  
  FlightPlan::Leg* nextLeg = _plan->nextLeg();
  if (nextLeg) {
    boost::tie(courseDeg, distanceM) = nextLeg->waypoint()->courseAndDistanceFrom(currentPos);
     
    wp1->setDoubleValue("dist", distanceM * SG_METER_TO_NM);
    wp1->setDoubleValue("true-bearing-deg", courseDeg);
    courseDeg -= magvar->getDoubleValue(); // expose magnetic bearing
    wp1->setDoubleValue("bearing-deg", courseDeg);
    setETAPropertyFromDistance(wp1->getChild("eta"), distanceM);    
    wp1->setDoubleValue("distance-along-route-nm", 
                        nextLeg->distanceAlongRoute());
    wp1->setDoubleValue("remaining-distance-nm", 
                        totalPathDistanceNm - nextLeg->distanceAlongRoute());
  }
  
  distanceToGo->setDoubleValue(totalDistanceRemaining);
  wpn->setDoubleValue("dist", totalDistanceRemaining);
  ete->setDoubleValue(totalDistanceRemaining / gs * 3600.0);
  setETAPropertyFromDistance(wpn->getChild("eta"), totalDistanceRemaining);
}

void FGRouteMgr::clearRoute()
{
  if (_plan) {
    _plan->clear();
  }
}

Waypt* FGRouteMgr::currentWaypt() const
{
  if (_plan && _plan->currentLeg()) {
    return _plan->currentLeg()->waypoint();
  }
  
  return NULL;
}

int FGRouteMgr::currentIndex() const
{
  if (!_plan) {
    return 0;
  }
  
  return _plan->currentIndex();
}

Waypt* FGRouteMgr::wayptAtIndex(int index) const
{
  if (_plan) {
    FlightPlan::Leg* leg = _plan->legAtIndex(index);
    if (leg) {
      return leg->waypoint();
    }
  }
  
  return NULL;
}

int FGRouteMgr::numLegs() const
{
  if (_plan) {
    return _plan->numLegs();
  }
  
  return 0;
}

void FGRouteMgr::setETAPropertyFromDistance(SGPropertyNode_ptr aProp, double aDistance)
{
  double speed = groundSpeed->getDoubleValue();
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

void FGRouteMgr::removeLegAtIndex(int aIndex)
{
  if (!_plan) {
    return;
  }
  
  _plan->deleteIndex(aIndex);
}
  
void FGRouteMgr::waypointsChanged()
{
  update_mirror();
   _edited->fireValueChanged();
  checkFinished();
}

// mirror internal route to the property system for inspection by other subsystems
void FGRouteMgr::update_mirror()
{
  mirror->removeChildren("wp");
  NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
  FGDialog* rmDlg = gui ? gui->getDialog("route-manager") : NULL;

  if (!_plan) {
    mirror->setIntValue("num", 0);
    if (rmDlg) {
      rmDlg->updateValues();
    }
    return;
  }
  
  int num = _plan->numLegs();
    
  for (int i = 0; i < num; i++) {
    FlightPlan::Leg* leg = _plan->legAtIndex(i);
    WayptRef wp = leg->waypoint();
    SGPropertyNode *prop = mirror->getChild("wp", i, 1);

    const SGGeod& pos(wp->position());
    prop->setStringValue("id", wp->ident().c_str());
    prop->setDoubleValue("longitude-deg", pos.getLongitudeDeg());
    prop->setDoubleValue("latitude-deg",pos.getLatitudeDeg());
   
    // leg course+distance

    prop->setDoubleValue("leg-bearing-true-deg", leg->courseDeg());
    prop->setDoubleValue("leg-distance-nm", leg->distanceNm());
    prop->setDoubleValue("distance-along-route-nm", leg->distanceAlongRoute());
    
    if (leg->altitudeRestriction() != RESTRICT_NONE) {
      double ft = leg->altitudeFt();
      prop->setDoubleValue("altitude-m", ft * SG_FEET_TO_METER);
      prop->setDoubleValue("altitude-ft", ft);
      prop->setIntValue("flight-level", static_cast<int>(ft / 1000) * 10);
    } else {
      prop->setDoubleValue("altitude-m", -9999.9);
      prop->setDoubleValue("altitude-ft", -9999.9);
    }
    
    if (leg->speedRestriction() == SPEED_RESTRICT_MACH) {
      prop->setDoubleValue("speed-mach", leg->speedMach());
    } else if (leg->speedRestriction() != RESTRICT_NONE) {
      prop->setDoubleValue("speed-kts", leg->speedKts());
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
  mirror->setIntValue("num", _plan->numLegs());
    
  if (rmDlg) {
    rmDlg->updateValues();
  }
  
  totalDistance->setDoubleValue(_plan->totalDistanceNm());
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
      SGPath path(mgr->_pathNode->getStringValue());
      mgr->loadRoute(path);
    } else if (!strcmp(s, "@SAVE")) {
      SGPath path(mgr->_pathNode->getStringValue());
      mgr->saveRoute(path);
    } else if (!strcmp(s, "@NEXT")) {
      mgr->jumpToIndex(mgr->currentIndex() + 1);
    } else if (!strcmp(s, "@PREVIOUS")) {
      mgr->jumpToIndex(mgr->currentIndex() - 1);
    } else if (!strncmp(s, "@JUMP", 5)) {
      mgr->jumpToIndex(atoi(s + 5));
    } else if (!strncmp(s, "@DELETE", 7))
        mgr->removeLegAtIndex(atoi(s + 7));
    else if (!strncmp(s, "@INSERT", 7)) {
        char *r;
        int pos = strtol(s + 7, &r, 10);
        if (*r++ != ':')
            return;
        while (isspace(*r))
            r++;
        if (*r)
            mgr->flightPlan()->insertWayptAtIndex(mgr->waypointFromString(r), pos);
    } else if (!strcmp(s, "@POSINIT")) {
      mgr->initAtPosition();
    } else
      mgr->flightPlan()->insertWayptAtIndex(mgr->waypointFromString(s), -1);
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
    _plan->setDeparture((FGAirport*) NULL);
    return;
  }
  
// on the ground
  SGGeod pos = globals->get_aircraft_position();
  if (!_plan->departureAirport()) {
    _plan->setDeparture(FGAirport::findClosest(pos, 20.0));
    if (!_plan->departureAirport()) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "initAtPosition: couldn't find an airport within 20nm");
      return;
    }
  }
  
  std::string rwy = departure->getStringValue("runway");
  FGRunway* r = NULL;
  if (!rwy.empty()) {
    r = _plan->departureAirport()->getRunwayByIdent(rwy);
  } else {
    r = _plan->departureAirport()->findBestRunwayForPos(pos);
  }
  
  if (!r) {
    return;
  }
  
  _plan->setDeparture(r);
  SG_LOG(SG_AUTOPILOT, SG_INFO, "initAtPosition: starting at " 
    << _plan->departureAirport()->ident() << " on runway " << r->ident());
}

bool FGRouteMgr::haveUserWaypoints() const
{
  // FIXME
  return false;
}

bool FGRouteMgr::activate()
{
  if (!_plan) {
    SG_LOG(SG_AUTOPILOT, SG_WARN, "::activate, no flight plan defined");
    return false;
  }
  
  if (isRouteActive()) {
    SG_LOG(SG_AUTOPILOT, SG_WARN, "duplicate route-activation, no-op");
    return false;
  }
 
  _plan->setCurrentIndex(0);
  active->setBoolValue(true);
  SG_LOG(SG_AUTOPILOT, SG_INFO, "route-manager, activate route ok");
  return true;
}


void FGRouteMgr::sequence()
{
  if (!_plan || !active->getBoolValue()) {
    SG_LOG(SG_AUTOPILOT, SG_ALERT, "trying to sequence waypoints with no active route");
    return;
  }
  
  if (checkFinished()) {
    return;
  }
  
  _plan->setCurrentIndex(_plan->currentIndex() + 1);
}

bool FGRouteMgr::checkFinished()
{
  if (!_plan) {
    return true;
  }
  
  bool done = false;
// done if we're stopped on the destination runway 
  if (_plan->currentLeg() && 
      (_plan->currentLeg()->waypoint()->source() == _plan->destinationRunway())) 
  {
    double gs = groundSpeed->getDoubleValue();
    done = weightOnWheels->getBoolValue() && (gs < 25);
  }
  
// also done if we hit the final waypoint
  if (_plan->currentIndex() >= _plan->numLegs()) {
    done = true;
  }
  
  if (done) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "reached end of active route");
    _finished->fireValueChanged();
    active->setBoolValue(false);
  }
  
  return done;
}

void FGRouteMgr::jumpToIndex(int index)
{
  if (!_plan) {
    return;
  }
  
  _plan->setCurrentIndex(index);
}

void FGRouteMgr::currentWaypointChanged()
{
  Waypt* cur = currentWaypt();
  FlightPlan::Leg* next = _plan ? _plan->nextLeg() : NULL;

  wp0->getChild("id")->setStringValue(cur ? cur->ident() : "");
  wp1->getChild("id")->setStringValue(next ? next->waypoint()->ident() : "");
  
  _currentWpt->fireValueChanged();
  SG_LOG(SG_AUTOPILOT, SG_INFO, "route manager, current-wp is now " << currentIndex());
}

const char* FGRouteMgr::getDepartureICAO() const
{
  if (!_plan || !_plan->departureAirport()) {
    return "";
  }
  
  return _plan->departureAirport()->ident().c_str();
}

const char* FGRouteMgr::getDepartureName() const
{
  if (!_plan || !_plan->departureAirport()) {
    return "";
  }
  
  return _plan->departureAirport()->name().c_str();
}

const char* FGRouteMgr::getDepartureRunway() const
{
  if (_plan && _plan->departureRunway()) {
    return _plan->departureRunway()->ident().c_str();
  }
  
  return "";
}

void FGRouteMgr::setDepartureRunway(const char* aIdent)
{
  FGAirport* apt = _plan->departureAirport();
  if (!apt || (aIdent == NULL)) {
    _plan->setDeparture(apt);
  } else if (apt->hasRunwayWithIdent(aIdent)) {
    _plan->setDeparture(apt->getRunwayByIdent(aIdent));
  }
}

void FGRouteMgr::setDepartureICAO(const char* aIdent)
{
  if ((aIdent == NULL) || (strlen(aIdent) < 4)) {
    _plan->setDeparture((FGAirport*) NULL);
  } else {
    _plan->setDeparture(FGAirport::findByIdent(aIdent));
  }
}

const char* FGRouteMgr::getSID() const
{
  if (_plan && _plan->sid()) {
    return _plan->sid()->ident().c_str();
  }
  
  return "";
}

void FGRouteMgr::setSID(const char* aIdent)
{
  FGAirport* apt = _plan->departureAirport();
  if (!apt || (aIdent == NULL)) {
    _plan->setSID((flightgear::SID*) NULL);
    return;
  } 
  
  string ident(aIdent);
  size_t hyphenPos = ident.find('-');
  if (hyphenPos != string::npos) {
    string sidIdent = ident.substr(0, hyphenPos);
    string transIdent = ident.substr(hyphenPos + 1);
    
    flightgear::SID* sid = apt->findSIDWithIdent(sidIdent);
    Transition* trans = sid ? sid->findTransitionByName(transIdent) : NULL;
    _plan->setSID(trans);
  } else {
    _plan->setSID(apt->findSIDWithIdent(aIdent));
  }
}

const char* FGRouteMgr::getDestinationICAO() const
{
  if (!_plan || !_plan->destinationAirport()) {
    return "";
  }
  
  return _plan->destinationAirport()->ident().c_str();
}

const char* FGRouteMgr::getDestinationName() const
{
  if (!_plan || !_plan->destinationAirport()) {
    return "";
  }
  
  return _plan->destinationAirport()->name().c_str();
}

void FGRouteMgr::setDestinationICAO(const char* aIdent)
{
  if ((aIdent == NULL) || (strlen(aIdent) < 4)) {
    _plan->setDestination((FGAirport*) NULL);
  } else {
    _plan->setDestination(FGAirport::findByIdent(aIdent));
  }
}

const char* FGRouteMgr::getDestinationRunway() const
{
  if (_plan && _plan->destinationRunway()) {
    return _plan->destinationRunway()->ident().c_str();
  }
  
  return "";
}

void FGRouteMgr::setDestinationRunway(const char* aIdent)
{
  FGAirport* apt = _plan->destinationAirport();
  if (!apt || (aIdent == NULL)) {
    _plan->setDestination(apt);
  } else if (apt->hasRunwayWithIdent(aIdent)) {
    _plan->setDestination(apt->getRunwayByIdent(aIdent));
  }
}

const char* FGRouteMgr::getApproach() const
{
  if (_plan && _plan->approach()) {
    return _plan->approach()->ident().c_str();
  }
  
  return "";
}

void FGRouteMgr::setApproach(const char* aIdent)
{
  FGAirport* apt = _plan->destinationAirport();
  if (!apt || (aIdent == NULL)) {
    _plan->setApproach(NULL);
  } else {
    _plan->setApproach(apt->findApproachWithIdent(aIdent));
  }
}

const char* FGRouteMgr::getSTAR() const
{
  if (_plan && _plan->star()) {
    return _plan->star()->ident().c_str();
  }
  
  return "";
}

void FGRouteMgr::setSTAR(const char* aIdent)
{
  FGAirport* apt = _plan->destinationAirport();
  if (!apt || (aIdent == NULL)) {
    _plan->setSTAR((STAR*) NULL);
    return;
  } 
  
  string ident(aIdent);
  size_t hyphenPos = ident.find('-');
  if (hyphenPos != string::npos) {
    string starIdent = ident.substr(0, hyphenPos);
    string transIdent = ident.substr(hyphenPos + 1);
    
    STAR* star = apt->findSTARWithIdent(starIdent);
    Transition* trans = star ? star->findTransitionByName(transIdent) : NULL;
    _plan->setSTAR(trans);
  } else {
    _plan->setSTAR(apt->findSTARWithIdent(aIdent));
  }
}

WayptRef FGRouteMgr::waypointFromString(const std::string& target)
{
  return _plan->waypointFromString(target);
}

double FGRouteMgr::getDepartureFieldElevation() const
{
  if (!_plan || !_plan->departureAirport()) {
    return 0.0;
  }
  
  return _plan->departureAirport()->elevation();
}

double FGRouteMgr::getDestinationFieldElevation() const
{
  if (!_plan || !_plan->destinationAirport()) {
    return 0.0;
  }
  
  return _plan->destinationAirport()->elevation();
}

SGPropertyNode_ptr FGRouteMgr::wayptNodeAtIndex(int index) const
{
  if ((index < 0) || (index >= numWaypts())) {
    throw sg_range_exception("waypt index out of range", "FGRouteMgr::wayptAtIndex");
  }
  
  return mirror->getChild("wp", index);
}
