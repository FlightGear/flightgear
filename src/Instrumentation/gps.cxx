// gps.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <Aircraft/aircraft.hxx>

#include <simgear/route/route.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/sg_inlines.h>
#include <Airports/simple.hxx>

#include <Main/fg_init.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <Navaids/fixlist.hxx>
#include <Navaids/navlist.hxx>

#include "gps.hxx"

SG_USING_STD(string);


GPS::GPS ()
    : _last_valid(false),
      _last_longitude_deg(0),
      _last_latitude_deg(0),
      _last_altitude_m(0),
      _last_speed_kts(0)
{
}

GPS::~GPS ()
{
}

void
GPS::init ()
{
    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);
    _magvar_node = fgGetNode("/environment/magnetic-variation-deg", true);
    _serviceable_node = fgGetNode("/instrumentation/gps/serviceable", true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/gps", true);
    _wp0_longitude_node = 
        fgGetNode("/instrumentation/gps/wp/wp[0]/longitude-deg", true);
    _wp0_latitude_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/latitude-deg", true);
    _wp0_altitude_node =
	fgGetNode("/instrumentation/gps/wp/wp[0]/altitude-ft", true);
    _wp0_ID_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/ID", true);
    _wp0_name_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/name", true);
    _wp0_course_node = 
        fgGetNode("/instrumentation/gps/wp/wp[0]/desired-course-deg", true);
    _wp0_waypoint_type_node =
	fgGetNode("/instrumentation/gps/wp/wp[0]/waypoint-type", true);
    _wp1_longitude_node = 
        fgGetNode("/instrumentation/gps/wp/wp[1]/longitude-deg", true);
    _wp1_latitude_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/latitude-deg", true);
    _wp1_altitude_node =
	fgGetNode("/instrumentation/gps/wp/wp[1]/altitude-ft", true);
    _wp1_ID_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/ID", true);
    _wp1_name_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/name", true);
    _wp1_course_node = 
        fgGetNode("/instrumentation/gps/wp/wp[1]/desired-course-deg", true);
    _wp1_waypoint_type_node =
	fgGetNode("/instrumentation/gps/wp/wp[1]/waypoint-type", true);
    _get_nearest_airport_node =
	fgGetNode("/instrumentation/gps/wp/wp[1]/get-nearest-airport", true);
    _tracking_bug_node =
	fgGetNode("/instrumentation/gps/tracking-bug", true);

    _raim_node = fgGetNode("/instrumentation/gps/raim", true);
    _indicated_longitude_node =
        fgGetNode("/instrumentation/gps/indicated-longitude-deg", true);
    _indicated_latitude_node =
        fgGetNode("/instrumentation/gps/indicated-latitude-deg", true);
    _indicated_altitude_node =
        fgGetNode("/instrumentation/gps/indicated-altitude-ft", true);
    _indicated_vertical_speed_node =
	fgGetNode("/instrumentation/gps/indicated-vertical-speed", true);
    _true_track_node =
        fgGetNode("/instrumentation/gps/indicated-track-true-deg", true);
    _magnetic_track_node =
        fgGetNode("/instrumentation/gps/indicated-track-magnetic-deg", true);
    _speed_node =
        fgGetNode("/instrumentation/gps/indicated-ground-speed-kt", true);
    _wp0_distance_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/distance-nm", true);
    _wp0_ttw_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/TTW",true);
    _wp0_bearing_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/bearing-true-deg", true);
    _wp0_mag_bearing_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/bearing-mag-deg", true);
    _wp0_course_deviation_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/course-deviation-deg", true);
    _wp0_course_error_nm_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/course-error-nm", true);
    _wp0_to_flag_node =
        fgGetNode("/instrumentation/gps/wp/wp[0]/to-flag", true);
    _wp1_distance_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/distance-nm", true);
    _wp1_ttw_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/TTW",true);
    _wp1_bearing_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/bearing-true-deg", true);
    _wp1_mag_bearing_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/bearing-mag-deg", true);
    _wp1_course_deviation_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/course-deviation-deg", true);
    _wp1_course_error_nm_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/course-error-nm", true);
    _wp1_to_flag_node =
        fgGetNode("/instrumentation/gps/wp/wp[1]/to-flag", true);
    _odometer_node =
        fgGetNode("/instrumentation/gps/odometer", true);
    _trip_odometer_node =
        fgGetNode("/instrumentation/gps/trip-odometer", true);
    _true_bug_error_node =
	fgGetNode("/instrumentation/gps/true-bug-error-deg", true);
    _magnetic_bug_error_node =
        fgGetNode("/instrumentation/gps/magnetic-bug-error-deg", true);
    _true_wp0_bearing_error_node =
	fgGetNode("/instrumentation/gps/wp/wp[0]/true-bearing-error-deg", true);
    _magnetic_wp0_bearing_error_node =
	fgGetNode("/instrumentation/gps/wp/wp[0]/magnetic-bearing-error-deg", true);
    _true_wp1_bearing_error_node =
	fgGetNode("/instrumentation/gps/wp/wp[1]/true-bearing-error-deg", true);
    _magnetic_wp1_bearing_error_node =
	fgGetNode("/instrumentation/gps/wp/wp[1]/magnetic-bearing-error-deg", true);
    _leg_distance_node =
        fgGetNode("/instrumentation/gps/wp/leg-distance-nm", true);
    _leg_course_node =
        fgGetNode("/instrumentation/gps/wp/leg-true-course-deg", true);
    _leg_magnetic_course_node =
        fgGetNode("/instrumentation/gps/wp/leg-mag-course-deg", true);
    _alt_dist_ratio_node =
        fgGetNode("/instrumentation/gps/wp/alt-dist-ratio", true);
    _leg_course_deviation_node =
        fgGetNode("/instrumentation/gps/wp/leg-course-deviation-deg", true);
    _leg_course_error_nm_node =
        fgGetNode("/instrumentation/gps/wp/leg-course-error-nm", true);
    _leg_to_flag_node =
        fgGetNode("/instrumentation/gps/wp/leg-to-flag", true);
    _alt_deviation_node =
        fgGetNode("/instrumentation/gps/wp/alt-deviation-ft", true);
}

void
GPS::update (double delta_time_sec)
{
                                // If it's off, don't bother.
    if (!_serviceable_node->getBoolValue() ||
        !_electrical_node->getBoolValue()) {
        _last_valid = false;
        _last_longitude_deg = 0;
        _last_latitude_deg = 0;
        _last_altitude_m = 0;
        _last_speed_kts = 0;
        _raim_node->setDoubleValue(false);
        _indicated_longitude_node->setDoubleValue(0);
        _indicated_latitude_node->setDoubleValue(0);
        _indicated_altitude_node->setDoubleValue(0);
	_indicated_vertical_speed_node->setDoubleValue(0);
        _true_track_node->setDoubleValue(0);
        _magnetic_track_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        _wp1_distance_node->setDoubleValue(0);
        _wp1_bearing_node->setDoubleValue(0);
        _wp1_longitude_node->setDoubleValue(0);
        _wp1_latitude_node->setDoubleValue(0);
        _wp1_course_node->setDoubleValue(0);
        _odometer_node->setDoubleValue(0);
        _trip_odometer_node->setDoubleValue(0);
        _tracking_bug_node->setDoubleValue(0);
        _true_bug_error_node->setDoubleValue(0);
        _magnetic_bug_error_node->setDoubleValue(0);
	_true_wp1_bearing_error_node->setDoubleValue(0);
	_magnetic_wp1_bearing_error_node->setDoubleValue(0);
        return;
    }

    // Get the aircraft position
    // TODO: Add noise and other errors.
    double longitude_deg = _longitude_node->getDoubleValue();
    double latitude_deg = _latitude_node->getDoubleValue();
    double altitude_m = _altitude_node->getDoubleValue() * SG_FEET_TO_METER;
    double magvar_deg = _magvar_node->getDoubleValue();

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



    double speed_kt, vertical_speed_mpm;

    _raim_node->setBoolValue(true);
    _indicated_longitude_node->setDoubleValue(longitude_deg);
    _indicated_latitude_node->setDoubleValue(latitude_deg);
    _indicated_altitude_node->setDoubleValue(altitude_m * SG_METER_TO_FEET);

    if (_last_valid) {
        double track1_deg, track2_deg, distance_m, odometer, mag_track_bearing;
        geo_inverse_wgs_84(altitude_m,
                           _last_latitude_deg, _last_longitude_deg,
                           latitude_deg, longitude_deg,
                           &track1_deg, &track2_deg, &distance_m);
        speed_kt = ((distance_m * SG_METER_TO_NM) *
                    ((1 / delta_time_sec) * 3600.0));
        vertical_speed_mpm = ((altitude_m - _last_altitude_m) * 60 /
			      delta_time_sec);
	_indicated_vertical_speed_node->setDoubleValue
            (vertical_speed_mpm * SG_METER_TO_FEET);
        _true_track_node->setDoubleValue(track1_deg);
	mag_track_bearing = track1_deg - magvar_deg;
        SG_NORMALIZE_RANGE(mag_track_bearing, 0.0, 360.0);
        _magnetic_track_node->setDoubleValue(mag_track_bearing);
        speed_kt = fgGetLowPass(_last_speed_kts, speed_kt, delta_time_sec/20.0);
        _last_speed_kts = speed_kt;
        _speed_node->setDoubleValue(speed_kt);

        odometer = _odometer_node->getDoubleValue();
        _odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);
        odometer = _trip_odometer_node->getDoubleValue();
        _trip_odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);

        // Get waypoint 0 position
        double wp0_longitude_deg = _wp0_longitude_node->getDoubleValue();
        double wp0_latitude_deg = _wp0_latitude_node->getDoubleValue();
	double wp0_altitude_m = _wp0_altitude_node->getDoubleValue() 
	    * SG_FEET_TO_METER;
        double wp0_course_deg = _wp0_course_node->getDoubleValue();
        double wp0_distance, wp0_bearing_deg, wp0_course_deviation_deg,
            wp0_course_error_m, wp0_TTW, wp0_bearing_error_deg;
        string wp0_ID = _wp0_ID_node->getStringValue();

        // Get waypoint 1 position
        double wp1_longitude_deg = _wp1_longitude_node->getDoubleValue();
        double wp1_latitude_deg = _wp1_latitude_node->getDoubleValue();
	double wp1_altitude_m = _wp1_altitude_node->getDoubleValue() 
	    * SG_FEET_TO_METER;
        double wp1_course_deg = _wp1_course_node->getDoubleValue();
        double wp1_distance, wp1_bearing_deg, wp1_course_deviation_deg,
            wp1_course_error_m, wp1_TTW, wp1_bearing_error_deg;
        string wp1_ID = _wp1_ID_node->getStringValue();
        
        // If the get-nearest-airport-node is true.
        // Get the nearest airport, and set it as waypoint 1.
        if (_get_nearest_airport_node->getBoolValue()) {
            FGAirport a;
            //cout << "Airport found" << endl;
            a = globals->get_airports()->search(longitude_deg, latitude_deg, false);
            _wp1_ID_node->setStringValue(a.id.c_str());
            wp1_longitude_deg = a.longitude;
            wp1_latitude_deg = a.latitude;
            _wp1_name_node->setStringValue(a.name.c_str());
            _get_nearest_airport_node->setBoolValue(false);
            _last_wp1_ID = wp1_ID = a.id.c_str();
        }
        
        // If the waypoint 0 ID has changed, try to find the new ID
        // in the airport-, fix-, nav-database.
        if ( !(_last_wp0_ID == wp0_ID) ) {
            string waypont_type =
                _wp0_waypoint_type_node->getStringValue();
            if (waypont_type == "airport") {
                FGAirport a;
                a = globals->get_airports()->search( wp0_ID );
                if ( a.id == wp0_ID ) {
                    //cout << "Airport found" << endl;
                    wp0_longitude_deg = a.longitude;
                    wp0_latitude_deg = a.latitude;
                    _wp0_name_node->setStringValue(a.name.c_str());
                }
            }
            else if (waypont_type == "nav") {
                FGNavRecord *n
                    = globals->get_navlist()->findByIdent(wp0_ID.c_str(), 
                                                          longitude_deg, 
                                                          latitude_deg);
                if ( n != NULL ) {
                    //cout << "Nav found" << endl;
                    wp0_longitude_deg = n->get_lon();
                    wp0_latitude_deg = n->get_lat();
                    _wp0_name_node->setStringValue(n->get_name().c_str());
                }
            }
            else if (waypont_type == "fix") {
                FGFix f;
                if ( globals->get_fixlist()->query(wp0_ID, &f) ) {
                    //cout << "Fix found" << endl;
                    wp0_longitude_deg = f.get_lon();
                    wp0_latitude_deg = f.get_lat();
                    _wp0_name_node->setStringValue(wp0_ID.c_str());
                }
            }
            _last_wp0_ID = wp0_ID;
        }
        
        // If the waypoint 1 ID has changed, try to find the new ID
        // in the airport-, fix-, nav-database.
        if ( !(_last_wp1_ID == wp1_ID) ) {
            string waypont_type =
                _wp1_waypoint_type_node->getStringValue();
            if (waypont_type == "airport") {
                FGAirport a;
                a = globals->get_airports()->search( wp1_ID );
                if ( a.id == wp1_ID ) {
                    //cout << "Airport found" << endl;
                    wp1_longitude_deg = a.longitude;
                    wp1_latitude_deg = a.latitude;
                    _wp1_name_node->setStringValue(a.name.c_str());
                }
            }
            else if (waypont_type == "nav") {
                FGNavRecord *n
                    = globals->get_navlist()->findByIdent(wp1_ID.c_str(), 
                                                          longitude_deg, 
                                                          latitude_deg);
                if ( n != NULL ) {
                    //cout << "Nav found" << endl;
                    wp1_longitude_deg = n->get_lon();
                    wp1_latitude_deg = n->get_lat();
                    _wp1_name_node->setStringValue(n->get_name().c_str());
                }
            }
            else if (waypont_type == "fix") {
                FGFix f;
                if ( globals->get_fixlist()->query(wp1_ID, &f) ) {
                    //cout << "Fix found" << endl;
                    wp1_longitude_deg = f.get_lon();
                    wp1_latitude_deg = f.get_lat();
                    _wp1_name_node->setStringValue(wp1_ID.c_str());
                }
            }
            _last_wp1_ID = wp1_ID;
        }



        // If any of the two waypoints have changed
        // we need to calculate a new course between them,
        // and values for vertical navigation.
        if ( wp0_longitude_deg != _wp0_longitude_deg ||
             wp0_latitude_deg  != _wp0_latitude_deg  ||
             wp0_altitude_m    != _wp0_altitude_m    ||
             wp1_longitude_deg != _wp1_longitude_deg ||
             wp1_latitude_deg  != _wp1_latitude_deg  ||
             wp1_altitude_m    != _wp1_altitude_m )
        {
            // Update the global variables
            _wp0_longitude_deg = wp0_longitude_deg;
            _wp0_latitude_deg  = wp0_latitude_deg;
            _wp0_altitude_m    = wp0_altitude_m;
            _wp1_longitude_deg = wp1_longitude_deg;
            _wp1_latitude_deg  = wp1_latitude_deg;
            _wp1_altitude_m    = wp1_altitude_m;

            // Get the course and distance from wp0 to wp1
            SGWayPoint wp0(wp0_longitude_deg, 
                           wp0_latitude_deg, wp0_altitude_m);
            SGWayPoint wp1(wp1_longitude_deg, 
                           wp1_latitude_deg, wp1_altitude_m);

            wp1.CourseAndDistance(wp0, &_course_deg, &_distance_m);
            double leg_mag_course = _course_deg - magvar_deg;
            SG_NORMALIZE_RANGE(leg_mag_course, 0.0, 360.0);

            // Get the altitude / distance ratio
            if ( distance_m > 0.0 ) {
                double alt_difference_m = wp0_altitude_m - wp1_altitude_m;
                _alt_dist_ratio = alt_difference_m / _distance_m;
            }

            _leg_distance_node->setDoubleValue(_distance_m * SG_METER_TO_NM);
            _leg_course_node->setDoubleValue(_course_deg);
            _leg_magnetic_course_node->setDoubleValue(leg_mag_course);
            _alt_dist_ratio_node->setDoubleValue(_alt_dist_ratio);

            _wp0_longitude_node->setDoubleValue(wp0_longitude_deg);
            _wp0_latitude_node->setDoubleValue(wp0_latitude_deg);
            _wp1_longitude_node->setDoubleValue(wp1_longitude_deg);
            _wp1_latitude_node->setDoubleValue(wp1_latitude_deg);
        }


        // Find the bearing and distance to waypoint 0.
        SGWayPoint wp0(wp0_longitude_deg, wp0_latitude_deg, wp0_altitude_m);
        wp0.CourseAndDistance(longitude_deg, latitude_deg, altitude_m,
                              &wp0_bearing_deg, &wp0_distance);
        _wp0_distance_node->setDoubleValue(wp0_distance * SG_METER_TO_NM);
        _wp0_bearing_node->setDoubleValue(wp0_bearing_deg);
	double wp0_mag_bearing_deg = wp0_bearing_deg - magvar_deg;
        SG_NORMALIZE_RANGE(wp0_mag_bearing_deg, 0.0, 360.0);
	_wp0_mag_bearing_node->setDoubleValue(wp0_mag_bearing_deg);
        wp0_bearing_error_deg = track1_deg - wp0_bearing_deg;
        SG_NORMALIZE_RANGE(wp0_bearing_error_deg, -180.0, 180.0);
        _true_wp0_bearing_error_node->setDoubleValue(wp0_bearing_error_deg);
        
        // Estimate time to waypoint 0.
        // The estimation does not take track into consideration,
        // so if you are going away from the waypoint the TTW will
        // increase. Makes most sense when travelling directly towards
        // the waypoint.
        if (speed_kt > 0.0 && wp0_distance > 0.0) {
          wp0_TTW = (wp0_distance * SG_METER_TO_NM) / (speed_kt / 3600);
        }
        else {
          wp0_TTW = 0.0;
        }
        unsigned int wp0_TTW_seconds = (int) (wp0_TTW + 0.5);
        if (wp0_TTW_seconds < 356400) { // That's 99 hours
          unsigned int wp0_TTW_minutes = 0;
          unsigned int wp0_TTW_hours   = 0;
          char wp0_TTW_str[9];
          while (wp0_TTW_seconds >= 3600) {
            wp0_TTW_seconds -= 3600;
            wp0_TTW_hours++;
          }
          while (wp0_TTW_seconds >= 60) {
            wp0_TTW_seconds -= 60;
            wp0_TTW_minutes++;
          }
          snprintf(wp0_TTW_str, 9, "%02d:%02d:%02d",
            wp0_TTW_hours, wp0_TTW_minutes, wp0_TTW_seconds);
          _wp0_ttw_node->setStringValue(wp0_TTW_str);
        }
        else
          _wp0_ttw_node->setStringValue("--:--:--");

        // Course deviation is the diffenrence between the bearing
        // and the course.
        wp0_course_deviation_deg = wp0_bearing_deg -
            wp0_course_deg;
        SG_NORMALIZE_RANGE(wp0_course_deviation_deg, -180.0, 180.0);

        // If the course deviation is less than 90 degrees to either side,
        // our desired course is towards the waypoint.
        // It does not matter if we are actually moving 
        // towards or from the waypoint.
        if (fabs(wp0_course_deviation_deg) < 90.0) {
            _wp0_to_flag_node->setBoolValue(true); }
        // If it's more than 90 degrees the desired
        // course is from the waypoint.
        else if (fabs(wp0_course_deviation_deg) > 90.0) {
          _wp0_to_flag_node->setBoolValue(false);
          // When the course is away from the waypoint, 
          // it makes sense to change the sign of the deviation.
          wp0_course_deviation_deg *= -1.0;
          SG_NORMALIZE_RANGE(wp0_course_deviation_deg, -90.0, 90.0);
	}

        _wp0_course_deviation_node->setDoubleValue(wp0_course_deviation_deg);

        // Cross track error.
        wp0_course_error_m = sin(wp0_course_deviation_deg * SG_PI / 180.0)
          * (wp0_distance);
        _wp0_course_error_nm_node->setDoubleValue(wp0_course_error_m 
                                                 * SG_METER_TO_NM);



        // Find the bearing and distance to waypoint 1.
        SGWayPoint wp1(wp1_longitude_deg, wp1_latitude_deg, wp1_altitude_m);
        wp1.CourseAndDistance(longitude_deg, latitude_deg, altitude_m,
                              &wp1_bearing_deg, &wp1_distance);
        _wp1_distance_node->setDoubleValue(wp1_distance * SG_METER_TO_NM);
        _wp1_bearing_node->setDoubleValue(wp1_bearing_deg);
	double wp1_mag_bearing_deg = wp1_bearing_deg - magvar_deg;
        SG_NORMALIZE_RANGE(wp1_mag_bearing_deg, 0.0, 360.0);
	_wp1_mag_bearing_node->setDoubleValue(wp1_mag_bearing_deg);
        wp1_bearing_error_deg = track1_deg - wp1_bearing_deg;
        SG_NORMALIZE_RANGE(wp1_bearing_error_deg, -180.0, 180.0);
        _true_wp1_bearing_error_node->setDoubleValue(wp1_bearing_error_deg);
        
        // Estimate time to waypoint 1.
        // The estimation does not take track into consideration,
        // so if you are going away from the waypoint the TTW will
        // increase. Makes most sense when travelling directly towards
        // the waypoint.
        if (speed_kt > 0.0 && wp1_distance > 0.0) {
          wp1_TTW = (wp1_distance * SG_METER_TO_NM) / (speed_kt / 3600);
        }
        else {
          wp1_TTW = 0.0;
        }
        unsigned int wp1_TTW_seconds = (int) (wp1_TTW + 0.5);
        if (wp1_TTW_seconds < 356400) { // That's 99 hours
          unsigned int wp1_TTW_minutes = 0;
          unsigned int wp1_TTW_hours   = 0;
          char wp1_TTW_str[9];
          while (wp1_TTW_seconds >= 3600) {
            wp1_TTW_seconds -= 3600;
            wp1_TTW_hours++;
          }
          while (wp1_TTW_seconds >= 60) {
            wp1_TTW_seconds -= 60;
            wp1_TTW_minutes++;
          }
          snprintf(wp1_TTW_str, 9, "%02d:%02d:%02d",
            wp1_TTW_hours, wp1_TTW_minutes, wp1_TTW_seconds);
          _wp1_ttw_node->setStringValue(wp1_TTW_str);
        }
        else
          _wp1_ttw_node->setStringValue("--:--:--");

        // Course deviation is the diffenrence between the bearing
        // and the course.
        wp1_course_deviation_deg = wp1_bearing_deg - wp1_course_deg;
        SG_NORMALIZE_RANGE(wp1_course_deviation_deg, -180.0, 180.0);

        // If the course deviation is less than 90 degrees to either side,
        // our desired course is towards the waypoint.
        // It does not matter if we are actually moving 
        // towards or from the waypoint.
        if (fabs(wp1_course_deviation_deg) < 90.0) {
            _wp1_to_flag_node->setBoolValue(true); }
        // If it's more than 90 degrees the desired
        // course is from the waypoint.
        else if (fabs(wp1_course_deviation_deg) > 90.0) {
            _wp1_to_flag_node->setBoolValue(false);
            // When the course is away from the waypoint, 
            // it makes sense to change the sign of the deviation.
            wp1_course_deviation_deg *= -1.0;
            SG_NORMALIZE_RANGE(wp1_course_deviation_deg, -90.0, 90.0);
	}

        _wp1_course_deviation_node->setDoubleValue(wp1_course_deviation_deg);

        // Cross track error.
        wp1_course_error_m = sin(wp1_course_deviation_deg * SG_PI / 180.0) 
          * (wp1_distance);
        _wp1_course_error_nm_node->setDoubleValue(wp1_course_error_m 
                                                 * SG_METER_TO_NM);


        // Leg course deviation is the diffenrence between the bearing
        // and the course.
        double course_deviation_deg = wp1_bearing_deg - _course_deg;
        SG_NORMALIZE_RANGE(course_deviation_deg, -180.0, 180.0);
        
        // If the course deviation is less than 90 degrees to either side,
        // our desired course is towards the waypoint.
        // It does not matter if we are actually moving 
        // towards or from the waypoint.
        if (fabs(course_deviation_deg) < 90.0) {
            _leg_to_flag_node->setBoolValue(true); }
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

        // Altitude deviation
        double desired_altitude_m = wp1_altitude_m
            + wp1_distance * _alt_dist_ratio;
        double altitude_deviation_m = altitude_m - desired_altitude_m;
        _alt_deviation_node->setDoubleValue(altitude_deviation_m * SG_METER_TO_FEET);



        // Tracking bug.
	double tracking_bug = _tracking_bug_node->getDoubleValue();
	double true_bug_error = tracking_bug - track1_deg;
	double magnetic_bug_error = tracking_bug - mag_track_bearing;

        // Get the errors into the (-180,180) range.
        SG_NORMALIZE_RANGE(true_bug_error, -180.0, 180.0);
        SG_NORMALIZE_RANGE(magnetic_bug_error, -180.0, 180.0);

	_true_bug_error_node->setDoubleValue(true_bug_error);
	_magnetic_bug_error_node->setDoubleValue(magnetic_bug_error);

    } else {
        _true_track_node->setDoubleValue(0.0);
        _magnetic_track_node->setDoubleValue(0.0);
        _speed_node->setDoubleValue(0.0);
    }

    _last_valid = true;
    _last_longitude_deg = longitude_deg;
    _last_latitude_deg = latitude_deg;
    _last_altitude_m = altitude_m;
}

// end of gps.cxx
