// dme.hxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_DME_HXX
#define __INSTRUMENTS_DME_HXX 1

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a DME radio.
 *
 * Input properties:
 *
 * /position/longitude-deg
 * /position/latitude-deg
 * /position/altitude-ft
 * /systems/electrical/outputs/dme
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/frequencies/source
 * /instrumentation/"name"/frequencies/selected-mhz
 *
 * Output properties:
 *
 * /instrumentation/"name"/in-range
 * /instrumentation/"name"/indicated-distance-nm
 * /instrumentation/"name"/indicated-ground-speed-kt
 * /instrumentation/"name"/indicated-time-kt
 */
class DME : public SGSubsystem
{

public:

    DME ( SGPropertyNode *node );
    virtual ~DME ();

    virtual void init ();
    virtual void update (double delta_time_sec);

private:

    void search (double frequency, double longitude_rad,
                 double latitude_rad, double altitude_m);

    SGPropertyNode_ptr _longitude_node;
    SGPropertyNode_ptr _latitude_node;
    SGPropertyNode_ptr _altitude_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _source_node;
    SGPropertyNode_ptr _frequency_node;

    SGPropertyNode_ptr _in_range_node;
    SGPropertyNode_ptr _distance_node;
    SGPropertyNode_ptr _speed_node;
    SGPropertyNode_ptr _time_node;

    double _last_distance_nm;
    double _last_frequency_mhz;
    double _time_before_search_sec;

    bool _transmitter_valid;
    SGVec3d _transmitter;
    double _transmitter_elevation_ft;
    double _transmitter_range_nm;
    double _transmitter_bias;

    string _name;
    int _num;

};


#endif // __INSTRUMENTS_DME_HXX
