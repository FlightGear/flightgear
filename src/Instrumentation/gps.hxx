// gps.hxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_GPS_HXX
#define __INSTRUMENTS_GPS_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a GPS radio.
 *
 * Input properties:
 *
 * /position/longitude-deg
 * /position/latitude-deg
 * /position/altitude-ft
 * /environment/magnetic-variation-deg
 * /systems/electrical/outputs/gps
 * /instrumentation/gps/serviceable
 * 
 * /instrumentation/gps/wp-longitude-deg
 * /instrumentation/gps/wp-latitude-deg
 * /instrumentation/gps/wp-altitude-ft
 * /instrumentation/gps/wp-ID
 * /instrumentation/gps/wp-name
 * /instrumentation/gps/desired-course-deg
 * /instrumentation/gps/get-nearest-airport
 * /instrumentation/gps/waypoint-type
 * /instrumentation/gps/tracking-bug
 *
 * Output properties:
 *
 * /instrumentation/gps/indicated-longitude-deg
 * /instrumentation/gps/indicated-latitude-deg
 * /instrumentation/gps/indicated-altitude-ft
 * /instrumentation/gps/indicated-vertical-speed-fpm
 * /instrumentation/gps/indicated-track-true-deg
 * /instrumentation/gps/indicated-track-magnetic-deg
 * /instrumentation/gps/indicated-ground-speed-kt
 *
 * /instrumentation/gps/wp-distance-nm
 * /instrumentation/gps/wp-bearing-deg
 * /instrumentation/gps/wp-bearing-mag-deg
 * /instrumentation/gps/TTW
 * /instrumentation/gps/course-deviation-deg
 * /instrumentation/gps/course-error-nm
 * /instrumentation/gps/to-flag
 * /instrumentation/gps/odometer
 * /instrumentation/gps/trip-odometer
 * /instrumentation/gps/true-bug-error-deg
 * /instrumentation/gps/magnetic-bug-error-deg
 * /instrumentation/gps/true-bearing-error-deg
 * /instrumentation/gps/magnetic-bearing-error-deg
 */
class GPS : public SGSubsystem
{

public:

    GPS ();
    virtual ~GPS ();

    virtual void init ();
    virtual void update (double delta_time_sec);

private:

    void search (double frequency, double longitude_rad,
                 double latitude_rad, double altitude_m);

    SGPropertyNode_ptr _longitude_node;
    SGPropertyNode_ptr _latitude_node;
    SGPropertyNode_ptr _altitude_node;
    SGPropertyNode_ptr _magvar_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _wp0_longitude_node;
    SGPropertyNode_ptr _wp0_latitude_node;
    SGPropertyNode_ptr _wp0_altitude_node;
    SGPropertyNode_ptr _wp0_ID_node;
    SGPropertyNode_ptr _wp0_name_node;
    SGPropertyNode_ptr _wp0_course_node;
    SGPropertyNode_ptr _get_nearest_airport_node;
    SGPropertyNode_ptr _wp0_waypoint_type_node;
    SGPropertyNode_ptr _wp1_longitude_node;
    SGPropertyNode_ptr _wp1_latitude_node;
    SGPropertyNode_ptr _wp1_altitude_node;
    SGPropertyNode_ptr _wp1_ID_node;
    SGPropertyNode_ptr _wp1_name_node;
    SGPropertyNode_ptr _wp1_course_node;
    SGPropertyNode_ptr _wp1_waypoint_type_node;
    SGPropertyNode_ptr _tracking_bug_node;

    SGPropertyNode_ptr _raim_node;
    SGPropertyNode_ptr _indicated_longitude_node;
    SGPropertyNode_ptr _indicated_latitude_node;
    SGPropertyNode_ptr _indicated_altitude_node;
    SGPropertyNode_ptr _indicated_vertical_speed_node;
    SGPropertyNode_ptr _true_track_node;
    SGPropertyNode_ptr _magnetic_track_node;
    SGPropertyNode_ptr _speed_node;
    SGPropertyNode_ptr _wp0_distance_node;
    SGPropertyNode_ptr _wp0_ttw_node;
    SGPropertyNode_ptr _wp0_bearing_node;
    SGPropertyNode_ptr _wp0_mag_bearing_node;
    SGPropertyNode_ptr _wp0_course_deviation_node;
    SGPropertyNode_ptr _wp0_course_error_nm_node;
    SGPropertyNode_ptr _wp0_to_flag_node;
    SGPropertyNode_ptr _wp1_distance_node;
    SGPropertyNode_ptr _wp1_ttw_node;
    SGPropertyNode_ptr _wp1_bearing_node;
    SGPropertyNode_ptr _wp1_mag_bearing_node;
    SGPropertyNode_ptr _wp1_course_deviation_node;
    SGPropertyNode_ptr _wp1_course_error_nm_node;
    SGPropertyNode_ptr _wp1_to_flag_node;
    SGPropertyNode_ptr _odometer_node;
    SGPropertyNode_ptr _trip_odometer_node;
    SGPropertyNode_ptr _true_bug_error_node;
    SGPropertyNode_ptr _magnetic_bug_error_node;
    SGPropertyNode_ptr _true_wp0_bearing_error_node;
    SGPropertyNode_ptr _magnetic_wp0_bearing_error_node;
    SGPropertyNode_ptr _true_wp1_bearing_error_node;
    SGPropertyNode_ptr _magnetic_wp1_bearing_error_node;
    SGPropertyNode_ptr _leg_distance_node;
    SGPropertyNode_ptr _leg_course_node;
    SGPropertyNode_ptr _leg_magnetic_course_node;
    SGPropertyNode_ptr _alt_dist_ratio_node;
    SGPropertyNode_ptr _leg_course_deviation_node;
    SGPropertyNode_ptr _leg_course_error_nm_node;
    SGPropertyNode_ptr _leg_to_flag_node;
    SGPropertyNode_ptr _alt_deviation_node;

    bool _last_valid;
    double _last_longitude_deg;
    double _last_latitude_deg;
    double _last_altitude_m;
    double _last_speed_kts;

    double _wp0_latitude_deg;
    double _wp0_longitude_deg;
    double _wp0_altitude_m;
    double _wp1_latitude_deg;
    double _wp1_longitude_deg;
    double _wp1_altitude_m;
    string _last_wp0_ID;
    string _last_wp1_ID;

    double _alt_dist_ratio;
    double _distance_m;
    double _course_deg;

    double bias_length;
    double bias_angle;
    double azimuth_error;
    double range_error;
    double elapsed_time;

};


#endif // __INSTRUMENTS_GPS_HXX
