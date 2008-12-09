// gps.hxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_GPS_HXX
#define __INSTRUMENTS_GPS_HXX 1

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/SGMath.hxx>

// forward decls
class SGRoute;

class SGGeodProperty
{
public:
    SGGeodProperty()
    {
    }
        
    void init(SGPropertyNode* base, const char* lonStr, const char* latStr, const char* altStr = NULL);    
    void init(const char* lonStr, const char* latStr, const char* altStr = NULL);    
    void clear();    
    void operator=(const SGGeod& geod);    
    SGGeod get() const;
private:
    SGPropertyNode_ptr _lon, _lat, _alt;
};

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

    GPS (SGPropertyNode *node);
    GPS ();
    virtual ~GPS ();

    virtual void init ();
    virtual void update (double delta_time_sec);

private:
    typedef struct {
        double dt;
        SGGeod pos;
        SGGeod wp0_pos;
        SGGeod wp1_pos;
        bool waypoint_changed;
        double speed_kt;
        double track1_deg;
        double track2_deg;
        double magvar_deg;
        double wp0_distance;
        double wp0_course_deg;
        double wp0_bearing_deg;
        double wp1_distance;
        double wp1_course_deg;
        double wp1_bearing_deg;
    } UpdateContext;
    
    void search (double frequency, const SGGeod& pos);

    /**
     * reset all output properties to default / non-service values
     */
    void clearOutput();

    void updateWithValid(UpdateContext& ctx);
    
    void updateNearestAirport(UpdateContext& ctx);
    void updateWaypoint0(UpdateContext& ctx);
    void updateWaypoint1(UpdateContext& ctx);

    void updateLegCourse(UpdateContext& ctx);
    void updateWaypoint0Course(UpdateContext& ctx);
    void updateWaypoint1Course(UpdateContext& ctx);

    void waypointChanged(UpdateContext& ctx);
    void updateTTWNode(UpdateContext& ctx, double distance_m, SGPropertyNode_ptr node);
    void updateTrackingBug(UpdateContext& ctx);
    
    SGPropertyNode_ptr _magvar_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _wp0_ID_node;
    SGPropertyNode_ptr _wp0_name_node;
    SGPropertyNode_ptr _wp0_course_node;
    SGPropertyNode_ptr _get_nearest_airport_node;
    SGPropertyNode_ptr _wp1_ID_node;
    SGPropertyNode_ptr _wp1_name_node;
    SGPropertyNode_ptr _wp1_course_node;
    SGPropertyNode_ptr _tracking_bug_node;

    SGPropertyNode_ptr _raim_node;
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
    SGGeod _last_pos;
    double _last_speed_kts;

    std::string _last_wp0_ID;
    std::string _last_wp1_ID;

    double _alt_dist_ratio;
    double _distance_m;
    double _course_deg;

    double _bias_length;
    double _bias_angle;
    double _azimuth_error;
    double _range_error;
    double _elapsed_time;

    std::string _name;
    int _num;

    SGGeodProperty _position;
    SGGeodProperty _wp0_position;
    SGGeodProperty _wp1_position;
    SGGeodProperty _indicated_pos;
};


#endif // __INSTRUMENTS_GPS_HXX
