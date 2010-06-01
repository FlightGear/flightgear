// gps.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gps.hxx"

#include <memory>
#include <set>

#include "Main/fg_props.hxx"
#include "Main/globals.hxx" // for get_subsystem
#include "Main/util.hxx" // for fgLowPass
#include "Navaids/positioned.hxx"
#include "Navaids/navrecord.hxx"
#include "Airports/simple.hxx"
#include "Airports/runways.hxx"
#include "Autopilot/route_mgr.hxx"

#include <simgear/math/sg_random.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/exception.hxx>

using std::auto_ptr;
using std::string;

///////////////////////////////////////////////////////////////////

void SGGeodProperty::init(SGPropertyNode* base, const char* lonStr, const char* latStr, const char* altStr)
{
    _lon = base->getChild(lonStr, 0, true);
    _lat = base->getChild(latStr, 0, true);
    if (altStr) {
        _alt = base->getChild(altStr, 0, true);
    }
}

void SGGeodProperty::init(const char* lonStr, const char* latStr, const char* altStr)
{
    _lon = fgGetNode(lonStr, true);
    _lat = fgGetNode(latStr, true);
    if (altStr) {
        _alt = fgGetNode(altStr, true);
    }
}

void SGGeodProperty::clear()
{
    _lon = _lat = _alt = NULL;
}

void SGGeodProperty::operator=(const SGGeod& geod)
{
    _lon->setDoubleValue(geod.getLongitudeDeg());
    _lat->setDoubleValue(geod.getLatitudeDeg());
    if (_alt) {
        _alt->setDoubleValue(geod.getElevationFt());
    }
}

SGGeod SGGeodProperty::get() const
{
    double lon = _lon->getDoubleValue(),
        lat = _lat->getDoubleValue();
    if (_alt) {
        return SGGeod::fromDegFt(lon, lat, _alt->getDoubleValue());
    } else {
        return SGGeod::fromDeg(lon,lat);
    }
}

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

/////////////////////////////////////////////////////////////////////////////

class GPSListener : public SGPropertyChangeListener
{
public:
  GPSListener(GPS *m) : 
    _gps(m),
    _guard(false) {}
    
  virtual void valueChanged (SGPropertyNode * prop)
  {
    if (_guard) {
      return;
    }
    
    _guard = true;
    if (prop == _gps->_route_current_wp_node) {
      _gps->routeManagerSequenced();
    } else if (prop == _gps->_route_active_node) {
      _gps->routeActivated();
    } else if (prop == _gps->_ref_navaid_id_node) {
      _gps->referenceNavaidSet(prop->getStringValue(""));
    } else if (prop == _gps->_routeEditedSignal) {
      _gps->routeEdited();
    } else if (prop == _gps->_routeFinishedSignal) {
      _gps->routeFinished();
    }
        
    _guard = false;
  }
  
  void setGuard(bool g) {
    _guard = g;
  }
private:
  GPS* _gps;
  bool _guard; // re-entrancy guard
};

////////////////////////////////////////////////////////////////////////////
/**
 * Helper to monitor for Nasal or other code accessing properties we haven't
 * defined. For the moment we complain about all such activites, since various
 * users assume all kinds of weird, wonderful and non-existent interfaces.
 */
 
class DeprecatedPropListener : public SGPropertyChangeListener
{
public:
  DeprecatedPropListener(SGPropertyNode* gps)
  {
    _parents.insert(gps);
    SGPropertyNode* wp = gps->getChild("wp"); 
    _parents.insert(wp);
    _parents.insert(wp->getChild("wp", 0));
    _parents.insert(wp->getChild("wp", 1));
    
    std::set<SGPropertyNode*>::iterator it;
    for (it = _parents.begin(); it != _parents.end(); ++it) {
      (*it)->addChangeListener(this);
    }
  }
  
  virtual void valueChanged (SGPropertyNode * prop)
  {
  }
  
  virtual void childAdded (SGPropertyNode * parent, SGPropertyNode * child)
  {
    if (isDeprecated(parent, child)) {
      SG_LOG(SG_INSTR, SG_WARN, "GPS: someone accessed a deprecated property:"
        << child->getPath(true));
    }
  }
private:
  bool isDeprecated(SGPropertyNode * parent, SGPropertyNode * child) const 
  {
    if (_parents.count(parent) < 1) {
      return false;
    }
    
    // no child exclusions yet
    return true;
  }
  
  std::set<SGPropertyNode*> _parents;
};

////////////////////////////////////////////////////////////////////////////
// configuration helper object

GPS::Config::Config() :
  _enableTurnAnticipation(true),
  _turnRate(3.0), // degrees-per-second, so 180 degree turn takes 60 seconds
  _overflightArmDistance(1.0),
  _waypointAlertTime(30.0),
  _minRunwayLengthFt(0.0),
  _requireHardSurface(true),
  _cdiMaxDeflectionNm(3.0), // linear mode, 3nm at the peg
  _driveAutopilot(true),
  _courseSelectable(false)
{
  _enableTurnAnticipation = false;
}

void GPS::Config::bind(GPS* aOwner, SGPropertyNode* aCfg)
{
  aOwner->tie(aCfg, "turn-rate-deg-sec", SGRawValuePointer<double>(&_turnRate));
  aOwner->tie(aCfg, "turn-anticipation", SGRawValuePointer<bool>(&_enableTurnAnticipation));
  aOwner->tie(aCfg, "wpt-alert-time", SGRawValuePointer<double>(&_waypointAlertTime));
  aOwner->tie(aCfg, "min-runway-length-ft", SGRawValuePointer<double>(&_minRunwayLengthFt));
  aOwner->tie(aCfg, "hard-surface-runways-only", SGRawValuePointer<bool>(&_requireHardSurface));
  aOwner->tie(aCfg, "cdi-max-deflection-nm", SGRawValuePointer<double>(&_cdiMaxDeflectionNm));
  aOwner->tie(aCfg, "drive-autopilot", SGRawValuePointer<bool>(&_driveAutopilot));
  aOwner->tie(aCfg, "course-selectable", SGRawValuePointer<bool>(&_courseSelectable));
}

////////////////////////////////////////////////////////////////////////////

GPS::GPS ( SGPropertyNode *node) : 
  _selectedCourse(0.0),
  _desiredCourse(0.0),
  _dataValid(false),
  _lastPosValid(false),
  _mode("init"),
  _name(node->getStringValue("name", "gps")),
  _num(node->getIntValue("number", 0)),
  _searchResultsCached(false),
  _computeTurnData(false),
  _anticipateTurn(false),
  _inTurn(false)
{
  string branch = "/instrumentation/" + _name;
  _gpsNode = fgGetNode(branch.c_str(), _num, true );
  _scratchNode = _gpsNode->getChild("scratch", 0, true);
}

GPS::~GPS ()
{
}

void
GPS::init ()
{
  _routeMgr = (FGRouteMgr*) globals->get_subsystem("route-manager");
  assert(_routeMgr);
  
  _position.init("/position/longitude-deg", "/position/latitude-deg", "/position/altitude-ft");
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
  SGPropertyNode *wp_node = _gpsNode->getChild("wp", 0, true);
  SGPropertyNode *wp1_node = wp_node->getChild("wp", 1, true);

  // for compatability, alias selected course down to wp/wp[1]/desired-course-deg
  SGPropertyNode* wp1Crs = wp1_node->getChild("desired-course-deg", 0, true);
  wp1Crs->alias(_gpsNode->getChild("desired-course-deg", 0, true));
    
//    _true_wp1_bearing_error_node =
//        wp1_node->getChild("true-bearing-error-deg", 0, true);
//    _magnetic_wp1_bearing_error_node =
  //      wp1_node->getChild("magnetic-bearing-error-deg", 0, true);

  _tracking_bug_node = _gpsNode->getChild("tracking-bug", 0, true);
         
// reference navid
  SGPropertyNode_ptr ref_navaid = _gpsNode->getChild("ref-navaid", 0, true);
  _ref_navaid_id_node = ref_navaid->getChild("id", 0, true);
  _ref_navaid_name_node = ref_navaid->getChild("name", 0, true);
  _ref_navaid_bearing_node = ref_navaid->getChild("bearing-deg", 0, true);
  _ref_navaid_frequency_node = ref_navaid->getChild("frequency-mhz", 0, true);
  _ref_navaid_distance_node = ref_navaid->getChild("distance-nm", 0, true);
  _ref_navaid_mag_bearing_node = ref_navaid->getChild("mag-bearing-deg", 0, true);
  _ref_navaid_elapsed = 0.0;
  _ref_navaid_set = false;
    
// route properties    
  // should these move to the route manager?
  _routeDistanceNm = _gpsNode->getChild("route-distance-nm", 0, true);
  _routeETE = _gpsNode->getChild("ETE", 0, true);
  _routeEditedSignal = fgGetNode("/autopilot/route-manager/signals/edited", true);
  _routeFinishedSignal = fgGetNode("/autopilot/route-manager/signals/finished", true);
  
// add listener to various things
  _listener = new GPSListener(this);
  _route_current_wp_node = fgGetNode("/autopilot/route-manager/current-wp", true);
  _route_current_wp_node->addChangeListener(_listener);
  _route_active_node = fgGetNode("/autopilot/route-manager/active", true);
  _route_active_node->addChangeListener(_listener);
  _ref_navaid_id_node->addChangeListener(_listener);
  _routeEditedSignal->addChangeListener(_listener);
  _routeFinishedSignal->addChangeListener(_listener);
  
// navradio slaving properties  
  SGPropertyNode* toFlag = _gpsNode->getChild("to-flag", 0, true);
  toFlag->alias(wp1_node->getChild("to-flag"));
  
  SGPropertyNode* fromFlag = _gpsNode->getChild("from-flag", 0, true);
  fromFlag->alias(wp1_node->getChild("from-flag"));
    
// autopilot drive properties
  _apDrivingFlag = fgGetNode("/autopilot/settings/gps-driving-true-heading", true);
  _apTrueHeading = fgGetNode("/autopilot/settings/true-heading-deg",true);
  _apTargetAltitudeFt = fgGetNode("/autopilot/settings/target-altitude-ft", true);
  _apAltitudeLock = fgGetNode("/autopilot/locks/altitude", true);
  
// realism prop[s]
  _realismSimpleGps = fgGetNode("/sim/realism/simple-gps", true);
  if (!_realismSimpleGps->hasValue()) {
    _realismSimpleGps->setBoolValue(true);
  }
  
  // last thing, add the deprecated prop watcher
  new DeprecatedPropListener(_gpsNode);
  
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
  _desiredCourseNode = _gpsNode->getChild("desired-course-deg");
    
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
  tie(_scratchNode, "valid", SGRawValueMethods<GPS, bool>(*this, &GPS::getScratchValid, NULL));
  tie(_scratchNode, "distance-nm", SGRawValueMethods<GPS, double>(*this, &GPS::getScratchDistance, NULL));
  tie(_scratchNode, "true-bearing-deg", SGRawValueMethods<GPS, double>(*this, &GPS::getScratchTrueBearing, NULL));
  tie(_scratchNode, "mag-bearing-deg", SGRawValueMethods<GPS, double>(*this, &GPS::getScratchMagBearing, NULL));
  tie(_scratchNode, "has-next", SGRawValueMethods<GPS, bool>(*this, &GPS::getScratchHasNext, NULL));
  _scratchValid = false;
  
// waypoint data (including various historical things)
  SGPropertyNode *wp_node = _gpsNode->getChild("wp", 0, true);
  SGPropertyNode *wp0_node = wp_node->getChild("wp", 0, true);
  SGPropertyNode *wp1_node = wp_node->getChild("wp", 1, true);

  tieSGGeodReadOnly(wp0_node, _wp0_position, "longitude-deg", "latitude-deg", "altitude-ft");
  tie(wp0_node, "ID", SGRawValueMethods<GPS, const char*>
    (*this, &GPS::getWP0Ident, NULL));
  tie(wp0_node, "name", SGRawValueMethods<GPS, const char*>
    (*this, &GPS::getWP0Name, NULL));
    
  tieSGGeodReadOnly(wp1_node, _wp1_position, "longitude-deg", "latitude-deg", "altitude-ft");
  tie(wp1_node, "ID", SGRawValueMethods<GPS, const char*>
    (*this, &GPS::getWP1Ident, NULL));
  tie(wp1_node, "name", SGRawValueMethods<GPS, const char*>
    (*this, &GPS::getWP1Name, NULL));
  
  tie(wp1_node, "distance-nm", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1Distance, NULL));
  tie(wp1_node, "bearing-true-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1Bearing, NULL));
  tie(wp1_node, "bearing-mag-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1MagBearing, NULL));
  tie(wp1_node, "TTW-sec", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1TTW, NULL));
  tie(wp1_node, "TTW", SGRawValueMethods<GPS, const char*>
    (*this, &GPS::getWP1TTWString, NULL));
  
  tie(wp1_node, "course-deviation-deg", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1CourseDeviation, NULL));
  tie(wp1_node, "course-error-nm", SGRawValueMethods<GPS, double>
    (*this, &GPS::getWP1CourseErrorNm, NULL));
  tie(wp1_node, "to-flag", SGRawValueMethods<GPS, bool>
    (*this, &GPS::getWP1ToFlag, NULL));
  tie(wp1_node, "from-flag", SGRawValueMethods<GPS, bool>
    (*this, &GPS::getWP1FromFlag, NULL));

// leg properties (only valid in DTO/LEG modes, not OBS)
  tie(wp_node, "leg-distance-nm", SGRawValueMethods<GPS, double>(*this, &GPS::getLegDistance, NULL));
  tie(wp_node, "leg-true-course-deg", SGRawValueMethods<GPS, double>(*this, &GPS::getLegCourse, NULL));
  tie(wp_node, "leg-mag-course-deg", SGRawValueMethods<GPS, double>(*this, &GPS::getLegMagCourse, NULL));
  tie(wp_node, "alt-dist-ratio", SGRawValueMethods<GPS, double>(*this, &GPS::getAltDistanceRatio, NULL));

// navradio slaving properties  
  tie(_gpsNode, "cdi-deflection", SGRawValueMethods<GPS,double>
    (*this, &GPS::getCDIDeflection));
}

void
GPS::unbind()
{
  for (unsigned int t=0; t<_tiedNodes.size(); ++t) {
    _tiedNodes[t]->untie();
  }
  _tiedNodes.clear();
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
  
  _raim_node->setDoubleValue(0.0);
  _indicated_pos = SGGeod();
  _wp1DistanceM = 0.0;
  _wp1TrueBearing = 0.0;
  _wp1_position = SGGeod();
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
  if (!_realismSimpleGps->getBoolValue()) {
    // If it's off, don't bother.
    if (!_serviceable_node->getBoolValue() || !_electrical_node->getBoolValue()) {
      clearOutput();
      return;
    }
  }
  
  if (delta_time_sec <= 0.0) {
    return; // paused, don't bother
  }    
    // TODO: Add noise and other errors.
/*

    // Bias and random error
    double random_factor = sg_random();
    double random_error = 1.4;
    double error_radius = 5.1;
    double bias_max_radius = 5.1;
    double random_max_radius = 1.4;

    bias_length += (random_factor-0.5) * 1.0e-3;
    if (bias_length <= 0.0) bias_length = 0.0;
    else if (bias_length >= bias_max_radius) bias_length = bias_max_radius;
    bias_angle  += (random_factor-0.5) * 1.0e-3;
    if (bias_angle <= 0.0) bias_angle = 0.0;
    else if (bias_angle >= 360.0) bias_angle = 360.0;

    double random_length = random_factor * random_max_radius;
    double random_angle = random_factor * 360.0;

    double bias_x = bias_length * cos(bias_angle * SG_PI / 180.0);
    double bias_y = bias_length * sin(bias_angle * SG_PI / 180.0);
    double random_x = random_length * cos(random_angle * SG_PI / 180.0);
    double random_y = random_length * sin(random_angle * SG_PI / 180.0);
    double error_x = bias_x + random_x;
    double error_y = bias_y + random_y;
    double error_length = sqrt(error_x*error_x + error_y*error_y);
    double error_angle = atan(error_y / error_x) * 180.0 / SG_PI;

    double lat2;
    double lon2;
    double az2;
    geo_direct_wgs_84 ( altitude_m, latitude_deg,
                        longitude_deg, error_angle,
                        error_length, &lat2, &lon2,
                        &az2 );
    //cout << lat2 << " " << lon2 << endl;
    printf("%f %f \n", bias_length, bias_angle);
    printf("%3.7f %3.7f \n", lat2, lon2);
    printf("%f %f \n", error_length, error_angle);

*/
  _raim_node->setDoubleValue(1.0);
  _indicated_pos = _position.get();
  updateBasicData(delta_time_sec);

  if (_dataValid) {
    if (_mode != "obs") {
      updateTurn();
    }
      
    updateWaypoints();
    updateTrackingBug();
    updateReferenceNavaid(delta_time_sec);
    updateRouteData();
    driveAutopilot();
  }
  
  if (_dataValid && (_mode == "init")) {
    // allow a realistic delay in the future, here
    SG_LOG(SG_INSTR, SG_INFO, "GPS initialisation complete");
        
    if (_route_active_node->getBoolValue()) {
      // GPS init with active route
      SG_LOG(SG_INSTR, SG_INFO, "GPS init with active route");
      selectLegMode();
    } else {
      // initialise in OBS mode, with waypt set to the nearest airport.
      // keep in mind at this point, _dataValid is not set
    
      auto_ptr<FGPositioned::Filter> f(createFilter(FGPositioned::AIRPORT));
      FGPositionedRef apt = FGPositioned::findClosest(_position.get(), 20.0, f.get());
      if (apt) {
        setScratchFromPositioned(apt, 0);
        selectOBSMode();
      }
    }
  } // of init mode check
  
  _last_pos = _indicated_pos;
  _lastPosValid = true;
}

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
  double northSouthM = SGGeodesy::distanceM(_last_pos, g);
  northSouthM = copysign(northSouthM, _indicated_pos.getLatitudeDeg() - _last_pos.getLatitudeDeg());
  
  double nsMSec = fgGetLowPass(_lastNSVelocity, northSouthM / dt, dt/2.0);
  _lastNSVelocity = nsMSec;
  _northSouthVelocity->setDoubleValue(nsMSec);


  g = _indicated_pos;
  g.setLatitudeDeg(_last_pos.getLatitudeDeg());
  double eastWestM = SGGeodesy::distanceM(_last_pos, g);
  eastWestM = copysign(eastWestM, _indicated_pos.getLongitudeDeg() - _last_pos.getLongitudeDeg());
  
  double ewMSec = fgGetLowPass(_lastEWVelocity, eastWestM / dt, dt/2.0);
  _lastEWVelocity = ewMSec;
  _eastWestVelocity->setDoubleValue(ewMSec);
  
  double odometer = _odometer_node->getDoubleValue();
  _odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);
  odometer = _trip_odometer_node->getDoubleValue();
  _trip_odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);
  
  if (!_dataValid) {
    SG_LOG(SG_INSTR, SG_INFO, "GPS setting data valid");
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

void
GPS::updateWaypoints()
{  
  double az2;
  SGGeodesy::inverse(_indicated_pos, _wp1_position, _wp1TrueBearing, az2,_wp1DistanceM);
}

void GPS::updateReferenceNavaid(double dt)
{
  if (!_ref_navaid_set) {
    _ref_navaid_elapsed += dt;
    if (_ref_navaid_elapsed > 5.0) {
      _ref_navaid_elapsed = 0.0;

      FGPositioned::TypeFilter vorFilter(FGPositioned::VOR);
      FGPositionedRef nav = FGPositioned::findClosest(_indicated_pos, 400.0, &vorFilter);
      if (!nav) {
        SG_LOG(SG_INSTR, SG_INFO, "GPS couldn't find a reference navid");
        _ref_navaid_id_node->setStringValue("");
        _ref_navaid_name_node->setStringValue("");
        _ref_navaid_bearing_node->setDoubleValue(0.0);
        _ref_navaid_mag_bearing_node->setDoubleValue(0.0);
        _ref_navaid_distance_node->setDoubleValue(0.0);
        _ref_navaid_frequency_node->setStringValue("");
      } else if (nav != _ref_navaid) {
        SG_LOG(SG_INSTR, SG_INFO, "GPS code selected new ref-navaid:" << nav->ident());
        _listener->setGuard(true);
        _ref_navaid_id_node->setStringValue(nav->ident().c_str());
        _ref_navaid_name_node->setStringValue(nav->name().c_str());
        FGNavRecord* vor = (FGNavRecord*) nav.ptr();
        _ref_navaid_frequency_node->setDoubleValue(vor->get_freq() / 100.0);
        _listener->setGuard(false);
      } else {
        // SG_LOG(SG_INSTR, SG_ALERT, "matched existing");
      }
      
      _ref_navaid = nav;
    }
  }
  
  if (_ref_navaid) {
    double trueCourse, distanceM, az2;
    SGGeodesy::inverse(_indicated_pos, _ref_navaid->geod(), trueCourse, az2, distanceM);
    _ref_navaid_distance_node->setDoubleValue(distanceM * SG_METER_TO_NM);
    _ref_navaid_bearing_node->setDoubleValue(trueCourse);
    _ref_navaid_mag_bearing_node->setDoubleValue(trueCourse - _magvar_node->getDoubleValue());
  }
}

void GPS::referenceNavaidSet(const std::string& aNavaid)
{
  _ref_navaid = NULL;
  // allow setting an empty string to restore normal nearest-vor selection
  if (aNavaid.size() > 0) {
    FGPositioned::TypeFilter vorFilter(FGPositioned::VOR);
    _ref_navaid = FGPositioned::findClosestWithIdent(aNavaid, 
      _position.get(), &vorFilter);
    
    if (!_ref_navaid) {
      SG_LOG(SG_INSTR, SG_ALERT, "GPS: unknown ref navaid:" << aNavaid);
    }
  }

  if (_ref_navaid) {
    _ref_navaid_set = true;
    SG_LOG(SG_INSTR, SG_INFO, "GPS code set explict ref-navaid:" << _ref_navaid->ident());
    _ref_navaid_id_node->setStringValue(_ref_navaid->ident().c_str());
    _ref_navaid_name_node->setStringValue(_ref_navaid->name().c_str());
    FGNavRecord* vor = (FGNavRecord*) _ref_navaid.ptr();
    _ref_navaid_frequency_node->setDoubleValue(vor->get_freq() / 100.0);
  } else {
    _ref_navaid_set = false;
    _ref_navaid_elapsed = 9999.0; // update next tick
  }
}

void GPS::routeActivated()
{
  if (_route_active_node->getBoolValue()) {
    SG_LOG(SG_INSTR, SG_INFO, "GPS::route activated, switching to LEG mode");
    selectLegMode();
    
    // if we've already passed the current waypoint, sequence.
    if (_dataValid && getWP1FromFlag()) {
      SG_LOG(SG_INSTR, SG_INFO, "GPS::route activated, FROM wp1, sequencing");
      _routeMgr->sequence();
    }
  } else if (_mode == "leg") {
    SG_LOG(SG_INSTR, SG_INFO, "GPS::route deactivated, switching to OBS mode");
    selectOBSMode();
  }
}

void GPS::routeManagerSequenced()
{
  if (_mode != "leg") {
    SG_LOG(SG_INSTR, SG_INFO, "GPS ignoring route sequencing, not in LEG mode");
    return;
  }
  
  int index = _routeMgr->currentWaypoint(),
    count = _routeMgr->size();
  if ((index < 0) || (index >= count)) {
    SG_LOG(SG_INSTR, SG_ALERT, "GPS: malformed route, index=" << index);
    return;
  }
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS waypoint index is now " << index);
  
  if (index > 0) {
    SGWayPoint wp0(_routeMgr->get_waypoint(index - 1));
    _wp0Ident = wp0.get_id();
    _wp0Name = wp0.get_name();
    _wp0_position = wp0.get_target();

  }
  
  SGWayPoint wp1(_routeMgr->get_waypoint(index));
  _wp1Ident = wp1.get_id();
  _wp1Name = wp1.get_name();
  _wp1_position = wp1.get_target();

  _desiredCourse = getLegMagCourse();
  _desiredCourseNode->fireValueChanged();
  wp1Changed();
}

void GPS::routeEdited()
{
  if (_mode != "leg") {
    return;
  }
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS route edited while in LEG mode, updating waypoints");
  routeManagerSequenced();
}

void GPS::routeFinished()
{
  if (_mode != "leg") {
    return;
  }
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS route finished, reverting to OBS");
  _mode = "obs";
  _wp0_position = _indicated_pos;
  wp1Changed();
}

void GPS::updateTurn()
{
  bool printProgress = false;
  
  if (_computeTurnData) {
    if (_last_speed_kts < 60) {
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
     _routeMgr->sequence();
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
    // FXIME
    //_wp1_course_deviation_node->setDoubleValue(deviationDeg);
    //_wp1_course_error_nm_node->setDoubleValue(deviationNm);
    //_cdiDeflectionNode->setDoubleValue(deviationDeg);
  }
}

void GPS::updateOverflight()
{
  if ((_wp1DistanceM * SG_METER_TO_NM) > _config.overflightArmDistanceNm()) {
    return;
  }
  
  if (getWP1ToFlag()) {
    return; // still heading towards the WP
  }
  
  if (_mode == "dto") {
    SG_LOG(SG_INSTR, SG_INFO, "GPS DTO reached destination point");
    
    // check for wp1 being on active route - resume leg mode
    if (_routeMgr->isRouteActive()) {
      int index = _routeMgr->findWaypoint(_wp1_position);
      if (index >= 0) {
        SG_LOG(SG_INSTR, SG_INFO, "GPS DTO, resuming LEG mode at wp:" << index);
        _mode = "leg";
        _routeMgr->jumpToIndex(index);
      }
    }
  } else if (_mode == "leg") {
    SG_LOG(SG_INSTR, SG_INFO, "GPS doing overflight sequencing");
    _routeMgr->sequence();
  } else if (_mode == "obs") {
    // nothing to do here, TO/FROM will update but that's fine
  }
  
  _computeTurnData = true;
}

void GPS::beginTurn()
{
  _inTurn = true;
  _turnSequenced = false;
  SG_LOG(SG_INSTR, SG_INFO, "begining turn");
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
  if (_mode != "leg") { // and approach modes in the future
    _anticipateTurn = false;
    return;
  }
  
  int curIndex = _routeMgr->currentWaypoint();
  if ((curIndex + 1) >= _routeMgr->size()) {
    _anticipateTurn = false;
    return;
  }
  
  if (!_config.turnAnticipationEnabled()) {
    _anticipateTurn = false;
    return;
  }
  
  _turnStartBearing = _desiredCourse;
// compute next leg course
  SGWayPoint wp1(_routeMgr->get_waypoint(curIndex)),
    wp2(_routeMgr->get_waypoint(curIndex + 1));
  double crs, dist;
  wp2.CourseAndDistance(wp1, &crs, &dist);
  

// compute offset bearing
  _turnAngle = crs - _turnStartBearing;
  SG_NORMALIZE_RANGE(_turnAngle, -180.0, 180.0);
  double median = _turnStartBearing + (_turnAngle * 0.5);
  double offsetBearing = median + copysign(90, _turnAngle);
  SG_NORMALIZE_RANGE(offsetBearing, 0.0, 360.0);
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS computeTurnData: in=" << _turnStartBearing <<
    ", out=" << crs << "; turnAngle=" << _turnAngle << ", median=" << median 
    << ", offset=" << offsetBearing);

  SG_LOG(SG_INSTR, SG_INFO, "next leg is now:" << wp1.get_id() << "->" << wp2.get_id());

  _turnPt = _wp1_position;
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
  double totalDistance = _wp1DistanceM * SG_METER_TO_NM;
  // walk all waypoints from wp2 to route end, and sum
  for (int i=_routeMgr->currentWaypoint()+1; i<_routeMgr->size(); ++i) {
    totalDistance += _routeMgr->get_waypoint(i).get_distance();
  }
  
  _routeDistanceNm->setDoubleValue(totalDistance * SG_METER_TO_NM);
  if (_last_speed_kts > 1.0) {
    double TTW = ((totalDistance * SG_METER_TO_NM) / _last_speed_kts) * 3600.0;
    _routeETE->setStringValue(makeTTWString(TTW));    
  }
}

void GPS::driveAutopilot()
{
  if (!_config.driveAutopilot() || !_realismSimpleGps->getBoolValue()) {
    _apDrivingFlag->setBoolValue(false);
    return;
  }
 
  // compatability feature - allow the route-manager / GPS to drive the
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
  if (!_config.driveAutopilot()) {
    return;
  }
  
  double altFt = _wp1_position.getElevationFt();
  if (altFt > -9990.0) {
    _apTargetAltitudeFt->setDoubleValue(altFt);
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
  
  return SGGeodesy::distanceNm(_wp0_position, _wp1_position);
}

double GPS::getLegCourse() const
{
  if (!_dataValid) {
    return -9999.0;
  }
  
  return SGGeodesy::courseDeg(_wp0_position, _wp1_position);
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

double GPS::getAltDistanceRatio() const
{
  if (!_dataValid || (_mode == "obs")) {
    return 0.0;
  }
  
  double dist = SGGeodesy::distanceM(_wp0_position, _wp1_position);
  if ( dist <= 0.0 ) {
    return 0.0;
  }
  
  double alt_difference_m = _wp0_position.getElevationM() - _wp1_position.getElevationM();
  return alt_difference_m / dist;
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
  if (!_dataValid || (_mode != "leg")) {
    return "";
  }
  
  return _wp0Ident.c_str();
}

const char* GPS::getWP0Name() const
{
  if (!_dataValid || (_mode != "leg")) {
    return "";
  }
  
  return _wp0Name.c_str();
}

const char* GPS::getWP1Ident() const
{
  if (!_dataValid) {
    return "";
  }
  
  return _wp1Ident.c_str();
}

const char* GPS::getWP1Name() const
{
  if (!_dataValid) {
    return "";
  }

  return _wp1Name.c_str();
}

double GPS::getWP1Distance() const
{
  if (!_dataValid) {
    return -1.0;
  }
  
  return _wp1DistanceM * SG_METER_TO_NM;
}

double GPS::getWP1TTW() const
{
  if (!_dataValid) {
    return -1.0;
  }
  
  if (_last_speed_kts < 1.0) {
    return -1.0;
  }
  
  return (getWP1Distance() / _last_speed_kts) * 3600.0;
}

const char* GPS::getWP1TTWString() const
{
  if (!_dataValid) {
    return "";
  }
  
  return makeTTWString(getWP1TTW());
}

double GPS::getWP1Bearing() const
{
  if (!_dataValid) {
    return -9999.0;
  }
  
  return _wp1TrueBearing;
}

double GPS::getWP1MagBearing() const
{
  if (!_dataValid) {
    return -9999.0;
  }

  double magBearing = _wp1TrueBearing - _magvar_node->getDoubleValue();
  SG_NORMALIZE_RANGE(magBearing, 0.0, 360.0);
  return magBearing;
}

double GPS::getWP1CourseDeviation() const
{
  if (!_dataValid) {
    return 0.0;
  }
  
  double dev = getWP1MagBearing() - _desiredCourse;
  SG_NORMALIZE_RANGE(dev, -180.0, 180.0);
  
  if (fabs(dev) > 90.0) {
    // When the course is away from the waypoint, 
    // it makes sense to change the sign of the deviation.
    dev *= -1.0;
    SG_NORMALIZE_RANGE(dev, -90.0, 90.0);
  }
  
  return dev;
}

double GPS::getWP1CourseErrorNm() const
{
  if (!_dataValid) {
    return 0.0;
  }
  
  double radDev = getWP1CourseDeviation() * SG_DEGREES_TO_RADIANS;
  double course_error_m = sin(radDev) * _wp1DistanceM;
  return course_error_m * SG_METER_TO_NM;
}

bool GPS::getWP1ToFlag() const
{
  if (!_dataValid) {
    return false;
  }
  
  double dev = getWP1MagBearing() - _desiredCourse;
  SG_NORMALIZE_RANGE(dev, -180.0, 180.0);

  return (fabs(dev) < 90.0);
}

bool GPS::getWP1FromFlag() const
{
  if (!_dataValid) {
    return false;
  }
  
  return !getWP1ToFlag();
}

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

/////////////////////////////////////////////////////////////////////////////
// command / scratch / search system

void GPS::setCommand(const char* aCmd)
{
  SG_LOG(SG_INSTR, SG_INFO, "GPS command:" << aCmd);
  
  if (!strcmp(aCmd, "direct")) {
    directTo();
  } else if (!strcmp(aCmd, "obs")) {
    selectOBSMode();
  } else if (!strcmp(aCmd, "leg")) {
    selectLegMode();
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
    if (index < 0 || (_routeMgr->size() == 0)) {
      index = _routeMgr->size();
    } else if (index >= _routeMgr->size()) {
      SG_LOG(SG_INSTR, SG_WARN, "GPS:route-insert-before, bad index:" << index);
      return;
    }
    
    insertWaypointAtIndex(index);
  } else if (!strcmp(aCmd, "route-insert-after")) {
    int index = _scratchNode->getIntValue("index");
    if (index < 0 || (_routeMgr->size() == 0)) {
      index = _routeMgr->size();
    } else if (index >= _routeMgr->size()) {
      SG_LOG(SG_INSTR, SG_WARN, "GPS:route-insert-after, bad index:" << index);
      return;
    } else {
      ++index; 
    }
  
    insertWaypointAtIndex(index);
  } else if (!strcmp(aCmd, "route-delete")) {
    int index = _scratchNode->getIntValue("index");
    if (index < 0) {
      index = _routeMgr->size();
    } else if (index >= _routeMgr->size()) {
      SG_LOG(SG_INSTR, SG_WARN, "GPS:route-delete, bad index:" << index);
      return;
    }
    
    removeWaypointAtIndex(index);
  } else {
    SG_LOG(SG_INSTR, SG_WARN, "GPS:unrecognized command:" << aCmd);
  }
}

void GPS::clearScratch()
{
  _scratchPos = SGGeod::fromDegFt(-9999.0, -9999.0, -9999.0);
  _scratchValid = false;  
  _scratchNode->setStringValue("type", "");
  _scratchNode->setStringValue("query", "");
}

bool GPS::isScratchPositionValid() const
{
  if ((_scratchPos.getLongitudeDeg() < -9990.0) ||
      (_scratchPos.getLatitudeDeg() < -9990.0)) {
   return false;   
  }
  
  return true;
}

void GPS::directTo()
{
  _wp0_position = _indicated_pos;
  
  if (isScratchPositionValid()) {
    _wp1Ident = _scratchNode->getStringValue("ident");
    _wp1Name = _scratchNode->getStringValue("name");
    _wp1_position = _scratchPos;
  }
  
  _mode = "dto";
  _desiredCourse = getLegMagCourse();
  _desiredCourseNode->fireValueChanged();
  clearScratch();
  wp1Changed();
}

void GPS::loadRouteWaypoint()
{
  _scratchValid = false;
//  if (!_routeMgr->isRouteActive()) {
//    SG_LOG(SG_INSTR, SG_WARN, "GPS:loadWaypoint: no active route");
//    return;
//  }
  
  int index = _scratchNode->getIntValue("index", -9999);
  clearScratch();
  
  if ((index < 0) || (index >= _routeMgr->size())) { // no index supplied, use current wp
    index = _routeMgr->currentWaypoint();
  }
  
  _searchIsRoute = true;
  setScratchFromRouteWaypoint(index);
}

void GPS::setScratchFromRouteWaypoint(int aIndex)
{
  assert(_searchIsRoute);
  if ((aIndex < 0) || (aIndex >= _routeMgr->size())) {
    SG_LOG(SG_INSTR, SG_WARN, "GPS:setScratchFromRouteWaypoint: route-index out of bounds");
    return;
  }
  
  _searchResultIndex = aIndex;
  SGWayPoint wp(_routeMgr->get_waypoint(aIndex));
  _scratchNode->setStringValue("name", wp.get_name());
  _scratchNode->setStringValue("ident", wp.get_id());
  _scratchPos = wp.get_target();
  _scratchValid = true;
  _scratchNode->setDoubleValue("course", wp.get_track());
  _scratchNode->setIntValue("index", aIndex);
  
  int lastResult = _routeMgr->size() - 1;
  _searchHasNext = (_searchResultIndex < lastResult);
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
  _searchResultsCached = true;
  _searchResultIndex = 0;
  _searchIsRoute = false;
  _searchHasNext = false;
  
  if (_searchResults.empty()) {
    SG_LOG(SG_INSTR, SG_INFO, "GPS:loadNearest: no matches at all");
    return;
  }
  
  _searchHasNext = (_searchResults.size() > 1);
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
  return FGPositioned::WAYPOINT;
}

FGPositioned::Filter* GPS::createFilter(FGPositioned::Type aTy)
{
  if (aTy == FGPositioned::AIRPORT) {
    return new FGAirport::HardSurfaceFilter(_config.minRunwayLengthFt());
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
  _searchOrderByRange = _scratchNode->getBoolValue("order-by-distance", true);
  _searchResultIndex = 0;
  _searchIsRoute = false;
  _searchHasNext = false;
  
  if (_searchExact && _searchOrderByRange) {
    // immediate mode search, get all the results now and cache them
    auto_ptr<FGPositioned::Filter> f(createFilter(_searchType));
    if (_searchNames) {
      _searchResults = FGPositioned::findAllWithNameSortedByRange(_searchQuery, _indicated_pos, f.get());
    } else {
      _searchResults = FGPositioned::findAllWithIdentSortedByRange(_searchQuery, _indicated_pos, f.get());
    }
    
    _searchResultsCached = true;
    
    if (_searchResults.empty()) {
      clearScratch();
      return;
    }
    
    _searchHasNext = (_searchResults.size() > 1);
    setScratchFromCachedSearchResult();
  } else {
    // iterative search, look up result zero
    _searchResultsCached = false;
    performSearch();
  }
}

void GPS::performSearch()
{
  auto_ptr<FGPositioned::Filter> f(createFilter(_searchType));
  clearScratch();
  
  FGPositionedRef r;
  if (_searchNames) {
    if (_searchOrderByRange) {
      r = FGPositioned::findClosestWithPartialName(_indicated_pos, _searchQuery, f.get(), _searchResultIndex, _searchHasNext);
    } else {
      r = FGPositioned::findWithPartialName(_searchQuery, f.get(), _searchResultIndex, _searchHasNext);
    }
  } else {
    if (_searchOrderByRange) {
      r = FGPositioned::findClosestWithPartialId(_indicated_pos, _searchQuery, f.get(), _searchResultIndex, _searchHasNext);
    } else {
      r = FGPositioned::findWithPartialId(_searchQuery, f.get(), _searchResultIndex, _searchHasNext);
    }
  }
  
  if (!r) {
    return;
  }
  
  setScratchFromPositioned(r.get(), _searchResultIndex);
}

void GPS::setScratchFromCachedSearchResult()
{
  assert(_searchResultsCached);
  int index = _searchResultIndex;
  
  if ((index < 0) || (index >= (int) _searchResults.size())) {
    SG_LOG(SG_INSTR, SG_WARN, "GPS:setScratchFromCachedSearchResult: index out of bounds:" << index);
    return;
  }
  
  setScratchFromPositioned(_searchResults[index], index);
  
  int lastResult = (int) _searchResults.size() - 1;
  _searchHasNext = (_searchResultIndex < lastResult);
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
  if (_searchResultsCached) {
    _scratchNode->setIntValue("result-count", _searchResults.size());
  }
  
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


void GPS::selectOBSMode()
{
  if (isScratchPositionValid()) {
    _wp1Ident = _scratchNode->getStringValue("ident");
    _wp1Name = _scratchNode->getStringValue("name");
    _wp1_position = _scratchPos;
  }
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS switching to OBS mode");
  _mode = "obs";
  _wp0_position = _indicated_pos;
  wp1Changed();
}

void GPS::selectLegMode()
{
  if (_mode == "leg") {
    return;
  }
  
  if (!_routeMgr->isRouteActive()) {
    SG_LOG(SG_INSTR, SG_WARN, "GPS:selectLegMode: no active route");
    return;
  }

  SG_LOG(SG_INSTR, SG_INFO, "GPS switching to LEG mode");
  _mode = "leg";
  
  // depending on the situation, this will either get over-written 
  // in routeManagerSequenced or not; either way it does no harm to
  // set it here.
  _wp0_position = _indicated_pos;

  // not really sequenced, but does all the work of updating wp0/1
  routeManagerSequenced();
}

void GPS::nextResult()
{
  if (!_searchHasNext) {
    return;
  }
  
  clearScratch();
  if (_searchIsRoute) {
    setScratchFromRouteWaypoint(++_searchResultIndex);
  } else if (_searchResultsCached) {
    ++_searchResultIndex;
    setScratchFromCachedSearchResult();
  } else {
    ++_searchResultIndex;
    performSearch();
  } // of iterative search case
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
  } else if (_searchResultsCached) {
    setScratchFromCachedSearchResult();
  } else {
    performSearch();
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
  FGPositioned::List dups = FGPositioned::findAllWithIdentSortedByRange(ident, _indicated_pos, &f);
  if (!dups.empty()) {
    SG_LOG(SG_INSTR, SG_WARN, "GPS:defineWaypoint: non-unique waypoint identifier, ho-hum");
  }
  
  SG_LOG(SG_INSTR, SG_INFO, "GPS:defineWaypoint: creating waypoint:" << ident);
  FGPositionedRef wpt = FGPositioned::createUserWaypoint(ident, _scratchPos);
  _searchResultsCached = false;
  setScratchFromPositioned(wpt.get(), -1);
}

void GPS::insertWaypointAtIndex(int aIndex)
{
  // note we do allow index = routeMgr->size(), that's an append
  if ((aIndex < 0) || (aIndex > _routeMgr->size())) {
    throw sg_range_exception("GPS::insertWaypointAtIndex: index out of bounds");
  }
  
  if (!isScratchPositionValid()) {
    SG_LOG(SG_INSTR, SG_WARN, "GPS:insertWaypointAtIndex: invalid lat/lon");
    return;
  }
  
  string ident = _scratchNode->getStringValue("ident");
  string name = _scratchNode->getStringValue("name");
  
  _routeMgr->add_waypoint(SGWayPoint(_scratchPos, ident, name), aIndex);
}

void GPS::removeWaypointAtIndex(int aIndex)
{
  if ((aIndex < 0) || (aIndex >= _routeMgr->size())) {
    throw sg_range_exception("GPS::removeWaypointAtIndex: index out of bounds");
  }
  
  _routeMgr->pop_waypoint(aIndex);
}

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
