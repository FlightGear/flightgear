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
 * Output properties:
 *
 * /instrumentation/gps/indicated-longitude-deg
 * /instrumentation/gps/indicated-latitude-deg
 * /instrumentation/gps/indicated-altitude-ft
 * /instrumentation/gps/indicated-track-true-deg
 * /instrumentation/gps/indicated-track-magnetic-deg
 * /instrumentation/gps/indicated-ground-speed-kt
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

    SGPropertyNode_ptr _raim_node;
    SGPropertyNode_ptr _indicated_longitude_node;
    SGPropertyNode_ptr _indicated_latitude_node;
    SGPropertyNode_ptr _indicated_altitude_node;
    SGPropertyNode_ptr _true_track_node;
    SGPropertyNode_ptr _magnetic_track_node;
    SGPropertyNode_ptr _speed_node;

    bool _last_valid;
    double _last_longitude_deg;
    double _last_latitude_deg;
    double _last_altitude_m;

};


#endif // __INSTRUMENTS_GPS_HXX
