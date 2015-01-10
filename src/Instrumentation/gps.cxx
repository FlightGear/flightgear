// gps.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gps.hxx"

#include <boost/tuple/tuple.hpp>

#include <memory>
#include <set>
#include <cstring>
#include <cstdio>

#include "Main/fg_props.hxx"
#include "Main/globals.hxx" // for get_subsystem
#include "Main/util.hxx" // for fgLowPass
#include "Navaids/positioned.hxx"
#include <Navaids/waypoint.hxx>
#include "Navaids/navrecord.hxx"
#include "Navaids/FlightPlan.hxx"
#include <Navaids/routePath.hxx>

#include "Airports/airport.hxx"
#include "Airports/runways.hxx"
#include "Autopilot/route_mgr.hxx"

#include <simgear/math/sg_random.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/exception.hxx>

using std::auto_ptr;
using std::string;
using namespace flightgear;


static const char* makeTTWString(double TTW)
{
  if ((TTW <= 0.0) || (TTW >= 356400.5)) { // 99 hours
    return "--:--:--";
  }
      
  unsigned int TTW_seconds = (int) (TTW + 0.5);
  unsigned int TTW_minutes = 0;
  unsigned int TTW_hours   = 0;
  static char TTW_str[9];
  TTW_hours   = TTW_seconds / 3600;
  TTW_minutes = (TTW_seconds / 60) % 60;
  TTW_seconds = TTW_seconds % 60;
  snprintf(TTW_str, 9, "%02d:%02d:%02d",
    TTW_hours, TTW_minutes, TTW_seconds);
  return TTW_str;
}

////////////////////////////////////////////////////////////////////////////
// configuration helper object

GPS::Config::Config() :
  _enableTurnAnticipation(true),
  _turnRate(3.0), // degrees-per-second, so 180 degree turn takes 60 seconds
  _overflightDistance(0.0),
  _overflightArmDistance(1.0),
  _overflightArmAngle(90.0),
  _waypointAlertTime(30.0),
  _requireHardSurface(true),
  _cdiMaxDeflectionNm(3.0), // linear mode, 3nm at the peg
  _driveAutopilot(true),
  _courseSelectable(false),
  _followLegTrackToFix(true)
{
  _enableTurnAnticipation = false;
}

void GPS::Config::bind(GPS* aOwner, SGPropertyNode* aCfg)
{
  aOwner->tie(aCfg, "turn-rate-deg-sec", SGRawValuePointer<double>(&_turnRate));
  aOwner->tie(aCfg, "turn-anticipation", SGRawValuePointer<bool>(&_enableTurnAnticipation));
  aOwner->tie(aCfg, "wpt-alert-time", SGRawValuePointer<double>(&_waypointAlertTime));
  aOwner->tie(aCfg, "hard-surface-runways-only", SGRawValuePointer<bool>(&_requireHardSurface));
  aOwner->tie(aCfg, "cdi-max-deflection-nm", SGRawValuePointer<double>(&_cdiMaxDeflectionNm));
  aOwner->tie(aCfg, "drive-autopilot", SGRawValuePointer<bool>(&_driveAutopilot));
  aOwner->tie(aCfg, "course-selectable", SGRawValuePointer<bool>(&_courseSelectable));
  aOwner->tie(aCfg, "follow-leg-track-to-fix", SGRawValuePointer<bool>(&_followLegTrackToFix));
  aOwner->tie(aCfg, "over-flight-distance-nm", SGRawValuePointer<double>(&_overflightDistance));
  aOwner->tie(aCfg, "over-flight-arm-distance-nm", SGRawValuePointer<double>(&_overflightArmDistance));
  aOwner->tie(aCfg, "over-flight-arm-angle-deg", SGRawValuePointer<double>(&_overflightArmAngle));
}

////////////////////////////////////////////////////////////////////////////

GPS::GPS ( SGPropertyNode *node, bool defaultGPSMode) :
  _selectedCourse(0.0),
  _desiredCourse(0.0),
  _dataValid(false),
  _lastPosValid(false),
  _defaultGPSMode(defaultGPSMode),
  _mode("init"),
  _name(node->getStringValue("name", "gps")),
  _num(node->getIntValue("number", 0)),
  _computeTurnData(false),
  _anticipateTurn(false),
  _inTurn(false),
  _callbackFlightPlanChanged(SGPropertyChangeCallback<GPS>(this,&GPS::routeManagerFlightPlanChanged,
                                                           fgGetNode("/autopilot/route-manager/signals/flightplan-changed", true))),
  _callbackRouteActivated(SGPropertyChangeCallback<GPS>(this,&GPS::routeActivated,
                                                      fgGetNode("/autopilot/route-manager/active", true)))
{
  string branch = "/instrumentation/" + _name;
  _gpsNode = fgGetNode(branch.c_str(), _num, true );
  _scratchNode = _gpsNode->getChild("scratch", 0, true);
  
  SGPropertyNode *wp_node = _gpsNode->getChild("wp", 0, true);
  _currentWayptNode = wp_node->getChild("wp", 1, true);
    
#if FG_210_COMPAT
    _searchIsRoute = false;
    _searchHasNext = false;
    _searchType = FGPositioned::INVALID;
#endif
}

GPS::~GPS ()
{
}

void
GPS::init ()
{
  _magvar_node = fgGetNode("/environment/magnetic-variation-deg", true);
  _serviceable_node = _gpsNode->getChild("serviceable", 0, true);
  _serviceable_node->setBoolValue(true);
  _electrical_node = fgGetNode("/systems/electrical/outputs/gps", true);

// basic GPS outputs
  _raim_node = _gpsNode->getChild("raim", 0, true);
  _odometer_node = _gpsNode->getChild("odometer", 0, true);
  _trip_odometer_node = _gpsNode->getChild("trip-odometer", 0, true);
  _true_bug_error_node = _gpsNode->getChild("true-bug-error-deg", 0, true);
  _magnetic_bug_error_node = _gpsNode->getChild("magnetic-bug-error-deg", 0, true);
  _eastWestVelocity = _gpsNode->getChild("ew-velocity-msec", 0, true);
  _northSouthVelocity = _gpsNode->getChild("ns-velocity-msec", 0, true);
  
// waypoints
  // for compatibility, alias selected course down to wp/wp[1]/desired-course-deg
  SGPropertyNode* wp1Crs = _currentWayptNode->getChild("desired-course-deg", 0, true);
  wp1Crs->alias(_gpsNode->getChild("desired-course-deg", 0, true));

  _tracking_bug_node = _gpsNode->getChild("tracking-bug", 0, true);
    
// route properties
  // should these move to the route manager?
  _routeDistanceNm = _gpsNode->getChild("route-distance-nm", 0, true);
  _routeETE = _gpsNode->getChild("ETE", 0, true);

    
// navradio slaving properties
  SGPropertyNode* toFlag = _gpsNode->getChild("to-flag", 0, true);
  toFlag->alias(_currentWayptNode->getChild("to-flag"));
  
  SGPropertyNode* fromFlag = _gpsNode->getChild("from-flag", 0, true);
  fromFlag->alias(_currentWayptNode->getChild("from-flag"));
    
// autopilot drive properties
  _apDrivingFlag = fgGetNode("/autopilot/settings/gps-driving-true-heading", true);
  _apTrueHeading = fgGetNode("/autopilot/settings/true-heading-deg",true);

  clearScratch();
  clearOutput();
}

void
GPS::reinit ()
{
  clearOutput();
}

void
GPS::bind()
{
  _config.bind(this, _gpsNode->getChild("config", 0, true));

// basic GPS outputs
  tie(_gpsNode, "selected-course-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getSelectedCourse, &GPS::setSelectedCourse));

  tie(_gpsNode, "desired-course-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getDesiredCourse, NULL));
  _desiredCourseNode = _gpsNode->getChild("desired-course-deg", 0, true);
    
  tieSGGeodReadOnly(_gpsNode, _indicated_pos, "indicated-longitude-deg", 
        "indicated-latitude-deg", "indicated-altitude-ft");

  tie(_gpsNode, "indicated-vertical-speed", SGRawValueMethods<GPS, double>
    (*this, &GPS::getVerticalSpeed, NULL));
  tie(_gpsNode, "indicated-track-true-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getTrueTrack, NULL));
  tie(_gpsNode, "indicated-track-magnetic-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getMagTrack, NULL));
  tie(_gpsNode, "indicated-ground-speed-kt", SGRawValueMethods<GPS, double>
    (*this, &GPS::getGroundspeedKts, NULL));
  
// command system
  tie(_gpsNode, "mode", SGRawValueMethods<GPS, const char*>(*this, &GPS::getMode, NULL));
  tie(_gpsNode, "command", SGRawValueMethods<GPS, const char*>(*this, &GPS::getCommand, &GPS::setCommand));
  tieSGGeod(_scratchNode, _scratchPos, "longitude-deg", "latitude-deg", "altitude-ft");
  
#if FG_210_COMPAT
    tie(_scratchNode, "valid", SGRawValueMethods<GPS, bool>(*this, &GPS::getScratchValid, NULL));
    tie(_scratchNode, "distance-nm", SGRawValueMethods<GPS, double>(*this, &GPS::getScratchDistance, NULL));
    tie(_scratchNode, "true-bearing-deg", SGRawValueMethods<GPS, double>(*this, &GPS::getScratchTrueBearing, NULL));
    tie(_scratchNode, "mag-bearing-deg", SGRawValueMethods<GPS, double>(*this, &GPS::getScratchMagBearing, NULL));
    tie(_scratchNode, "has-next", SGRawValueMethods<GPS, bool>(*this, &GPS::getScratchHasNext, NULL));
    _scratchValid = false;
    
    _scratchNode->setStringValue("type", "");
    _scratchNode->setStringValue("query", "");
#endif
    
  SGPropertyNode *wp_node = _gpsNode->getChild("wp", 0, true);
  SGPropertyNode* wp0_node = wp_node->getChild("wp", 0, true);
  tieSGGeodReadOnly(wp0_node, _wp0_position, "longitude-deg", "latitude-deg", "altitude-ft");
  tie(wp0_node, "ID", SGRawValueMethods<GPS, const char*>
      (*this, &GPS::getWP0Ident, NULL));
  tie(wp0_node, "name", SGRawValueMethods<GPS, const char*>
      (*this, &GPS::getWP0Name, NULL));

  tie(_currentWayptNode, "valid", SGRawValueMethods<GPS, bool>
       (*this, &GPS::getWP1IValid, NULL));
    
  tie(_currentWayptNode, "ID", SGRawValueMethods<GPS, const char*>
    (*this, &GPS::getWP1Ident, NULL));
  tie(_currentWayptNode, "name", SGRawValueMethods<GPS, const char*>
    (*this, &GPS::getWP1Name, NULL));
    
  tie(_currentWayptNode, "distance-nm", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1Distance, NULL));
  tie(_currentWayptNode, "bearing-true-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1Bearing, NULL));
  tie(_currentWayptNode, "bearing-mag-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1MagBearing, NULL));
  tie(_currentWayptNode, "TTW-sec", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1TTW, NULL));
  tie(_currentWayptNode, "TTW", SGRawValueMethods<GPS, const char*>
    (*this, &GPS::getWP1TTWString, NULL));
  
  tie(_currentWayptNode, "course-deviation-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1CourseDeviation, NULL));
  tie(_currentWayptNode, "course-error-nm", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1CourseErrorNm, NULL));
  tie(_currentWayptNode, "to-flag", SGRawValueMethods<GPS, bool>
    (*this, &GPS::getWP1ToFlag, NULL));
  tie(_currentWayptNode, "from-flag", SGRawValueMethods<GPS, bool>
    (*this, &GPS::getWP1FromFlag, NULL));

// leg properties (only valid in DTO/LEG modes, not OBS)
  tie(wp_node, "leg-distance-nm", SGRawValueMethods<GPS, double>(*this, &GPS::getLegDistance, NULL));
  tie(wp_node, "leg-true-course-deg", SGRawValueMethods<GPS, double>(*this, &GPS::getLegCourse, NULL));
  tie(wp_node, "leg-mag-course-deg", SGRawValueMethods<GPS, double>(*this, &GPS::getLegMagCourse, NULL));

// navradio slaving properties  
  tie(_gpsNode, "cdi-deflection", SGRawValueMethods<GPS,double>
    (*this, &GPS::getCDIDeflection));
}

void
GPS::unbind()
{
  _tiedProperties.Untie();
}

void
GPS::clearOutput()
{
  _dataValid = false;
  _last_speed_kts = 0.0;
  _last_pos = SGGeod();
  _lastPosValid = false;
  _indicated_pos = SGGeod();
  _last_vertical_speed = 0.0;
  _last_true_track = 0.0;
  _lastEWVelocity = _lastNSVelocity = 0.0;
  _currentWaypt = _prevWaypt = NULL;
  _legDistanceNm = -1.0;
  
  _raim_node->setDoubleValue(0.0);
  _indicated_pos = SGGeod();
  _odometer_node->setDoubleValue(0);
  _trip_odometer_node->setDoubleValue(0);
  _tracking_bug_node->setDoubleValue(0);
  _true_bug_error_node->setDoubleValue(0);
  _magnetic_bug_error_node->setDoubleValue(0);
  _northSouthVelocity->setDoubleValue(0.0);
  _eastWestVelocity->setDoubleValue(0.0);
}

void
GPS::update (double delta_time_sec)
{
  if (!_defaultGPSMode) {
    // If it's off, don't bother.
      // check if value is defined, since many aircraft don't define an output
      // for the GPS, but expect the default one to work.
      bool elecOn = !_electrical_node->hasValue() || _electrical_node->getBoolValue();
    if (!_serviceable_node->getBoolValue() || !elecOn) {
      clearOutput();
      return;
    }
  }
  
  if (delta_time_sec <= 0.0) {
    return; // paused, don't bother
  }    
  
  _raim_node->setDoubleValue(1.0);
  _indicated_pos = globals->get_aircraft_position();
  updateBasicData(delta_time_sec);

  if (_dataValid) {
    if (_wayptController.get()) {
      _wayptController->update(delta_time_sec);
      SGGeod p(_wayptController->position());
      _currentWayptNode->setDoubleValue("longitude-deg", p.getLongitudeDeg());
      _currentWayptNode->setDoubleValue("latitude-deg", p.getLatitudeDeg());
      _currentWayptNode->setDoubleValue("altitude-ft", p.getElevationFt());
      
      _desiredCourse = getLegMagCourse();
      
      updateTurn();
      updateRouteData();
    }

    
    updateTrackingBug();
    driveAutopilot();
  }
  
  if (_dataValid && (_mode == "init")) {
    // will select LEG mode if the route is active
    routeManagerFlightPlanChanged(NULL);
    
    FGRouteMgr* routeMgr = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
    if (!routeMgr->isRouteActive()) {
      // initialise in OBS mode, with waypt set to the nearest airport.
      // keep in mind at this point, _dataValid is not set
      auto_ptr<FGPositioned::Filter> f(new FGAirport::HardSurfaceFilter());
      FGPositionedRef apt = FGPositioned::findClosest(_indicated_pos, 20.0, f.get());
      if (apt) {
        selectOBSMode(new flightgear::NavaidWaypoint(apt, NULL));
      } else {
          selectOBSMode(NULL);
      }
        
        
    }

    if (_mode != "init")
    {
      // allow a realistic delay in the future, here
    }
  } // of init mode check
  
  _last_pos = _indicated_pos;
  _lastPosValid = !(_last_pos == SGGeod());
}

void GPS::routeManagerFlightPlanChanged(SGPropertyNode*)
{
  if (_route) {
    _route->removeDelegate(this);
  }
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS saw route-manager flight-plan replaced.");
  FGRouteMgr* routeMgr = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
  _route = routeMgr->flightPlan();
  if (_route) {
    _route->addDelegate(this);
  }
  
  if (routeMgr->isRouteActive()) {
    selectLegMode();
  } else {
    selectOBSMode(_currentWaypt); // revert to OBS on current waypoint
  }
}

void GPS::routeActivated(SGPropertyNode* aNode)
{
  bool isActive = aNode->getBoolValue();
  if (isActive) {
    SG_LOG(SG_INSTR, SG_INFO, "GPS::route activated, switching to LEG mode");
    selectLegMode();
    
    // if we've already passed the current waypoint, sequence.
    if (_dataValid && getWP1FromFlag()) {
      SG_LOG(SG_INSTR, SG_INFO, "GPS::route activated, FROM wp1, sequencing");
      sequence();
    }
  } else if (_mode == "leg") {
    SG_LOG(SG_INSTR, SG_INFO, "GPS::route deactivated, switching to OBS mode");
    selectOBSMode(_currentWaypt);
  }
}

///////////////////////////////////////////////////////////////////////////
// implement the RNAV interface 
SGGeod GPS::position()
{
  if (!_dataValid) {
    return SGGeod();
  }
  
  return _indicated_pos;
}

double GPS::trackDeg()
{
  return _last_true_track;
}

double GPS::groundSpeedKts()
{
  return _last_speed_kts;
}

double GPS::vspeedFPM()
{
  return _last_vertical_speed;
}

double GPS::magvarDeg()
{
  return _magvar_node->getDoubleValue();
}

double GPS::overflightDistanceM()
{
  return _config.overflightDistanceNm() * SG_NM_TO_METER;
}

double GPS::overflightArmDistanceM()
{
  return _config.overflightArmDistanceNm() * SG_NM_TO_METER;
}

double GPS::overflightArmAngleDeg()
{
  return _config.overflightArmAngleDeg();
}

double GPS::selectedMagCourse()
{
  return _selectedCourse;
}

SGGeod GPS::previousLegWaypointPosition(bool& isValid)
{
	FlightPlan::Leg* leg = _route->previousLeg();
	if (leg){
		Waypt* waypt = leg->waypoint();
		if (waypt) {
            isValid = true;
            // ensure computations use runway end, not threshold
            if (waypt->type() == "runway") {
                RunwayWaypt* rwpt = static_cast<RunwayWaypt*>(waypt);
                return rwpt->runway()->end();
            }

			return waypt->position();
		}
	}

	isValid = false;
    return SGGeod();
}
///////////////////////////////////////////////////////////////////////////

void
GPS::updateBasicData(double dt)
{
  if (!_lastPosValid) {
    return;
  }
  
  double distance_m;
  double track2_deg;
  SGGeodesy::inverse(_last_pos, _indicated_pos, _last_true_track, track2_deg, distance_m );
    
  double speed_kt = ((distance_m * SG_METER_TO_NM) * ((1 / dt) * 3600.0));
  double vertical_speed_mpm = ((_indicated_pos.getElevationM() - _last_pos.getElevationM()) * 60 / dt);
  _last_vertical_speed = vertical_speed_mpm * SG_METER_TO_FEET;
  
  speed_kt = fgGetLowPass(_last_speed_kts, speed_kt, dt/10.0);
  _last_speed_kts = speed_kt;
  
  SGGeod g = _indicated_pos;
  g.setLongitudeDeg(_last_pos.getLongitudeDeg());
  double northSouthM = dist(SGVec3d::fromGeod(_last_pos), SGVec3d::fromGeod(g));
  northSouthM = copysign(northSouthM, _indicated_pos.getLatitudeDeg() - _last_pos.getLatitudeDeg());
  
  double nsMSec = fgGetLowPass(_lastNSVelocity, northSouthM / dt, dt/2.0);
  _lastNSVelocity = nsMSec;
  _northSouthVelocity->setDoubleValue(nsMSec);

  g = _indicated_pos;
  g.setLatitudeDeg(_last_pos.getLatitudeDeg());
  double eastWestM = dist(SGVec3d::fromGeod(_last_pos), SGVec3d::fromGeod(g));
  eastWestM = copysign(eastWestM, _indicated_pos.getLongitudeDeg() - _last_pos.getLongitudeDeg());
  
  double ewMSec = fgGetLowPass(_lastEWVelocity, eastWestM / dt, dt/2.0);
  _lastEWVelocity = ewMSec;
  _eastWestVelocity->setDoubleValue(ewMSec);
  
  double odometer = _odometer_node->getDoubleValue();
  _odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);
  odometer = _trip_odometer_node->getDoubleValue();
  _trip_odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);
  
  if (!_dataValid) {
    _dataValid = true;
  }
}

void
GPS::updateTrackingBug()
{
  double tracking_bug = _tracking_bug_node->getDoubleValue();
  double true_bug_error = tracking_bug - getTrueTrack();
  double magnetic_bug_error = tracking_bug - getMagTrack();

  // Get the errors into the (-180,180) range.
  SG_NORMALIZE_RANGE(true_bug_error, -180.0, 180.0);
  SG_NORMALIZE_RANGE(magnetic_bug_error, -180.0, 180.0);

  _true_bug_error_node->setDoubleValue(true_bug_error);
  _magnetic_bug_error_node->setDoubleValue(magnetic_bug_error);
}

void GPS::endOfFlightPlan()
{
  selectOBSMode(_currentWaypt);
}

void GPS::currentWaypointChanged()
{
  if (!_route) {
    return;
  }
  
  int index = _route->currentIndex(),
    count = _route->numLegs();
  if ((index < 0) || (index >= count)) {
    _currentWaypt=NULL;
    _prevWaypt=NULL;
    // no active leg on the route
    return;
  }
    
  if (index > 0) {
    _prevWaypt = _route->previousLeg()->waypoint();
    if (_prevWaypt->flag(WPT_DYNAMIC)) {
      _wp0_position = _indicated_pos;
    } else {
      _wp0_position = _prevWaypt->position();
    }
  }
  
  _currentWaypt = _route->currentLeg()->waypoint();
    wp1Changed(); // rebuild the active controller
  _desiredCourse = getLegMagCourse();
  _desiredCourseNode->fireValueChanged();
}


void GPS::waypointsChanged()
{
  if (_mode != "leg") {
    return;
  }
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS route edited while in LEG mode, updating waypoints");
  currentWaypointChanged();
}

void GPS::cleared()
{
    if (_mode != "leg") {
        return;
    }
    
    selectOBSMode(_currentWaypt);
}

void GPS::sequence()
{
    if (!_route) {
        return;
    }
  
    int nextIndex = _route->currentIndex() + 1;
    if (nextIndex >= _route->numLegs()) {
        SG_LOG(SG_INSTR, SG_INFO, "sequenced final leg, end of route");
        _route->finish();
        selectOBSMode(_currentWaypt);
        return;
    }
    
    // will callback into currentWaypointChanged
    _route->setCurrentIndex(nextIndex);
}

void GPS::updateTurn()
{
  bool printProgress = false;
  
  if (_computeTurnData) {
    if (_last_speed_kts < 10) {
      // need valid leg course and sensible ground speed to compute the turn
      return;
    }
    
    computeTurnData();
    printProgress = true;
  }
  
  if (!_anticipateTurn) {
    updateOverflight();
    return;
  }

  updateTurnData();
  // find bearing to turn centre
  double bearing, az2, distanceM;
  SGGeodesy::inverse(_indicated_pos, _turnCentre, bearing, az2, distanceM);
  double progress = computeTurnProgress(bearing);
  
  if (printProgress) {
    SG_LOG(SG_INSTR, SG_INFO,"turn progress=" << progress);
  }
  
  if (!_inTurn && (progress > 0.0)) {
    beginTurn();
  }
  
  if (_inTurn && !_turnSequenced && (progress > 0.5)) {
    _turnSequenced = true;
     SG_LOG(SG_INSTR, SG_INFO, "turn passed midpoint, sequencing");
     sequence();
  }
  
  if (_inTurn && (progress >= 1.0)) {
    endTurn();
  }
  
  if (_inTurn) {
    // drive deviation and desired course
    double desiredCourse = bearing - copysign(90, _turnAngle);
    SG_NORMALIZE_RANGE(desiredCourse, 0.0, 360.0);
    double deviationNm = (distanceM * SG_METER_TO_NM) - _turnRadius;
    double deviationDeg = desiredCourse - getMagTrack();
    deviationNm = copysign(deviationNm, deviationDeg);
    // FIXME
    //_wp1_course_deviation_node->setDoubleValue(deviationDeg);
    //_wp1_course_error_nm_node->setDoubleValue(deviationNm);
    //_cdiDeflectionNode->setDoubleValue(deviationDeg);
  }
}

void GPS::updateOverflight()
{
  if (!_wayptController->isDone()) {
    return;
  }
  
  if (_mode == "dto") {
    SG_LOG(SG_INSTR, SG_INFO, "GPS DTO reached destination point");
    
    // check for wp1 being on active route - resume leg mode
    if (_route) {
      int index = _route->findWayptIndex(_currentWaypt->position());
      if (index >= 0) {
        SG_LOG(SG_INSTR, SG_INFO, "GPS DTO, resuming LEG mode at wp:" << index);
        _mode = "leg";
        _route->setCurrentIndex(index);
        sequence(); // and sequence to the next point
      }
    }
    
    if (_mode == "dto") {
      // if we didn't enter leg mode, drop back to OBS mode
      // select OBS mode, but keep current waypoint-as is
      _mode = "obs";
      wp1Changed();
    }
  } else if (_mode == "leg") {
    SG_LOG(SG_INSTR, SG_DEBUG, "GPS doing overflight sequencing");
    sequence();
  } else if (_mode == "obs") {
    // nothing to do here, TO/FROM will update but that's fine
  }
  _computeTurnData = true;
}

void GPS::beginTurn()
{
  _inTurn = true;
  _turnSequenced = false;
  SG_LOG(SG_INSTR, SG_INFO, "beginning turn");
}

void GPS::endTurn()
{
  _inTurn = false;
  SG_LOG(SG_INSTR, SG_INFO, "ending turn");
  _computeTurnData = true;
}

double GPS::computeTurnProgress(double aBearing) const
{
  double startBearing = _turnStartBearing + copysign(90, _turnAngle);
  return (aBearing - startBearing) / _turnAngle;
}

void GPS::computeTurnData()
{
  _computeTurnData = false;
  if ((_mode != "leg") || !_route->nextLeg()) {
    _anticipateTurn = false;
    return;
  }
  
  WayptRef next = _route->nextLeg()->waypoint();
  if (next->flag(WPT_DYNAMIC) ||
      !_config.turnAnticipationEnabled() ||
      next->flag(WPT_OVERFLIGHT))
  {
    _anticipateTurn = false;
    return;
  }
  
  _turnStartBearing = _desiredCourse;
// compute next leg course
  RoutePath path(_route);
  double crs = path.trackForIndex(_route->currentIndex() + 1);

// compute offset bearing
  _turnAngle = crs - _turnStartBearing;
  SG_NORMALIZE_RANGE(_turnAngle, -180.0, 180.0);
  double median = _turnStartBearing + (_turnAngle * 0.5);
  double offsetBearing = median + copysign(90, _turnAngle);
  SG_NORMALIZE_RANGE(offsetBearing, 0.0, 360.0);
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS computeTurnData: in=" << _turnStartBearing <<
    ", out=" << crs << "; turnAngle=" << _turnAngle << ", median=" << median 
    << ", offset=" << offsetBearing);

  SG_LOG(SG_INSTR, SG_INFO, "next leg is now:" << _currentWaypt->ident() << "->" << next->ident());

  _turnPt = _currentWaypt->position();
  _anticipateTurn = true;
}

void GPS::updateTurnData()
{
  // depends on ground speed, so needs to be updated per-frame
  _turnRadius = computeTurnRadiusNm(_last_speed_kts);
  
  // compute the turn centre, based on the turn radius.
  // key thing is to understand that we're working a right-angle triangle,
  // where the right-angle is the point we start the turn. From that point,
  // one side is the inbound course to the turn pt, and the other is the
  // perpendicular line, of length 'r', to the turn centre.
  // the triangle's hypotenuse, which we need to find, is the distance from the
  // turn pt to the turn center (in the direction of the offset bearing)
  // note that d - _turnRadius tell us how much we're 'cutting' the corner.
  
  double halfTurnAngle = fabs(_turnAngle * 0.5) * SG_DEGREES_TO_RADIANS;
  double d = _turnRadius / cos(halfTurnAngle);
  
 // SG_LOG(SG_INSTR, SG_INFO, "turnRadius=" << _turnRadius << ", d=" << d
 //   << " (cut distance=" << d - _turnRadius << ")");
  
  double median = _turnStartBearing + (_turnAngle * 0.5);
  double offsetBearing = median + copysign(90, _turnAngle);
  SG_NORMALIZE_RANGE(offsetBearing, 0.0, 360.0);
  
  double az2;
  SGGeodesy::direct(_turnPt, offsetBearing, d * SG_NM_TO_METER, _turnCentre, az2); 
}

double GPS::computeTurnRadiusNm(double aGroundSpeedKts) const
{
	// turn time is seconds to execute a 360 turn. 
  double turnTime = 360.0 / _config.turnRateDegSec();
  
  // c is ground distance covered in that time (circumference of the circle)
	double c = turnTime * (aGroundSpeedKts / 3600.0); // convert knts to nm/sec
  
  // divide by 2PI to go from circumference -> radius
	return c / (2 * M_PI);
}

void GPS::updateRouteData()
{
  double totalDistance = _wayptController->distanceToWayptM() * SG_METER_TO_NM;
  // walk all waypoints from wp2 to route end, and sum
  for (int i=_route->currentIndex()+1; i<_route->numLegs(); ++i) {
    FlightPlan::Leg* leg = _route->legAtIndex(i);
    // omit missed-approach waypoints in distance calculation
    if (leg->waypoint()->flag(WPT_MISS))
      continue;
      
    totalDistance += leg->distanceNm();
  }
  
  _routeDistanceNm->setDoubleValue(totalDistance * SG_METER_TO_NM);
  if (_last_speed_kts > 1.0) {
    double TTW = ((totalDistance * SG_METER_TO_NM) / _last_speed_kts) * 3600.0;
    _routeETE->setStringValue(makeTTWString(TTW));    
  }
}

void GPS::driveAutopilot()
{
  if (!_config.driveAutopilot() || !_defaultGPSMode) {
    _apDrivingFlag->setBoolValue(false);
    return;
  }
 
  // compatibility feature - allow the route-manager / GPS to drive the
  // generic autopilot heading hold *in leg mode only* 
  
  bool drive = _mode == "leg";
  _apDrivingFlag->setBoolValue(drive);
  
  if (drive) {
    // FIXME: we want to set desired track, not heading, here
    _apTrueHeading->setDoubleValue(getWP1Bearing());
  }
}

void GPS::wp1Changed()
{
  if (!_currentWaypt)
    return;
  if (_mode == "leg") {
    _wayptController.reset(WayptController::createForWaypt(this, _currentWaypt));
  } else if (_mode == "obs") {
    _wayptController.reset(new OBSController(this, _currentWaypt));
  } else if (_mode == "dto") {
    _wayptController.reset(new DirectToController(this, _currentWaypt, _wp0_position));
  }

  _wayptController->init();

  if (_mode == "obs") {
    _legDistanceNm = -1.0;
  } else {
    _wayptController->update(0.0);
    _legDistanceNm = _wayptController->distanceToWayptM() * SG_METER_TO_NM;
  }
}

/////////////////////////////////////////////////////////////////////////////
// property getter/setters

void GPS::setSelectedCourse(double crs)
{
  if (_selectedCourse == crs) {
    return;
  }
  
  _selectedCourse = crs;
  if ((_mode == "obs") || _config.courseSelectable()) {
    _desiredCourse = _selectedCourse;
    _desiredCourseNode->fireValueChanged();
  }
}

double GPS::getLegDistance() const
{
  if (!_dataValid || (_mode == "obs")) {
    return -1;
  }
  
  return _legDistanceNm;
}

double GPS::getLegCourse() const
{
  if (!_dataValid || !_wayptController.get()) {
    return -9999.0;
  }
  
  return _wayptController->targetTrackDeg();
}

double GPS::getLegMagCourse() const
{
  if (!_dataValid) {
    return 0.0;
  }
  
  double m = getLegCourse() - _magvar_node->getDoubleValue();
  SG_NORMALIZE_RANGE(m, 0.0, 360.0);
  return m;
}

double GPS::getMagTrack() const
{
  if (!_dataValid) {
    return 0.0;
  }
  
  double m = getTrueTrack() - _magvar_node->getDoubleValue();
  SG_NORMALIZE_RANGE(m, 0.0, 360.0);
  return m;
}

double GPS::getCDIDeflection() const
{
  if (!_dataValid) {
    return 0.0;
  }
  
  double defl;
  if (_config.cdiDeflectionIsAngular()) {
    defl = getWP1CourseDeviation();
    SG_CLAMP_RANGE(defl, -10.0, 10.0); // as in navradio.cxx
  } else {
    double fullScale = _config.cdiDeflectionLinearPeg();
    double normError = getWP1CourseErrorNm() / fullScale;
    SG_CLAMP_RANGE(normError, -1.0, 1.0);
    defl = normError * 10.0; // re-scale to navradio limits, i.e [-10.0 .. 10.0]
  }
  
  return defl;
}

const char* GPS::getWP0Ident() const
{
  if (!_dataValid || (_mode != "leg") || !_prevWaypt) {
    return "";
  }
  
// work around std::string::c_str() storage lifetime with libc++
// real fix is to allow tie-ing with std::string instead of char*
  static char identBuf[8];
  strncpy(identBuf, _prevWaypt->ident().c_str(), 8);

  return identBuf;
}

const char* GPS::getWP0Name() const
{
    if (!_dataValid || !_prevWaypt || !_prevWaypt->source()) {
        return "";
    }
    
    return _prevWaypt->source()->name().c_str();
}

bool GPS::getWP1IValid() const
{
    return _dataValid && _currentWaypt.get();
}

const char* GPS::getWP1Ident() const
{
  if (!_dataValid || !_currentWaypt) {
    return "";
  }
  
// work around std::string::c_str() storage lifetime with libc++
// real fix is to allow tie-ing with std::string instead of char*
  static char identBuf[8];
  strncpy(identBuf, _currentWaypt->ident().c_str(), 8);
    
  return identBuf;
}

const char* GPS::getWP1Name() const
{
    if (!_dataValid || !_currentWaypt || !_currentWaypt->source()) {
        return "";
    }
    
    return _currentWaypt->source()->name().c_str();
}

double GPS::getWP1Distance() const
{
  if (!_dataValid || !_wayptController.get()) {
    return -1.0;
  }
  
  return _wayptController->distanceToWayptM() * SG_METER_TO_NM;
}

double GPS::getWP1TTW() const
{
  if (!_dataValid || !_wayptController.get()) {
    return -1.0;
  }
  
  return _wayptController->timeToWaypt();
}

const char* GPS::getWP1TTWString() const
{
  double t = getWP1TTW();
  if (t <= 0.0) {
    return "";
  }
  
  return makeTTWString(t);
}

double GPS::getWP1Bearing() const
{
  if (!_dataValid || !_wayptController.get()) {
    return -9999.0;
  }
  
  return _wayptController->trueBearingDeg();
}

double GPS::getWP1MagBearing() const
{
  if (!_dataValid || !_wayptController.get()) {
    return -9999.0;
  }

  double magBearing =  _wayptController->trueBearingDeg() - _magvar_node->getDoubleValue();
  SG_NORMALIZE_RANGE(magBearing, 0.0, 360.0);
  return magBearing;
}

double GPS::getWP1CourseDeviation() const
{
  if (!_dataValid || !_wayptController.get()) {
    return 0.0;
  }

  return _wayptController->courseDeviationDeg();
}

double GPS::getWP1CourseErrorNm() const
{
  if (!_dataValid || !_wayptController.get()) {
    return 0.0;
  }
  
  return _wayptController->xtrackErrorNm();
}

bool GPS::getWP1ToFlag() const
{
  if (!_dataValid || !_wayptController.get()) {
    return false;
  }
  
  return _wayptController->toFlag();
}

bool GPS::getWP1FromFlag() const
{
  if (!_dataValid || !_wayptController.get()) {
    return false;
  }
  
  return !getWP1ToFlag();
}

#if FG_210_COMPAT
double GPS::getScratchDistance() const
{
    if (!_scratchValid) {
        return 0.0;
    }
    
    return SGGeodesy::distanceNm(_indicated_pos, _scratchPos);
}

double GPS::getScratchTrueBearing() const
{
    if (!_scratchValid) {
        return 0.0;
    }
    
    return SGGeodesy::courseDeg(_indicated_pos, _scratchPos);
}

double GPS::getScratchMagBearing() const
{
    if (!_scratchValid) {
        return 0.0;
    }
    
    double crs = getScratchTrueBearing() - _magvar_node->getDoubleValue();
    SG_NORMALIZE_RANGE(crs, 0.0, 360.0);
    return crs;
}

#endif

/////////////////////////////////////////////////////////////////////////////
// scratch system

void GPS::setCommand(const char* aCmd)
{
  SG_LOG(SG_INSTR, SG_DEBUG, "GPS command:" << aCmd);
  
  if (!strcmp(aCmd, "direct")) {
    directTo();
  } else if (!strcmp(aCmd, "obs")) {
    selectOBSMode(NULL);
  } else if (!strcmp(aCmd, "leg")) {
    selectLegMode();
#if FG_210_COMPAT
  } else if (!strcmp(aCmd, "load-route-wpt")) {
      loadRouteWaypoint();
  } else if (!strcmp(aCmd, "nearest")) {
      loadNearest();
  } else if (!strcmp(aCmd, "search")) {
      _searchNames = false;
      search();
  } else if (!strcmp(aCmd, "search-names")) {
      _searchNames = true;
      search();
  } else if (!strcmp(aCmd, "next")) {
      nextResult();
  } else if (!strcmp(aCmd, "previous")) {
      previousResult();
  } else if (!strcmp(aCmd, "define-user-wpt")) {
      defineWaypoint();
  } else if (!strcmp(aCmd, "route-insert-before")) {
      int index = _scratchNode->getIntValue("index");
      if (index < 0 || (_route->numLegs() == 0)) {
          index = _route->numLegs();
      } else if (index >= _route->numLegs()) {
          SG_LOG(SG_INSTR, SG_WARN, "GPS:route-insert-before, bad index:" << index);
          return;
      }
      
      insertWaypointAtIndex(index);
  } else if (!strcmp(aCmd, "route-insert-after")) {
      int index = _scratchNode->getIntValue("index");
      if (index < 0 || (_route->numLegs() == 0)) {
          index = _route->numLegs();
      } else if (index >= _route->numLegs()) {
          SG_LOG(SG_INSTR, SG_WARN, "GPS:route-insert-after, bad index:" << index);
          return;
      } else {
          ++index;
      }
      
      insertWaypointAtIndex(index);
  } else if (!strcmp(aCmd, "route-delete")) {
      int index = _scratchNode->getIntValue("index");
      if (index < 0) {
          index = _route->numLegs();
      } else if (index >= _route->numLegs()) {
          SG_LOG(SG_INSTR, SG_WARN, "GPS:route-delete, bad index:" << index);
          return;
      }
      
      removeWaypointAtIndex(index);
#endif
  } else {
    SG_LOG(SG_INSTR, SG_WARN, "GPS:unrecognized command:" << aCmd);
  }
}

void GPS::clearScratch()
{
  _scratchPos = SGGeod::fromDegFt(-9999.0, -9999.0, -9999.0);
  _scratchNode->setBoolValue("valid", false);
#if FG_210_COMPAT
    _scratchNode->setStringValue("type", "");
    _scratchNode->setStringValue("query", "");
#endif
}

bool GPS::isScratchPositionValid() const
{
  if ((_scratchPos.getLongitudeDeg() < -9990.0) ||
      (_scratchPos.getLatitudeDeg() < -9990.0)) {
   return false;   
  }
  
  return true;
}

FGPositionedRef GPS::positionedFromScratch() const
{
    if (!isScratchPositionValid()) {
        return NULL;
    }
    
    std::string ident(_scratchNode->getStringValue("ident"));
    return FGPositioned::findClosestWithIdent(ident, _scratchPos);
}

void GPS::directTo()
{  
  if (!isScratchPositionValid()) {
    return;
  }
  
  _prevWaypt = NULL;
  _wp0_position = _indicated_pos;
    
    FGPositionedRef pos = positionedFromScratch();
    if (pos) {
        _currentWaypt = new NavaidWaypoint(pos, NULL);
    } else {
        _currentWaypt = new BasicWaypt(_scratchPos, _scratchNode->getStringValue("ident"), NULL);
    }
    
    _mode = "dto";
  wp1Changed();
}

void GPS::selectOBSMode(flightgear::Waypt* waypt)
{
  if (!waypt && isScratchPositionValid()) {
    FGPositionedRef pos = positionedFromScratch();
    if (pos) {
        waypt = new NavaidWaypoint(pos, NULL);
    } else {
        waypt = new BasicWaypt(_scratchPos, _scratchNode->getStringValue("ident"), NULL);
    }
  }
  
  _mode = "obs";
  _prevWaypt = NULL;
  _wp0_position = _indicated_pos;
  _currentWaypt = waypt;
  wp1Changed();
}

void GPS::selectLegMode()
{
  if (_mode == "leg") {
    return;
  }
  
  if (!_route) {
    SG_LOG(SG_INSTR, SG_WARN, "GPS:selectLegMode: no active route");
    return;
  }

  _mode = "leg";  

  // depending on the situation, this will either get over-written 
  // in routeManagerSequenced or not; either way it does no harm to
  // set it here.
  _wp0_position = _indicated_pos;

  // not really sequenced, but does all the work of updating wp0/1
  currentWaypointChanged();
}

#if FG_210_COMPAT

void GPS::loadRouteWaypoint()
{
    _scratchValid = false;    
    int index = _scratchNode->getIntValue("index", -9999);
    clearScratch();
    
    if ((index < 0) || (index >= _route->numLegs())) { // no index supplied, use current wp
        index = _route->currentIndex();
    }
    
    _searchIsRoute = true;
    setScratchFromRouteWaypoint(index);
}

void GPS::setScratchFromRouteWaypoint(int aIndex)
{
    assert(_searchIsRoute);
    if ((aIndex < 0) || (aIndex >= _route->numLegs())) {
        SG_LOG(SG_INSTR, SG_WARN, "GPS:setScratchFromRouteWaypoint: route-index out of bounds");
        return;
    }
    
    _searchResultIndex = aIndex;
    WayptRef wp = _route->legAtIndex(aIndex)->waypoint();
    _scratchNode->setStringValue("ident", wp->ident());
    _scratchPos = wp->position();
    _scratchValid = true;
    _scratchNode->setIntValue("index", aIndex);
}

void GPS::loadNearest()
{
    string sty(_scratchNode->getStringValue("type"));
    FGPositioned::Type ty = FGPositioned::typeFromName(sty);
    if (ty == FGPositioned::INVALID) {
        SG_LOG(SG_INSTR, SG_WARN, "GPS:loadNearest: request type is invalid:" << sty);
        return;
    }
    
    auto_ptr<FGPositioned::Filter> f(createFilter(ty));
    int limitCount = _scratchNode->getIntValue("max-results", 1);
    double cutoffDistance = _scratchNode->getDoubleValue("cutoff-nm", 400.0);
    
    SGGeod searchPos = _indicated_pos;
    if (isScratchPositionValid()) {
        searchPos = _scratchPos;
    }
    
    clearScratch(); // clear now, regardless of whether we find a match or not
    
    _searchResults =
    FGPositioned::findClosestN(searchPos, limitCount, cutoffDistance, f.get());
    _searchResultIndex = 0;
    _searchIsRoute = false;
    
    if (_searchResults.empty()) {
        return;
    }
    
    setScratchFromCachedSearchResult();
}

bool GPS::SearchFilter::pass(FGPositioned* aPos) const
{
    switch (aPos->type()) {
        case FGPositioned::AIRPORT:
            // heliport and seaport too?
        case FGPositioned::VOR:
        case FGPositioned::NDB:
        case FGPositioned::FIX:
        case FGPositioned::TACAN:
        case FGPositioned::WAYPOINT:
            return true;
        default:
            return false;
    }
}

FGPositioned::Type GPS::SearchFilter::minType() const
{
    return FGPositioned::AIRPORT;
}

FGPositioned::Type GPS::SearchFilter::maxType() const
{
    return FGPositioned::VOR;
}

FGPositioned::Filter* GPS::createFilter(FGPositioned::Type aTy)
{
    if (aTy == FGPositioned::AIRPORT) {
        return new FGAirport::HardSurfaceFilter();
    }
    
    // if we were passed INVALID, assume it means 'all types interesting to a GPS'
    if (aTy == FGPositioned::INVALID) {
        return new SearchFilter;
    }
    
    return new FGPositioned::TypeFilter(aTy);
}

void GPS::search()
{
    // parse search terms into local members, and exec the first search
    string sty(_scratchNode->getStringValue("type"));
    _searchType = FGPositioned::typeFromName(sty);
    _searchQuery = _scratchNode->getStringValue("query");
    if (_searchQuery.empty()) {
        SG_LOG(SG_INSTR, SG_WARN, "empty GPS search query");
        clearScratch();
        return;
    }
    
    _searchExact = _scratchNode->getBoolValue("exact", true);
    _searchResultIndex = 0;
    _searchIsRoute = false;
    
    auto_ptr<FGPositioned::Filter> f(createFilter(_searchType));
    if (_searchNames) {
        _searchResults = FGPositioned::findAllWithName(_searchQuery, f.get(), _searchExact);
    } else {
        _searchResults = FGPositioned::findAllWithIdent(_searchQuery, f.get(), _searchExact);
    }
    
    bool orderByRange = _scratchNode->getBoolValue("order-by-distance", true);
    if (orderByRange) {
        FGPositioned::sortByRange(_searchResults, _indicated_pos);
    }
    
    if (_searchResults.empty()) {
        clearScratch();
        return;
    }
    
    setScratchFromCachedSearchResult();
}

bool GPS::getScratchHasNext() const
{
    int lastResult;
    if (_searchIsRoute) {
        lastResult = _route->numLegs() - 1;
    } else {
        lastResult = (int) _searchResults.size() - 1;
    }
    
    if (lastResult < 0) { // search array might be empty
        return false;
    }
    
    return (_searchResultIndex < lastResult);
}

void GPS::setScratchFromCachedSearchResult()
{
    int index = _searchResultIndex;
    
    if ((index < 0) || (index >= (int) _searchResults.size())) {
        SG_LOG(SG_INSTR, SG_WARN, "GPS:setScratchFromCachedSearchResult: index out of bounds:" << index);
        return;
    }
    
    setScratchFromPositioned(_searchResults[index], index);
}

void GPS::setScratchFromPositioned(FGPositioned* aPos, int aIndex)
{
    clearScratch();
    assert(aPos);
    
    _scratchPos = aPos->geod();
    _scratchNode->setStringValue("name", aPos->name());
    _scratchNode->setStringValue("ident", aPos->ident());
    _scratchNode->setStringValue("type", FGPositioned::nameForType(aPos->type()));
    
    if (aIndex >= 0) {
        _scratchNode->setIntValue("index", aIndex);
    }
    
    _scratchValid = true;
    _scratchNode->setIntValue("result-count", _searchResults.size());
    
    switch (aPos->type()) {
        case FGPositioned::VOR:
            _scratchNode->setDoubleValue("frequency-mhz", static_cast<FGNavRecord*>(aPos)->get_freq() / 100.0);
            break;
            
        case FGPositioned::NDB:
            _scratchNode->setDoubleValue("frequency-khz", static_cast<FGNavRecord*>(aPos)->get_freq() / 100.0);
            break;
            
        case FGPositioned::AIRPORT:
            addAirportToScratch((FGAirport*)aPos);
            break;
            
        default:
            // no-op
            break;
    }
    
    // look for being on the route and set?
}

void GPS::addAirportToScratch(FGAirport* aAirport)
{
    for (unsigned int r=0; r<aAirport->numRunways(); ++r) {
        SGPropertyNode* rwyNd = _scratchNode->getChild("runways", r, true);
        FGRunway* rwy = aAirport->getRunwayByIndex(r);
        // TODO - filter out unsuitable runways in the future
        // based on config again
        
        rwyNd->setStringValue("id", rwy->ident().c_str());
        rwyNd->setIntValue("length-ft", rwy->lengthFt());
        rwyNd->setIntValue("width-ft", rwy->widthFt());
        rwyNd->setIntValue("heading-deg", rwy->headingDeg());
        // map surface code to a string
        // TODO - lighting information
        
        if (rwy->ILS()) {
            rwyNd->setDoubleValue("ils-frequency-mhz", rwy->ILS()->get_freq() / 100.0);
        }
    } // of runways iteration
}

void GPS::nextResult()
{
    if (!getScratchHasNext()) {
        return;
    }
    
    clearScratch();
    if (_searchIsRoute) {
        setScratchFromRouteWaypoint(++_searchResultIndex);
    } else {
        ++_searchResultIndex;
        setScratchFromCachedSearchResult();
    }
}

void GPS::previousResult()
{
    if (_searchResultIndex <= 0) {
        return;
    }
    
    clearScratch();
    --_searchResultIndex;
    
    if (_searchIsRoute) {
        setScratchFromRouteWaypoint(_searchResultIndex);
    } else {
        setScratchFromCachedSearchResult();
    }
}

void GPS::defineWaypoint()
{
    if (!isScratchPositionValid()) {
        SG_LOG(SG_INSTR, SG_WARN, "GPS:defineWaypoint: invalid lat/lon");
        return;
    }
    
    string ident = _scratchNode->getStringValue("ident");
    if (ident.size() < 2) {
        SG_LOG(SG_INSTR, SG_WARN, "GPS:defineWaypoint: waypoint identifier must be at least two characters");
        return;
    }
    
    // check for duplicate idents
    FGPositioned::TypeFilter f(FGPositioned::WAYPOINT);
    FGPositionedList dups = FGPositioned::findAllWithIdent(ident, &f);
    if (!dups.empty()) {
        SG_LOG(SG_INSTR, SG_WARN, "GPS:defineWaypoint: non-unique waypoint identifier, ho-hum");
    }
    
    SG_LOG(SG_INSTR, SG_INFO, "GPS:defineWaypoint: creating waypoint:" << ident);
    FGPositionedRef wpt = FGPositioned::createUserWaypoint(ident, _scratchPos);
    _searchResults.clear();
    _searchResults.push_back(wpt);
    setScratchFromPositioned(wpt.get(), -1);
}

void GPS::insertWaypointAtIndex(int aIndex)
{
    // note we do allow index = routeMgr->size(), that's an append
    if ((aIndex < 0) || (aIndex > _route->numLegs())) {
        throw sg_range_exception("GPS::insertWaypointAtIndex: index out of bounds");
    }
    
    if (!isScratchPositionValid()) {
        SG_LOG(SG_INSTR, SG_WARN, "GPS:insertWaypointAtIndex: invalid lat/lon");
        return;
    }
    
    string ident = _scratchNode->getStringValue("ident");
    
    WayptRef wpt = new BasicWaypt(_scratchPos, ident, NULL);
    _route->insertWayptAtIndex(wpt, aIndex);
}

void GPS::removeWaypointAtIndex(int aIndex)
{
    if ((aIndex < 0) || (aIndex >= _route->numLegs())) {
        throw sg_range_exception("GPS::removeWaypointAtIndex: index out of bounds");
    }
    
    _route->deleteIndex(aIndex);
}


#endif // of FG_210_COMPAT

void GPS::tieSGGeod(SGPropertyNode* aNode, SGGeod& aRef, 
  const char* lonStr, const char* latStr, const char* altStr)
{
  tie(aNode, lonStr, SGRawValueMethods<SGGeod, double>(aRef, &SGGeod::getLongitudeDeg, &SGGeod::setLongitudeDeg));
  tie(aNode, latStr, SGRawValueMethods<SGGeod, double>(aRef, &SGGeod::getLatitudeDeg, &SGGeod::setLatitudeDeg));
  
  if (altStr) {
    tie(aNode, altStr, SGRawValueMethods<SGGeod, double>(aRef, &SGGeod::getElevationFt, &SGGeod::setElevationFt));
  }
}

void GPS::tieSGGeodReadOnly(SGPropertyNode* aNode, SGGeod& aRef, 
  const char* lonStr, const char* latStr, const char* altStr)
{
  tie(aNode, lonStr, SGRawValueMethods<SGGeod, double>(aRef, &SGGeod::getLongitudeDeg, NULL));
  tie(aNode, latStr, SGRawValueMethods<SGGeod, double>(aRef, &SGGeod::getLatitudeDeg, NULL));
  
  if (altStr) {
    tie(aNode, altStr, SGRawValueMethods<SGGeod, double>(aRef, &SGGeod::getElevationFt, NULL));
  }
}

// end of gps.cxx
