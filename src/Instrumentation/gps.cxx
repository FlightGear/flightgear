// gps.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <string.h>

#include STL_STRING

#include <Aircraft/aircraft.hxx>
#include <FDM/flight.hxx>
#include <Controls/controls.hxx>
#include <Scenery/scenery.hxx>

#include <simgear/constants.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/route/route.hxx>

#include <Airports/simple.hxx>
#include <GUI/gui.h>
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
    _wp_longitude_node = 
        fgGetNode("/instrumentation/gps/wp-longitude-deg", true);
    _wp_latitude_node =
        fgGetNode("/instrumentation/gps/wp-latitude-deg", true);
    _wp_ID_node =
        fgGetNode("/instrumentation/gps/wp-ID", true);
    _wp_name_node =
        fgGetNode("/instrumentation/gps/wp-name", true);
    _wp_course_node = 
        fgGetNode("/instrumentation/gps/indicated-course-deg", true);
    _get_nearest_airport_node =
      fgGetNode("/instrumentation/gps/get-nearest-airport", true);
    _waypoint_type_node =
      fgGetNode("/instrumentation/gps/waypoint-type", true);

    _raim_node = fgGetNode("/instrumentation/gps/raim", true);
    _indicated_longitude_node =
        fgGetNode("/instrumentation/gps/indicated-longitude-deg", true);
    _indicated_latitude_node =
        fgGetNode("/instrumentation/gps/indicated-latitude-deg", true);
    _indicated_altitude_node =
        fgGetNode("/instrumentation/gps/indicated-altitude-ft", true);
    _true_track_node =
        fgGetNode("/instrumentation/gps/indicated-track-true-deg", true);
    _magnetic_track_node =
        fgGetNode("/instrumentation/gps/indicated-track-magnetic-deg", true);
    _speed_node =
        fgGetNode("/instrumentation/gps/indicated-ground-speed-kt", true);
    _wp_distance_node =
        fgGetNode("/instrumentation/gps/wp-distance-nm", true);
    _wp_ttw_node =
        fgGetNode("/instrumentation/gps/TTW",true);
    _wp_bearing_node =
        fgGetNode("/instrumentation/gps/wp-bearing-deg", true);
    _wp_course_deviation_node =
        fgGetNode("/instrumentation/gps/course-deviation-deg", true);
    _wp_course_error_nm_node =
        fgGetNode("/instrumentation/gps/course-error-nm", true);
    _wp_to_flag_node =
        fgGetNode("/instrumentation/gps/to-flag", true);
    _odometer_node =
        fgGetNode("/instrumentation/gps/odometer", true);
    _trip_odometer_node =
      fgGetNode("/instrumentation/gps/trip-odometer", true);
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
        _true_track_node->setDoubleValue(0);
        _magnetic_track_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        _wp_distance_node->setDoubleValue(0);
        _wp_bearing_node->setDoubleValue(0);
        _wp_longitude_node->setDoubleValue(0);
        _wp_latitude_node->setDoubleValue(0);
        _wp_course_node->setDoubleValue(0);
        _odometer_node->setDoubleValue(0);
        _trip_odometer_node->setDoubleValue(0);
        return;
    }

                                // Get the aircraft position
    double longitude_deg = _longitude_node->getDoubleValue();
    double latitude_deg = _latitude_node->getDoubleValue();
    double altitude_m = _altitude_node->getDoubleValue() * SG_FEET_TO_METER;
    double magvar_deg = _magvar_node->getDoubleValue();

    double speed_kt;

    _raim_node->setBoolValue(true);
    _indicated_longitude_node->setDoubleValue(longitude_deg);
    _indicated_latitude_node->setDoubleValue(latitude_deg);
    _indicated_altitude_node->setDoubleValue(altitude_m * SG_METER_TO_FEET);

    if (_last_valid) {
        double track1_deg, track2_deg, distance_m, odometer;
        geo_inverse_wgs_84(altitude_m,
                           _last_latitude_deg, _last_longitude_deg,
                           latitude_deg, longitude_deg,
                           &track1_deg, &track2_deg, &distance_m);
        speed_kt = ((distance_m * SG_METER_TO_NM) *
                    ((1 / delta_time_sec) * 3600.0));
        _true_track_node->setDoubleValue(track1_deg);
        _magnetic_track_node->setDoubleValue(track1_deg - magvar_deg);
        speed_kt = fgGetLowPass(_last_speed_kts, speed_kt, delta_time_sec/20.0);
        _last_speed_kts = speed_kt;
        _speed_node->setDoubleValue(speed_kt);

        odometer = _odometer_node->getDoubleValue();
        _odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);
        odometer = _trip_odometer_node->getDoubleValue();
        _trip_odometer_node->setDoubleValue(odometer + distance_m * SG_METER_TO_NM);

        // Get waypoint position
        double wp_longitude_deg = _wp_longitude_node->getDoubleValue();
        double wp_latitude_deg = _wp_latitude_node->getDoubleValue();
        double wp_course_deg = 
          _wp_course_node->getDoubleValue();
        double wp_distance, wp_bearing_deg, wp_course_deviation_deg,
            wp_course_error_m, wp_TTW;
        // double wp_actual_radial_deg;
        string wp_ID = _wp_ID_node->getStringValue();

        // If the get-nearest-airport-node is true.
        // Get the nearest airport, and set it as waypoint.
        if (_get_nearest_airport_node->getBoolValue()) {
          FGAirport a;
          cout << "Airport found" << endl;
          a = globals->get_airports()->search(longitude_deg, latitude_deg, false);
          _wp_ID_node->setStringValue(a.id.c_str());
          wp_longitude_deg = a.longitude;
          wp_latitude_deg = a.latitude;
          _wp_name_node->setStringValue(a.name.c_str());
          _get_nearest_airport_node->setBoolValue(false);
          _last_wp_ID = wp_ID = a.id.c_str();
        }

        // If the waypoint ID has changed, try to find the new ID
        // in the airport-, fix-, nav-database.
        if ( !(_last_wp_ID == wp_ID) ) {
          string waypont_type =
            _waypoint_type_node->getStringValue();
          if (waypont_type == "airport") {
            FGAirport a;
            a = globals->get_airports()->search( wp_ID );
            if ( a.id == wp_ID ) {
              cout << "Airport found" << endl;
              wp_longitude_deg = a.longitude;
              wp_latitude_deg = a.latitude;
              _wp_name_node->setStringValue(a.name.c_str());
            }
          }
          else if (waypont_type == "nav") {
            FGNav * n;
            if ( (n = current_navlist->findByIdent(wp_ID.c_str(), 
                                                      longitude_deg, 
                                                      latitude_deg)) != NULL) {
              cout << "Nav found" << endl;
              wp_longitude_deg = n->get_lon();
              wp_latitude_deg = n->get_lat();
              _wp_name_node->setStringValue(n->get_name().c_str());
            }
          }
          else if (waypont_type == "fix") {
            FGFix f;
            if ( current_fixlist->query(wp_ID, &f) ) {
              cout << "Fix found" << endl;
              wp_longitude_deg = f.get_lon();
              wp_latitude_deg = f.get_lat();
              _wp_name_node->setStringValue(wp_ID.c_str());
            }
          }
          _last_wp_ID = wp_ID;
        }

        // Find the bearing and distance to the waypoint.
        SGWayPoint wp(wp_longitude_deg, wp_latitude_deg, altitude_m);
        wp.CourseAndDistance(longitude_deg, latitude_deg, altitude_m,
                            &wp_bearing_deg, &wp_distance);
        _wp_longitude_node->setDoubleValue(wp_longitude_deg);
        _wp_latitude_node->setDoubleValue(wp_latitude_deg);
        _wp_distance_node->setDoubleValue(wp_distance * SG_METER_TO_NM);
        _wp_bearing_node->setDoubleValue(wp_bearing_deg);
        
        // Estimate time to waypoint.
        // The estimation does not take track into consideration,
        // so if you are going away from the waypoint the TTW will
        // increase. Makes most sense when travelling directly towards
        // the waypoint.
        if (speed_kt > 0.0 && wp_distance > 0.0) {
          wp_TTW = (wp_distance * SG_METER_TO_NM) / (speed_kt / 3600);
        }
        else {
          wp_TTW = 0.0;
        }
        unsigned int wp_TTW_seconds = (int) (wp_TTW + 0.5);
        if (wp_TTW_seconds < 356400) { // That's 99 hours
          unsigned int wp_TTW_minutes = 0;
          unsigned int wp_TTW_hours   = 0;
          char wp_TTW_str[8];
          while (wp_TTW_seconds >= 3600) {
            wp_TTW_seconds -= 3600;
            wp_TTW_hours++;
          }
          while (wp_TTW_seconds >= 60) {
            wp_TTW_seconds -= 60;
            wp_TTW_minutes++;
          }
          sprintf(wp_TTW_str, "%02d:%02d:%02d",
            wp_TTW_hours, wp_TTW_minutes, wp_TTW_seconds);
          _wp_ttw_node->setStringValue(wp_TTW_str);
        }
        else
          _wp_ttw_node->setStringValue("--:--:--");

        // Course deviation is the diffenrence between the bearing
        // and the course.
        wp_course_deviation_deg = wp_bearing_deg -
          wp_course_deg;
        while (wp_course_deviation_deg < -180.0) {
          wp_course_deviation_deg += 360.0; }
        while (wp_course_deviation_deg > 180.0) {
          wp_course_deviation_deg -= 360.0; }

        // If the course deviation is less than 90 degrees to either side,
        // our desired course is towards the waypoint.
        // It does not matter if we are actually moving towards or from the waypoint.
        if (fabs(wp_course_deviation_deg) < 90.0) {
            _wp_to_flag_node->setBoolValue(true); }
        // If it's more than 90 degrees the desired course is from the waypoint.
        else if (fabs(wp_course_deviation_deg) > 90.0) {
          _wp_to_flag_node->setBoolValue(false);
          // When the course is away from the waypoint, 
          // it makes sense to change the sign of the deviation.
          wp_course_deviation_deg *= -1.0;
          while (wp_course_deviation_deg < -90.0)
            wp_course_deviation_deg += 180.0;
          while (wp_course_deviation_deg >  90.0)
            wp_course_deviation_deg -= 180.0; }

        _wp_course_deviation_node->setDoubleValue(wp_course_deviation_deg);

        // Cross track error.
        wp_course_error_m = sin(wp_course_deviation_deg * SG_PI / 180.0) 
          * (wp_distance);
        _wp_course_error_nm_node->setDoubleValue(wp_course_error_m 
                                                 * SG_METER_TO_NM);

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
