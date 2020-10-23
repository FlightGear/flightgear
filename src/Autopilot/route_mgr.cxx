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


#include <config.h>

#include <cstdio>

#include <simgear/compiler.h>
#include "route_mgr.hxx"

#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/sg_inlines.h>

#include <Main/globals.hxx>
#include "Main/fg_props.hxx"
#include "Navaids/positioned.hxx"
#include <Navaids/waypoint.hxx>
#include <Navaids/procedure.hxx>
#include <Navaids/routePath.hxx>

#include "Airports/airport.hxx"
#include "Airports/runways.hxx"
#include <GUI/new_gui.hxx>
#include <GUI/dialog.hxx>
#include <Main/util.hxx>        // fgValidatePath()
#include <GUI/MessageBox.hxx>

#define RM "/autopilot/route-manager/"

using namespace flightgear;
using std::string;

static bool commandLoadFlightPlan(const SGPropertyNode* arg, SGPropertyNode *)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  SGPath path = SGPath::fromUtf8(arg->getStringValue("path"));
  return self->loadRoute(path);
}

static bool commandSaveFlightPlan(const SGPropertyNode* arg, SGPropertyNode *)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  SGPath path = SGPath::fromUtf8(arg->getStringValue("path"));
  SGPath authorizedPath = fgValidatePath(path, true /* write */);

  if (!authorizedPath.isNull()) {
    return self->saveRoute(authorizedPath);
  } else {
    std::string msg =
          "The route manager was asked to write the flightplan to '" +
          path.utf8Str() + "', but this path is not authorized for writing. " +
          "Please choose another location, for instance in the $FG_HOME/Export "
          "folder (" + (globals->get_fg_home() / "Export").utf8Str() + ").";

    SG_LOG(SG_AUTOPILOT, SG_ALERT, msg);
    modalMessageBox("FlightGear", "Unable to write to the specified file",
                        msg);
    return false;
  }
}

static bool commandActivateFlightPlan(const SGPropertyNode* arg, SGPropertyNode *)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  bool activate = arg->getBoolValue("activate", true);
  if (activate) {
    self->activate();
  } else {
    self->deactivate();
  }
  
  return true;
}

static bool commandClearFlightPlan(const SGPropertyNode*, SGPropertyNode *)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  self->clearRoute();
  return true;
}

static bool commandSetActiveWaypt(const SGPropertyNode* arg, SGPropertyNode *)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  int index = arg->getIntValue("index");
  if ((index < 0) || (index >= self->numLegs())) {
    return false;
  }
  
  self->jumpToIndex(index);
  return true;
}

static bool commandInsertWaypt(const SGPropertyNode* arg, SGPropertyNode *)
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

static bool commandDeleteWaypt(const SGPropertyNode* arg, SGPropertyNode *)
{
  FGRouteMgr* self = (FGRouteMgr*) globals->get_subsystem("route-manager");
  int index = arg->getIntValue("index");
  self->removeLegAtIndex(index);
  return true;
}

/////////////////////////////////////////////////////////////////////////////

FGRouteMgr::FGRouteMgr() :
  input(fgGetNode( RM "input", true )),
  mirror(fgGetNode( RM "route", true ))
{
  listener = new InputListener(this);
  input->setStringValue("");
  input->addChangeListener(listener);
  
  SGCommandMgr* cmdMgr = globals->get_commands();
  cmdMgr->addCommand("define-user-waypoint", this, &FGRouteMgr::commandDefineUserWaypoint);
  cmdMgr->addCommand("delete-user-waypoint", this, &FGRouteMgr::commandDeleteUserWaypoint);
    
  cmdMgr->addCommand("load-flightplan", commandLoadFlightPlan);
  cmdMgr->addCommand("save-flightplan", commandSaveFlightPlan);
  cmdMgr->addCommand("activate-flightplan", commandActivateFlightPlan);
  cmdMgr->addCommand("clear-flightplan", commandClearFlightPlan);
  cmdMgr->addCommand("set-active-waypt", commandSetActiveWaypt);
  cmdMgr->addCommand("insert-waypt", commandInsertWaypt);
  cmdMgr->addCommand("delete-waypt", commandDeleteWaypt);
}


FGRouteMgr::~FGRouteMgr()
{
  input->removeChangeListener(listener);
  delete listener;
    
    if (_plan) {
		_plan->removeDelegate(this);
	}

    SGCommandMgr* cmdMgr = globals->get_commands();
    cmdMgr->removeCommand("define-user-waypoint");
    cmdMgr->removeCommand("delete-user-waypoint");
    cmdMgr->removeCommand("load-flightplan");
    cmdMgr->removeCommand("save-flightplan");
    cmdMgr->removeCommand("activate-flightplan");
    cmdMgr->removeCommand("clear-flightplan");
    cmdMgr->removeCommand("set-active-waypt");
    cmdMgr->removeCommand("insert-waypt");
    cmdMgr->removeCommand("delete-waypt");
}


void FGRouteMgr::init() {
  SGPropertyNode_ptr rm(fgGetNode(RM));
  
  magvar = fgGetNode("/environment/magnetic-variation-deg", true);
     
  departure = fgGetNode(RM "departure", true);
  departure->tie("airport", SGStringValueMethods<FGRouteMgr>(*this, 
    &FGRouteMgr::getDepartureICAO, &FGRouteMgr::setDepartureICAO));
  departure->tie("runway", SGStringValueMethods<FGRouteMgr>(*this, 
                                                            &FGRouteMgr::getDepartureRunway,
                                                            &FGRouteMgr::setDepartureRunway));
  departure->tie("sid", SGStringValueMethods<FGRouteMgr>(*this, 
                                                         &FGRouteMgr::getSID,
                                                         &FGRouteMgr::setSID));
  
  departure->tie("name", SGStringValueMethods<FGRouteMgr>(*this, 
    &FGRouteMgr::getDepartureName, nullptr));
  departure->tie("field-elevation-ft", SGRawValueMethods<FGRouteMgr, double>(*this, 
                                                                             &FGRouteMgr::getDepartureFieldElevation, nullptr));
  departure->getChild("etd", 0, true);
  departure->getChild("takeoff-time", 0, true);

  destination = fgGetNode(RM "destination", true);
  destination->getChild("airport", 0, true);
  
  destination->tie("airport", SGStringValueMethods<FGRouteMgr>(*this, 
    &FGRouteMgr::getDestinationICAO, &FGRouteMgr::setDestinationICAO));
  destination->tie("runway", SGStringValueMethods<FGRouteMgr>(*this, 
                             &FGRouteMgr::getDestinationRunway, 
                            &FGRouteMgr::setDestinationRunway));
  destination->tie("star", SGStringValueMethods<FGRouteMgr>(*this, 
                                                            &FGRouteMgr::getSTAR,
                                                            &FGRouteMgr::setSTAR));
  destination->tie("approach", SGStringValueMethods<FGRouteMgr>(*this, 
                                                                &FGRouteMgr::getApproach,
                                                                &FGRouteMgr::setApproach));
  
  destination->tie("name", SGStringValueMethods<FGRouteMgr>(*this, 
    &FGRouteMgr::getDestinationName, nullptr));
  destination->tie("field-elevation-ft", SGRawValueMethods<FGRouteMgr, double>(*this, 
                                                                      &FGRouteMgr::getDestinationFieldElevation, nullptr));
  
  destination->getChild("eta", 0, true);
  destination->getChild("eta-seconds", 0, true);
  destination->getChild("touchdown-time", 0, true);

  alternate = fgGetNode(RM "alternate", true);
  alternate->tie("airport", SGStringValueMethods<FGRouteMgr>(*this,
                             &FGRouteMgr::getAlternate,
                            &FGRouteMgr::setAlternate));
  alternate->tie("name", SGStringValueMethods<FGRouteMgr>(*this,
    &FGRouteMgr::getAlternateName, nullptr));

  cruise = fgGetNode(RM "cruise", true);
  cruise->tie("altitude-ft", SGRawValueMethods<FGRouteMgr, int>(*this,
                                                           &FGRouteMgr::getCruiseAltitudeFt,
                                                           &FGRouteMgr::setCruiseAltitudeFt));
  cruise->tie("flight-level", SGRawValueMethods<FGRouteMgr, int>(*this,
                                                            &FGRouteMgr::getCruiseFlightLevel,
                                                            &FGRouteMgr::setCruiseFlightLevel));
  cruise->tie("speed-kts", SGRawValueMethods<FGRouteMgr, int>(*this,
                                                         &FGRouteMgr::getCruiseSpeedKnots,
                                                         &FGRouteMgr::setCruiseSpeedKnots));
  cruise->tie("mach", SGRawValueMethods<FGRouteMgr, double>(*this,
                                                       &FGRouteMgr::getCruiseSpeedMach,
                                                       &FGRouteMgr::setCruiseSpeedMach));

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
  _flightplanChanged = fgGetNode(RM "signals/flightplan-changed", true);
  
  _currentWpt = fgGetNode(RM "current-wp", true);
  _currentWpt->setAttribute(SGPropertyNode::LISTENER_SAFE, true);
  _currentWpt->tie(SGRawValueMethods<FGRouteMgr, int>
    (*this, &FGRouteMgr::currentIndex, &FGRouteMgr::jumpToIndex));
      
  wp0 = fgGetNode(RM "wp", 0, true);
  wp0->getChild("id", 0, true);
  wp0->getChild("dist", 0, true);
  wp0->getChild("eta", 0, true);
  wp0->getChild("eta-seconds", 0, true);
  wp0->getChild("bearing-deg", 0, true);
  
  wp1 = fgGetNode(RM "wp", 1, true);
  wp1->getChild("id", 0, true);
  wp1->getChild("dist", 0, true);
  wp1->getChild("eta", 0, true);
  wp1->getChild("eta-seconds", 0, true);
  
  wpn = fgGetNode(RM "wp-last", 0, true);
  wpn->getChild("dist", 0, true);
  wpn->getChild("eta", 0, true);
  wpn->getChild("eta-seconds", 0, true);
  
  _pathNode = fgGetNode(RM "file-path", 0, true);
}


void FGRouteMgr::postinit()
{
  setFlightPlan(new FlightPlan());
  _plan->setIdent("default-flightplan");
    
  SGPath path = SGPath::fromUtf8(_pathNode->getStringValue());
  if (!path.isNull()) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "loading flight-plan from: " << path);
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

FlightPlanRef FGRouteMgr::flightPlan() const
{
  return _plan;
}

void FGRouteMgr::setFlightPlan(const FlightPlanRef& plan)
{
  if (plan == _plan) {
    return;
  }
  
  if (_plan) {
      _plan->removeDelegate(this);

      if (isRouteActive()) {
          _plan->finish();
      }
      
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
      time_t now = globals->get_time_params()->get_cur_time();
    elapsedFlightTime->setDoubleValue(difftime(now, _takeoffTime));
    
    if (weightOnWheels->getBoolValue()) {
      // touch down
      destination->setIntValue("touchdown-time", now);
      airborne->setBoolValue(false);
    }
  } else { // not airborne
    if (weightOnWheels->getBoolValue() || (gs < 40)) {
      // either taking-off or rolling-out after touchdown
    } else {
      airborne->setBoolValue(true);
      _takeoffTime = globals->get_time_params()->get_cur_time(); // start the clock
      departure->setIntValue("takeoff-time", _takeoffTime);
    }
  }
  
  if (!active->getBoolValue()) {
    return;
  }

// basic course/distance information
  SGGeod currentPos = globals->get_aircraft_position();

  FlightPlan::Leg* leg = _plan ? _plan->currentLeg() : NULL;
  if (!leg) {
    return;
  }
  
  // use RoutePath to compute location of active WP
  RoutePath path(_plan);
  SGGeod wpPos = path.positionForIndex(_plan->currentIndex());
  double courseDeg, az2, distanceM;
  SGGeodesy::inverse(currentPos, wpPos, courseDeg, az2, distanceM);

  // update wp0 / wp1 / wp-last
  wp0->setDoubleValue("dist", distanceM * SG_METER_TO_NM);
  wp0->setDoubleValue("true-bearing-deg", courseDeg);
  courseDeg -= magvar->getDoubleValue(); // expose magnetic bearing
  wp0->setDoubleValue("bearing-deg", courseDeg);
  setETAPropertyFromDistance(wp0, distanceM);
  
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
    wpPos = path.positionForIndex(_plan->currentIndex() + 1);
    SGGeodesy::inverse(currentPos, wpPos, courseDeg, az2, distanceM);

    wp1->setDoubleValue("dist", distanceM * SG_METER_TO_NM);
    wp1->setDoubleValue("true-bearing-deg", courseDeg);
    courseDeg -= magvar->getDoubleValue(); // expose magnetic bearing
    wp1->setDoubleValue("bearing-deg", courseDeg);
    setETAPropertyFromDistance(wp1, distanceM);    
    wp1->setDoubleValue("distance-along-route-nm", 
                        nextLeg->distanceAlongRoute());
    wp1->setDoubleValue("remaining-distance-nm", 
                        totalPathDistanceNm - nextLeg->distanceAlongRoute());
  }
  
  distanceToGo->setDoubleValue(totalDistanceRemaining);
  wpn->setDoubleValue("dist", totalDistanceRemaining);
  ete->setDoubleValue(totalDistanceRemaining / gs * 3600.0);
  setETAPropertyFromDistance(wpn, totalDistanceRemaining);
}

void FGRouteMgr::clearRoute()
{
  if (_plan) {
      _plan->clearLegs();
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
  if (!_plan) {
    throw sg_range_exception("wayptAtindex: no flightplan");
  }
  
  return _plan->legAtIndex(index)->waypoint();
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
  aProp->getChild("eta-seconds")->setIntValue( eta * 3600 );
  if ( eta >= 100.0 ) { 
      eta = 99.999; // clamp
  }
  
  if ( eta < (1.0/6.0) ) {
    eta *= 60.0; // within 10 minutes, bump up to min/secs
  }
  
  int major = (int)eta, 
      minor = (int)((eta - (int)eta) * 60.0);
  snprintf( eta_str, 64, "%d:%02d", major, minor );
  aProp->getChild("eta")->setStringValue( eta_str );
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
    prop->setStringValue("id", wp->ident());
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
      SGPath path = SGPath::fromUtf8(mgr->_pathNode->getStringValue());
      mgr->loadRoute(path);
    } else if (!strcmp(s, "@SAVE")) {
      SGPath path = SGPath::fromUtf8(mgr->_pathNode->getStringValue());
      SGPath authorizedPath = fgValidatePath(path, true /* write */);

      if (!authorizedPath.isNull()) {
        mgr->saveRoute(authorizedPath);
      } else {
        std::string msg =
          "The route manager was asked to write the flightplan to '" +
          path.utf8Str() + "', but this path is not authorized for writing. " +
          "Please choose another location, for instance in the $FG_HOME/Export "
          "folder (" + (globals->get_fg_home() / "Export").utf8Str() + ").";

        SG_LOG(SG_AUTOPILOT, SG_ALERT, msg);
        modalMessageBox("FlightGear", "Unable to write to the specified file",
                        msg);
      }
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
    } else
      mgr->flightPlan()->insertWayptAtIndex(mgr->waypointFromString(s), -1);
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
 
  _plan->activate();
  active->setBoolValue(true);
  SG_LOG(SG_AUTOPILOT, SG_INFO, "route-manager, activate route ok");
  return true;
}

void FGRouteMgr::deactivate()
{
  if (!isRouteActive()) {
    return;
  }
  
  SG_LOG(SG_AUTOPILOT, SG_INFO, "deactivating flight plan");
  active->setBoolValue(false);
}

void FGRouteMgr::jumpToIndex(int index)
{
  if (!_plan) {
    return;
  }

  // this method is tied() to current-wp property, but FlightPlan::setCurrentIndex
  // will throw on invalid input, so guard against invalid values here.
  // See Sentry FLIGHTGEAR-71
  if ((index < -1) || (index >= _plan->numLegs())) {
    SG_LOG(SG_AUTOPILOT, SG_WARN, "FGRouteMgr::jumpToIndex: ignoring invalid index:" << index);
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

std::string FGRouteMgr::getDepartureICAO() const
{
  if (!_plan || !_plan->departureAirport()) {
    return "";
  }
  
  return _plan->departureAirport()->ident();
}

std::string FGRouteMgr::getDepartureName() const
{
  if (!_plan || !_plan->departureAirport()) {
    return "";
  }
  
  return _plan->departureAirport()->name();
}

std::string FGRouteMgr::getDepartureRunway() const
{
  if (_plan && _plan->departureRunway()) {
    return _plan->departureRunway()->ident();
  }
  
  return "";
}

void FGRouteMgr::setDepartureRunway(const std::string& aIdent)
{
    if (!_plan) {
        return;
    }
    
  FGAirport* apt = _plan->departureAirport();
  if (!apt || aIdent.empty()) {
    _plan->setDeparture(apt);
  } else if (apt->hasRunwayWithIdent(aIdent)) {
    _plan->setDeparture(apt->getRunwayByIdent(aIdent));
  }
}

void FGRouteMgr::setDepartureICAO(const std::string& aIdent)
{
    if (!_plan) {
        return;
    }
    
  if (aIdent.length() < 3) {
    _plan->setDeparture((FGAirport*) nullptr);
  } else {
    _plan->setDeparture(FGAirport::findByIdent(aIdent));
  }
}

std::string FGRouteMgr::getSID() const
{
  if (_plan && _plan->sid()) {
    return _plan->sid()->ident();
  }
  
  return "";
}

static double headingDiffDeg(double a, double b)
{
  double rawDiff = b - a;
  SG_NORMALIZE_RANGE(rawDiff, -180.0, 180.0);
  return rawDiff;
}

flightgear::SID* createDefaultSID(FGRunway* aRunway, double enrouteCourse)
{
  if (!aRunway) {
    return NULL;
  }
  
  double runwayElevFt = aRunway->end().getElevationFt();
  WayptVec wpts;
  std::ostringstream ss;
  ss << aRunway->ident() << "-3";
  
  SGGeod p = aRunway->pointOnCenterline(aRunway->lengthM() + (3.0 * SG_NM_TO_METER));
  WayptRef w = new BasicWaypt(p, ss.str(), NULL);
  w->setAltitude(runwayElevFt + 3000.0, RESTRICT_AT);
  wpts.push_back(w);
  
  ss.str("");
  ss << aRunway->ident() << "-6";
  p = aRunway->pointOnCenterline(aRunway->lengthM() + (6.0 * SG_NM_TO_METER));
  w = new BasicWaypt(p, ss.str(), NULL);
  w->setAltitude(runwayElevFt + 6000.0, RESTRICT_AT);
  wpts.push_back(w);

  if (enrouteCourse >= 0.0) {
    // valid enroute course
    int index = 3;
    double course = aRunway->headingDeg();
    double diff;
    while (fabs(diff = headingDiffDeg(course, enrouteCourse)) > 45.0) {
      // turn in the sign of the heading change 45 degrees
      course += copysign(45.0, diff);
      ss.str("");
      ss << "DEP-" << index++;
      SGGeod pos = wpts.back()->position();
      pos = SGGeodesy::direct(pos, course, 3.0 * SG_NM_TO_METER);
      w = new BasicWaypt(pos, ss.str(), NULL);
      wpts.push_back(w);
    }
  } else {
    // no enroute course, just keep runway heading
    ss.str("");
    ss << aRunway->ident() << "-9";
    p = aRunway->pointOnCenterline(aRunway->lengthM() + (9.0 * SG_NM_TO_METER));
    w = new BasicWaypt(p, ss.str(), NULL);
    w->setAltitude(runwayElevFt + 9000.0, RESTRICT_AT);
    wpts.push_back(w);
  }
  
  for (Waypt* w : wpts) {
    w->setFlag(WPT_DEPARTURE);
    w->setFlag(WPT_GENERATED);
  }
  
  return flightgear::SID::createTempSID("DEFAULT", aRunway, wpts);
}

void FGRouteMgr::setSID(const std::string& aIdent)
{
    if (!_plan) {
        return;
    }
    
  FGAirport* apt = _plan->departureAirport();
  if (!apt || aIdent.empty()) {
    _plan->setSID((flightgear::SID*) NULL);
    return;
  } 
  
  if (aIdent == "DEFAULT") {
    double enrouteCourse = -1.0;
    if (_plan->destinationAirport()) {
      enrouteCourse = SGGeodesy::courseDeg(apt->geod(), _plan->destinationAirport()->geod());
    }
    
    _plan->setSID(createDefaultSID(_plan->departureRunway(), enrouteCourse));
    return;
  }
  
  size_t hyphenPos = aIdent.find('-');
  if (hyphenPos != string::npos) {
    string sidIdent = aIdent.substr(0, hyphenPos);
    string transIdent = aIdent.substr(hyphenPos + 1);
    
    flightgear::SID* sid = apt->findSIDWithIdent(sidIdent);
    Transition* trans = sid ? sid->findTransitionByName(transIdent) : NULL;
    _plan->setSID(trans);
  } else {
    _plan->setSID(apt->findSIDWithIdent(aIdent));
  }
}

std::string FGRouteMgr::getDestinationICAO() const
{
  if (!_plan || !_plan->destinationAirport()) {
    return "";
  }
  
  return _plan->destinationAirport()->ident();
}

std::string FGRouteMgr::getDestinationName() const
{
  if (!_plan || !_plan->destinationAirport()) {
    return "";
  }
  
  return _plan->destinationAirport()->name();
}

void FGRouteMgr::setDestinationICAO(const std::string& aIdent)
{
    if (!_plan) {
        return;
    }
    
  if (aIdent.length() < 3) {
    _plan->setDestination((FGAirport*) NULL);
  } else {
    _plan->setDestination(FGAirport::findByIdent(aIdent));
  }
}

std::string FGRouteMgr::getDestinationRunway() const
{
  if (_plan && _plan->destinationRunway()) {
    return _plan->destinationRunway()->ident();
  }
  
  return "";
}

void FGRouteMgr::setDestinationRunway(const std::string& aIdent)
{
    if (!_plan) {
        return;
    }
    
  FGAirport* apt = _plan->destinationAirport();
  if (!apt || aIdent.empty()) {
    _plan->setDestination(apt);
  } else if (apt->hasRunwayWithIdent(aIdent)) {
    _plan->setDestination(apt->getRunwayByIdent(aIdent));
  }
}

std::string FGRouteMgr::getApproach() const
{
  if (_plan && _plan->approach()) {
    return _plan->approach()->ident();
  }
  
  return "";
}

flightgear::Approach* createDefaultApproach(FGRunway* aRunway, double aEnrouteCourse)
{
  if (!aRunway) {
    return NULL;
  }

  double thresholdElevFt = aRunway->threshold().getElevationFt();
  const double approachHeightFt = 2000.0;
  double glideslopeDistanceM = (approachHeightFt * SG_FEET_TO_METER) /
    tan(3.0 * SG_DEGREES_TO_RADIANS);
  
  std::ostringstream ss;
  ss << aRunway->ident() << "-12";
  WayptVec wpts;
  SGGeod p = aRunway->pointOnCenterline(-12.0 * SG_NM_TO_METER);
  WayptRef w = new BasicWaypt(p, ss.str(), NULL);
  w->setAltitude(thresholdElevFt + 4000, RESTRICT_AT);
  wpts.push_back(w);

// work back form the first point on the centerline
  
  if (aEnrouteCourse >= 0.0) {
    // valid enroute course
    int index = 4;
    double course = aRunway->headingDeg();
    double diff;
    while (fabs(diff = headingDiffDeg(aEnrouteCourse, course)) > 45.0) {
      // turn in the sign of the heading change 45 degrees
      course -= copysign(45.0, diff);
      ss.str("");
      ss << "APP-" << index++;
      SGGeod pos = wpts.front()->position();
      pos = SGGeodesy::direct(pos, course + 180.0, 3.0 * SG_NM_TO_METER);
      w = new BasicWaypt(pos, ss.str(), NULL);
      wpts.insert(wpts.begin(), w);
    }
  }
    
  p = aRunway->pointOnCenterline(-8.0 * SG_NM_TO_METER);
  ss.str("");
  ss << aRunway->ident() << "-8";
  w = new BasicWaypt(p, ss.str(), NULL);
  w->setAltitude(thresholdElevFt + approachHeightFt, RESTRICT_AT);
  wpts.push_back(w);
  
  p = aRunway->pointOnCenterline(-glideslopeDistanceM);    
  ss.str("");
  ss << aRunway->ident() << "-GS";
  w = new BasicWaypt(p, ss.str(), NULL);
  w->setAltitude(thresholdElevFt + approachHeightFt, RESTRICT_AT);
  wpts.push_back(w);
    
  for (Waypt* w : wpts) {
    w->setFlag(WPT_APPROACH);
    w->setFlag(WPT_GENERATED);
  }
  
  return Approach::createTempApproach("DEFAULT", aRunway, wpts);
}

void FGRouteMgr::setApproach(const std::string& aIdent)
{
    if (!_plan) {
        return;
    }
    
  FGAirport* apt = _plan->destinationAirport();
  if (aIdent == "DEFAULT") {
    double enrouteCourse = -1.0;
    if (_plan->departureAirport()) {
      enrouteCourse = SGGeodesy::courseDeg(_plan->departureAirport()->geod(), apt->geod());
    }
    
    _plan->setApproach(createDefaultApproach(_plan->destinationRunway(), enrouteCourse));
    return;
  }
  
  if (!apt || aIdent.empty()) {
      _plan->setApproach(static_cast<Approach*>(nullptr));
  } else {
    _plan->setApproach(apt->findApproachWithIdent(aIdent));
  }
}

std::string FGRouteMgr::getSTAR() const
{
  if (_plan && _plan->star()) {
    return _plan->star()->ident();
  }
  
  return "";
}

void FGRouteMgr::setSTAR(const std::string& aIdent)
{
    if (!_plan) {
        return;
    }
    
  FGAirport* apt = _plan->destinationAirport();
  if (!apt || aIdent.empty()) {
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

int FGRouteMgr::getCruiseAltitudeFt() const
{
    if (!_plan)
        return 0;

    return _plan->cruiseAltitudeFt();
}

void FGRouteMgr::setCruiseAltitudeFt(int ft)
{
    if (!_plan)
        return;

    _plan->setCruiseAltitudeFt(ft);
}

int FGRouteMgr::getCruiseFlightLevel() const
{
    if (!_plan)
        return 0;

    return _plan->cruiseFlightLevel();
}

void FGRouteMgr::setCruiseFlightLevel(int fl)
{
    if (!_plan)
        return;

    _plan->setCruiseFlightLevel(fl);
}

int FGRouteMgr::getCruiseSpeedKnots() const
{
    if (!_plan)
        return 0;

    return _plan->cruiseSpeedKnots();
}

void FGRouteMgr::setCruiseSpeedKnots(int kts)
{
    if (!_plan)
        return;

    _plan->setCruiseSpeedKnots(kts);
}

double FGRouteMgr::getCruiseSpeedMach() const
{
    if (!_plan)
        return 0.0;

    return _plan->cruiseSpeedMach();
}

void FGRouteMgr::setCruiseSpeedMach(double m)
{
    if (!_plan)
        return;

    _plan->setCruiseSpeedMach(m);
}

string FGRouteMgr::getAlternate() const
{
    if (!_plan || !_plan->alternate())
        return {};

    return _plan->alternate()->ident();
}

std::string FGRouteMgr::getAlternateName() const
{
    if (!_plan || !_plan->alternate())
        return {};

    return _plan->alternate()->name();
}

void FGRouteMgr::setAlternate(const string &icao)
{
    if (!_plan)
        return;

    _plan->setAlternate(FGAirport::findByIdent(icao));
    alternate->fireValueChanged();
}

SGPropertyNode_ptr FGRouteMgr::wayptNodeAtIndex(int index) const
{
  if ((index < 0) || (index >= numWaypts())) {
    throw sg_range_exception("waypt index out of range", "FGRouteMgr::wayptAtIndex");
  }
  
  return mirror->getChild("wp", index);
}

bool FGRouteMgr::commandDefineUserWaypoint(const SGPropertyNode * arg, SGPropertyNode * root)
{
    std::string ident = arg->getStringValue("ident");
    if (ident.empty()) {
        SG_LOG(SG_AUTOPILOT, SG_WARN, "missing ident defining user waypoint");
        return false;
    }
    
    // check for duplicate idents
    FGPositioned::TypeFilter f(FGPositioned::WAYPOINT);
    FGPositionedList dups = FGPositioned::findAllWithIdent(ident, &f);
    if (!dups.empty()) {
        SG_LOG(SG_AUTOPILOT, SG_WARN, "defineUserWaypoint: non-unique waypoint identifier:" << ident);
        return false;
    }

    SGGeod pos(SGGeod::fromDeg(arg->getDoubleValue("longitude-deg"),
                               arg->getDoubleValue("latitude-deg")));
    FGPositioned::createUserWaypoint(ident, pos);
    return true;
}

bool FGRouteMgr::commandDeleteUserWaypoint(const SGPropertyNode * arg, SGPropertyNode * root)
{
    std::string ident = arg->getStringValue("ident");
    if (ident.empty()) {
        SG_LOG(SG_AUTOPILOT, SG_WARN, "missing ident deleting user waypoint");
        return false;
    }
    
    return FGPositioned::deleteUserWaypoint(ident);
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGRouteMgr> registrantFGRouteMgr(
    SGSubsystemMgr::GENERAL,
    {{"gui", SGSubsystemMgr::Dependency::HARD}});
