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
 * /instrumentation/gps/wp-ID
 * /instrumentation/gps/wp-name
 * /instrumentation/gps/course-deg
 * /instrumentation/gps/get-nearest-airport
 * /instrumentation/gps/waypoint-type
 *
 * Output properties:
 *
 * /instrumentation/gps/longitude-deg
 * /instrumentation/gps/latitude-deg
 * /instrumentation/gps/altitude-ft
 * /instrumentation/gps/track-true-deg
 * /instrumentation/gps/track-magnetic-deg
 * /instrumentation/gps/ground-speed-kt
 *
 * /instrumentation/gps/wp-distance-m
 * /instrumentation/gps/wp-bearing-deg
 * /instrumentation/gps/TTW
 * /instrumentation/gps/wp-bearing-deg
 * /instrumentation/gps/radials/actual-deg
 * /instrumentation/gps/course-deviation-deg
 * /instrumentation/gps/course-error-nm
 * /instrumentation/gps/to-flag
 * /instrumentation/gps/odometer
 * /instrumentation/gps/trip-odometer
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
    SGPropertyNode_ptr _wp_longitude_node;
    SGPropertyNode_ptr _wp_latitude_node;
    SGPropertyNode_ptr _wp_ID_node;
    SGPropertyNode_ptr _wp_name_node;
    SGPropertyNode_ptr _wp_course_node;
    SGPropertyNode_ptr _get_nearest_airport_node;
    SGPropertyNode_ptr _waypoint_type_node;

    SGPropertyNode_ptr _raim_node;
    SGPropertyNode_ptr _indicated_longitude_node;
    SGPropertyNode_ptr _indicated_latitude_node;
    SGPropertyNode_ptr _indicated_altitude_node;
    SGPropertyNode_ptr _true_track_node;
    SGPropertyNode_ptr _magnetic_track_node;
    SGPropertyNode_ptr _speed_node;
    SGPropertyNode_ptr _wp_distance_node;
    SGPropertyNode_ptr _wp_ttw_node;
    SGPropertyNode_ptr _wp_bearing_node;
    SGPropertyNode_ptr _wp_actual_radial_node;
    SGPropertyNode_ptr _wp_course_deviation_node;
    SGPropertyNode_ptr _wp_course_error_nm_node;
    SGPropertyNode_ptr _wp_to_flag_node;
    SGPropertyNode_ptr _odometer_node;
    SGPropertyNode_ptr _trip_odometer_node;

    bool _last_valid;
    double _last_longitude_deg;
    double _last_latitude_deg;
    double _last_altitude_m;
    double _last_speed_kts;

    double _wp_latitude;
    double _wp_longitude;
    string _last_wp_ID;

};


#endif // __INSTRUMENTS_GPS_HXX
