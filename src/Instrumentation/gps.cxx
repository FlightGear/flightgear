// gps.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gps.hxx"

#include <simgear/compiler.h>
#include <Aircraft/aircraft.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx> // for fgLowPass
#include <Navaids/positioned.hxx>

#include <simgear/math/sg_random.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>

using std::string;

 
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


GPS::GPS ( SGPropertyNode *node)
    : _last_valid(false),
      _alt_dist_ratio(0),
      _distance_m(0),
      _course_deg(0),
      _name(node->getStringValue("name", "gps")),
      _num(node->getIntValue("number", 0))
{
}

GPS::~GPS ()
{
}

void
GPS::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _position.init("/position/longitude-deg", "/position/latitude-deg", "/position/altitude-ft");
    _magvar_node = fgGetNode("/environment/magnetic-variation-deg", true);
    _serviceable_node = node->getChild("serviceable", 0, true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/gps", true);

    SGPropertyNode *wp_node = node->getChild("wp", 0, true);
    SGPropertyNode *wp0_node = wp_node->getChild("wp", 0, true);
    SGPropertyNode *wp1_node = wp_node->getChild("wp", 1, true);

    _wp0_position.init(wp0_node, "longitude-deg", "latitude-deg", "altitude-ft");
    _wp0_ID_node = wp0_node->getChild("ID", 0, true);
    _wp0_name_node = wp0_node->getChild("name", 0, true);
    _wp0_course_node = wp0_node->getChild("desired-course-deg", 0, true);
    _wp0_distance_node = wp0_node->getChild("distance-nm", 0, true);
    _wp0_ttw_node = wp0_node->getChild("TTW", 0, true);
    _wp0_bearing_node = wp0_node->getChild("bearing-true-deg", 0, true);
    _wp0_mag_bearing_node = wp0_node->getChild("bearing-mag-deg", 0, true);
    _wp0_course_deviation_node =
        wp0_node->getChild("course-deviation-deg", 0, true);
    _wp0_course_error_nm_node = wp0_node->getChild("course-error-nm", 0, true);
    _wp0_to_flag_node = wp0_node->getChild("to-flag", 0, true);
    _true_wp0_bearing_error_node =
        wp0_node->getChild("true-bearing-error-deg", 0, true);
    _magnetic_wp0_bearing_error_node =
        wp0_node->getChild("magnetic-bearing-error-deg", 0, true);

    _wp1_position.init(wp1_node, "longitude-deg", "latitude-deg", "altitude-ft");
    _wp1_ID_node = wp1_node->getChild("ID", 0, true);
    _wp1_name_node = wp1_node->getChild("name", 0, true);
    _wp1_course_node = wp1_node->getChild("desired-course-deg", 0, true);
    _wp1_distance_node = wp1_node->getChild("distance-nm", 0, true);
    _wp1_ttw_node = wp1_node->getChild("TTW", 0, true);
    _wp1_bearing_node = wp1_node->getChild("bearing-true-deg", 0, true);
    _wp1_mag_bearing_node = wp1_node->getChild("bearing-mag-deg", 0, true);
    _wp1_course_deviation_node =
        wp1_node->getChild("course-deviation-deg", 0, true);
    _wp1_course_error_nm_node = wp1_node->getChild("course-error-nm", 0, true);
    _wp1_to_flag_node = wp1_node->getChild("to-flag", 0, true);
    _true_wp1_bearing_error_node =
        wp1_node->getChild("true-bearing-error-deg", 0, true);
    _magnetic_wp1_bearing_error_node =
        wp1_node->getChild("magnetic-bearing-error-deg", 0, true);
    _get_nearest_airport_node = 
        wp1_node->getChild("get-nearest-airport", 0, true);

    _tracking_bug_node = node->getChild("tracking-bug", 0, true);
    _raim_node = node->getChild("raim", 0, true);

    _indicated_pos.init(node, "indicated-longitude-deg", 
        "indicated-latitude-deg", "indicated-altitude-ft");
        
    _indicated_vertical_speed_node =
        node->getChild("indicated-vertical-speed", 0, true);
    _true_track_node =
        node->getChild("indicated-track-true-deg", 0, true);
    _magnetic_track_node =
        node->getChild("indicated-track-magnetic-deg", 0, true);
    _speed_node =
        node->getChild("indicated-ground-speed-kt", 0, true);
    _odometer_node =
        node->getChild("odometer", 0, true);
    _trip_odometer_node =
        node->getChild("trip-odometer", 0, true);
    _true_bug_error_node =
        node->getChild("true-bug-error-deg", 0, true);
    _magnetic_bug_error_node =
        node->getChild("magnetic-bug-error-deg", 0, true);

    _leg_distance_node =
        wp_node->getChild("leg-distance-nm", 0, true);
    _leg_course_node =
        wp_node->getChild("leg-true-course-deg", 0, true);
    _leg_magnetic_course_node =
        wp_node->getChild("leg-mag-course-deg", 0, true);
    _alt_dist_ratio_node =
        wp_node->getChild("alt-dist-ratio", 0, true);
    _leg_course_deviation_node =
        wp_node->getChild("leg-course-deviation-deg", 0, true);
    _leg_course_error_nm_node =
        wp_node->getChild("leg-course-error-nm", 0, true);
    _leg_to_flag_node =
        wp_node->getChild("leg-to-flag", 0, true);
    _alt_deviation_node =
        wp_node->getChild("alt-deviation-ft", 0, true);
        
    _serviceable_node->setBoolValue(true);
}

void
GPS::clearOutput()
{
    _last_valid = false;
    _last_speed_kts = 0;
    _last_pos = SGGeod();
    _raim_node->setDoubleValue(false);
    _indicated_pos = SGGeod();
	  _indicated_vertical_speed_node->setDoubleValue(0);
    _true_track_node->setDoubleValue(0);
    _magnetic_track_node->setDoubleValue(0);
    _speed_node->setDoubleValue(0);
    _wp1_distance_node->setDoubleValue(0);
    _wp1_bearing_node->setDoubleValue(0);
    _wp1_position = SGGeod();
    _wp1_course_node->setDoubleValue(0);
    _odometer_node->setDoubleValue(0);
    _trip_odometer_node->setDoubleValue(0);
    _tracking_bug_node->setDoubleValue(0);
    _true_bug_error_node->setDoubleValue(0);
    _magnetic_bug_error_node->setDoubleValue(0);
	  _true_wp1_bearing_error_node->setDoubleValue(0);
	  _magnetic_wp1_bearing_error_node->setDoubleValue(0);
}

void
GPS::update (double delta_time_sec)
{
   // If it's off, don't bother.
    if (!_serviceable_node->getBoolValue() || !_electrical_node->getBoolValue()) {
        clearOutput();
        return;
    }

    UpdateContext ctx;
    ctx.dt = delta_time_sec;
    ctx.waypoint_changed = false;
    ctx.pos = _position.get();
    
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
    _raim_node->setBoolValue(true);
    _indicated_pos = ctx.pos;

    if (_last_valid) {
        updateWithValid(ctx);
    } else {
        _true_track_node->setDoubleValue(0.0);
        _magnetic_track_node->setDoubleValue(0.0);
        _speed_node->setDoubleValue(0.0);
        _last_valid = true;
    }

    _last_pos = ctx.pos;
}

void
GPS::updateNearestAirport(UpdateContext& ctx)
{
    if (!_get_nearest_airport_node->getBoolValue()) {
        return;
    }
    
    // If the get-nearest-airport-node is true.
    // Get the nearest airport, and set it as waypoint 1.
    
    FGPositioned::TypeFilter aptFilter(FGPositioned::AIRPORT);
    FGPositionedRef a = FGPositioned::findClosest(ctx.pos, 360.0, &aptFilter);
    if (!a) {
        return;
    }

    _wp1_position = a->geod();
    _wp1_ID_node->setStringValue(a->ident().c_str());
    _wp1_name_node->setStringValue(a->name().c_str());
    _get_nearest_airport_node->setBoolValue(false);
    _last_wp1_ID = a->ident(); // don't trigger updateWaypoint1();
    ctx.waypoint_changed = true;
}

void
GPS::updateWithValid(UpdateContext& ctx)
{
    assert(_last_valid);
    double distance_m;
    SGGeodesy::inverse(_last_pos, ctx.pos, ctx.track1_deg, ctx.track2_deg, distance_m );
    
    ctx.speed_kt = ((distance_m * SG_METER_TO_NM) * ((1 / ctx.dt) * 3600.0));
    
    double vertical_speed_mpm = ((ctx.pos.getElevationM() - _last_pos.getElevationM()) * 60 /
			      ctx.dt);
	  _indicated_vertical_speed_node->setDoubleValue(vertical_speed_mpm * SG_METER_TO_FEET);
    _true_track_node->setDoubleValue(ctx.track1_deg);
    
    ctx.magvar_deg = _magvar_node->getDoubleValue();
    double mag_track_bearing = ctx.track1_deg - ctx.magvar_deg;
    SG_NORMALIZE_RANGE(mag_track_bearing, 0.0, 360.0);
    _magnetic_track_node->setDoubleValue(mag_track_bearing);
    ctx.speed_kt = fgGetLowPass(_last_speed_kts, ctx.speed_kt, ctx.dt/20.0);
    _last_speed_kts = ctx.speed_kt;
    _speed_node->setDoubleValue(ctx.speed_kt);

    double odometer = _odometer_node->getDoubleValue();
    _odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);
    odometer = _trip_odometer_node->getDoubleValue();
    _trip_odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);
  
    updateNearestAirport(ctx);
    updateWaypoint0(ctx);
    updateWaypoint1(ctx);

    ctx.wp0_pos = _wp0_position.get();
    ctx.wp1_pos = _wp1_position.get();
    // if this flag is set, we need to recompute leg data, because either
    // WP0 or WP1 has been updated
    if (ctx.waypoint_changed) {
      waypointChanged(ctx);
    }

    ctx.wp0_course_deg = _wp0_course_node->getDoubleValue();
    ctx.wp1_course_deg = _wp1_course_node->getDoubleValue();
    
    updateWaypoint0Course(ctx);
    updateWaypoint1Course(ctx);
    updateLegCourse(ctx);
  
    // Altitude deviation
    //double desired_altitude_m = wp1_altitude_m
    //        + wp1_distance * _alt_dist_ratio;
    //double altitude_deviation_m = altitude_m - desired_altitude_m;
    //    _alt_deviation_node->setDoubleValue(altitude_deviation_m * SG_METER_TO_FEET);
    
    updateTrackingBug(ctx);
}

void
GPS::updateLegCourse(UpdateContext& ctx)
{
     // Leg course deviation is the diffenrence between the bearing
    // and the course.
    double course_deviation_deg = ctx.wp1_bearing_deg - _course_deg;
    SG_NORMALIZE_RANGE(course_deviation_deg, -180.0, 180.0);
        
    // If the course deviation is less than 90 degrees to either side,
    // our desired course is towards the waypoint.
    // It does not matter if we are actually moving 
    // towards or from the waypoint.
    if (fabs(course_deviation_deg) < 90.0) {
        _leg_to_flag_node->setBoolValue(true); 
    }
    // If it's more than 90 degrees the desired
    // course is from the waypoint.
    else if (fabs(course_deviation_deg) > 90.0) {
        _leg_to_flag_node->setBoolValue(false);
        // When the course is away from the waypoint, 
        // it makes sense to change the sign of the deviation.
        course_deviation_deg *= -1.0;
        SG_NORMALIZE_RANGE(course_deviation_deg, -90.0, 90.0);
    }
    
    _leg_course_deviation_node->setDoubleValue(course_deviation_deg);
        
    // Cross track error.
    double course_error_m = sin(course_deviation_deg * SG_PI / 180.0)
            * (_distance_m);
    _leg_course_error_nm_node->setDoubleValue(course_error_m * SG_METER_TO_NM);

}

void
GPS::updateTrackingBug(UpdateContext& ctx)
{
    double tracking_bug = _tracking_bug_node->getDoubleValue();
    double true_bug_error = tracking_bug - ctx.track1_deg;
    double magnetic_bug_error = tracking_bug - _magnetic_track_node->getDoubleValue();

    // Get the errors into the (-180,180) range.
    SG_NORMALIZE_RANGE(true_bug_error, -180.0, 180.0);
    SG_NORMALIZE_RANGE(magnetic_bug_error, -180.0, 180.0);

    _true_bug_error_node->setDoubleValue(true_bug_error);
    _magnetic_bug_error_node->setDoubleValue(magnetic_bug_error);
}

void
GPS::waypointChanged(UpdateContext& ctx)
{
    // If any of the two waypoints have changed
    // we need to calculate a new course between them,
    // and values for vertical navigation.
    assert(ctx.waypoint_changed);

    double track2;
    SGGeodesy::inverse(ctx.wp0_pos, ctx.wp1_pos, _course_deg, track2, _distance_m);
    
    double leg_mag_course = _course_deg - _magvar_node->getDoubleValue();
    SG_NORMALIZE_RANGE(leg_mag_course, 0.0, 360.0);

    // Get the altitude / distance ratio
    if ( _distance_m > 0.0 ) {
        double alt_difference_m = ctx.wp0_pos.getElevationM() - ctx.wp1_pos.getElevationM();
        _alt_dist_ratio = alt_difference_m / _distance_m;
    }

    _leg_distance_node->setDoubleValue(_distance_m * SG_METER_TO_NM);
    _leg_course_node->setDoubleValue(_course_deg);
    _leg_magnetic_course_node->setDoubleValue(leg_mag_course);
    _alt_dist_ratio_node->setDoubleValue(_alt_dist_ratio);
}

void
GPS::updateWaypoint0(UpdateContext& ctx)
{
    string id(_wp0_ID_node->getStringValue());
    if (_last_wp0_ID == id) {
        return; // easy, nothing to do
    }
    
    FGPositionedRef result = FGPositioned::findClosestWithIdent(id, ctx.pos);
    if (!result) {
        // not found, hmm
        _last_wp0_ID = id;
        return;
    }
    
    _wp0_position = result->geod();
    _wp0_name_node->setStringValue(result->name().c_str());
    _last_wp0_ID = id;
    ctx.waypoint_changed = true;
}

void
GPS::updateWaypoint1(UpdateContext& ctx)
{
    string id(_wp1_ID_node->getStringValue());
    if (_last_wp1_ID == id) {
        return; // easy, nothing to do
    }
    
    FGPositionedRef result = FGPositioned::findClosestWithIdent(id, ctx.pos);
    if (!result) {
        // not found, hmm
        _last_wp1_ID = id;
        return;
    }
    
    _wp1_position = result->geod();
    _wp1_name_node->setStringValue(result->name().c_str());
    _last_wp1_ID = id;
    ctx.waypoint_changed = true;
}

void
GPS::updateTTWNode(UpdateContext& ctx, double distance_m, SGPropertyNode_ptr node)
{
    // Estimate time to waypoint.
    // The estimation does not take track into consideration,
    // so if you are going away from the waypoint the TTW will
    // increase. Makes most sense when travelling directly towards
    // the waypoint.
    double TTW = 0.0;
    double speed_nm_per_second = ctx.speed_kt / 3600;
    if (speed_nm_per_second > SGLimitsd::min() && distance_m > 0.0) {
        TTW = (distance_m * SG_METER_TO_NM) / speed_nm_per_second;
    }
    if (TTW < 356400.5) { // That's 99 hours
      unsigned int TTW_seconds = (int) (TTW + 0.5);
      unsigned int TTW_minutes = 0;
      unsigned int TTW_hours   = 0;
      char TTW_str[9];
      TTW_hours   = TTW_seconds / 3600;
      TTW_minutes = (TTW_seconds / 60) % 60;
      TTW_seconds = TTW_seconds % 60;
      snprintf(TTW_str, 9, "%02d:%02d:%02d",
        TTW_hours, TTW_minutes, TTW_seconds);
      node->setStringValue(TTW_str);
    } else {
      node->setStringValue("--:--:--");
    }
}

void
GPS::updateWaypoint0Course(UpdateContext& ctx)
{
    // Find the bearing and distance to waypoint 0.
    double az2;
    SGGeodesy::inverse(ctx.pos, ctx.wp0_pos, ctx.wp0_bearing_deg, az2,ctx.wp0_distance);
    _wp0_distance_node->setDoubleValue(ctx.wp0_distance * SG_METER_TO_NM);
    _wp0_bearing_node->setDoubleValue(ctx.wp0_bearing_deg);
        
    double mag_bearing_deg = ctx.wp0_bearing_deg - ctx.magvar_deg;
    SG_NORMALIZE_RANGE(mag_bearing_deg, 0.0, 360.0);
    _wp0_mag_bearing_node->setDoubleValue(mag_bearing_deg);
    double bearing_error_deg = ctx.track1_deg - ctx.wp0_bearing_deg;
    SG_NORMALIZE_RANGE(bearing_error_deg, -180.0, 180.0);
    _true_wp0_bearing_error_node->setDoubleValue(bearing_error_deg);
    
    updateTTWNode(ctx, ctx.wp0_distance, _wp0_ttw_node);
        
    // Course deviation is the diffenrence between the bearing
    // and the course.
    double course_deviation_deg = ctx.wp0_bearing_deg -
        ctx.wp0_course_deg;
    SG_NORMALIZE_RANGE(course_deviation_deg, -180.0, 180.0);

    // If the course deviation is less than 90 degrees to either side,
    // our desired course is towards the waypoint.
    // It does not matter if we are actually moving 
    // towards or from the waypoint.
    if (fabs(course_deviation_deg) < 90.0) {
        _wp0_to_flag_node->setBoolValue(true); 
    }
    // If it's more than 90 degrees the desired
    // course is from the waypoint.
    else if (fabs(course_deviation_deg) > 90.0) {
      _wp0_to_flag_node->setBoolValue(false);
      // When the course is away from the waypoint, 
      // it makes sense to change the sign of the deviation.
      course_deviation_deg *= -1.0;
      SG_NORMALIZE_RANGE(course_deviation_deg, -90.0, 90.0);
    }

    _wp0_course_deviation_node->setDoubleValue(course_deviation_deg);

    // Cross track error.
    double course_error_m = sin(course_deviation_deg * SG_PI / 180.0)
          * (ctx.wp0_distance);
    _wp0_course_error_nm_node->setDoubleValue(course_error_m * SG_METER_TO_NM);
}

void
GPS::updateWaypoint1Course(UpdateContext& ctx)
{
    // Find the bearing and distance to waypoint 0.
    double az2;
    SGGeodesy::inverse(ctx.pos, ctx.wp1_pos, ctx.wp1_bearing_deg, az2,ctx.wp1_distance);
    _wp1_distance_node->setDoubleValue(ctx.wp1_distance * SG_METER_TO_NM);
    _wp1_bearing_node->setDoubleValue(ctx.wp1_bearing_deg);
        
    double mag_bearing_deg = ctx.wp1_bearing_deg - ctx.magvar_deg;
    SG_NORMALIZE_RANGE(mag_bearing_deg, 0.0, 360.0);
    _wp1_mag_bearing_node->setDoubleValue(mag_bearing_deg);
    double bearing_error_deg = ctx.track1_deg - ctx.wp1_bearing_deg;
    SG_NORMALIZE_RANGE(bearing_error_deg, -180.0, 180.0);
    _true_wp1_bearing_error_node->setDoubleValue(bearing_error_deg);
    
    updateTTWNode(ctx, ctx.wp1_distance, _wp1_ttw_node);
        
    // Course deviation is the diffenrence between the bearing
    // and the course.
    double course_deviation_deg = ctx.wp1_bearing_deg -
        ctx.wp1_course_deg;
    SG_NORMALIZE_RANGE(course_deviation_deg, -180.0, 180.0);

    // If the course deviation is less than 90 degrees to either side,
    // our desired course is towards the waypoint.
    // It does not matter if we are actually moving 
    // towards or from the waypoint.
    if (fabs(course_deviation_deg) < 90.0) {
        _wp1_to_flag_node->setBoolValue(true); 
    }
    // If it's more than 90 degrees the desired
    // course is from the waypoint.
    else if (fabs(course_deviation_deg) > 90.0) {
      _wp1_to_flag_node->setBoolValue(false);
      // When the course is away from the waypoint, 
      // it makes sense to change the sign of the deviation.
      course_deviation_deg *= -1.0;
      SG_NORMALIZE_RANGE(course_deviation_deg, -90.0, 90.0);
    }

    _wp1_course_deviation_node->setDoubleValue(course_deviation_deg);

    // Cross track error.
    double course_error_m = sin(course_deviation_deg * SG_PI / 180.0)
          * (ctx.wp1_distance);
    _wp1_course_error_nm_node->setDoubleValue(course_error_m * SG_METER_TO_NM);

}

// end of gps.cxx
