// dme.hxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_DME_HXX
#define __INSTRUMENTS_DME_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/math/point3d.hxx>
#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>


/**
 * Model a DME radio.
 *
 * Input properties:
 *
 * /position/longitude-deg
 * /position/latitude-deg
 * /position/altitude-ft
 * /systems/electrical/outputs/dme
 * /instrumentation/dme/serviceable
 * /instrumentation/dme/frequencies/source
 * /instrumentation/dme/frequencies/selected-mhz
 *
 * Output properties:
 *
 * /instrumentation/dme/in-range
 * /instrumentation/dme/indicated-distance-nm
 * /instrumentation/dme/indicated-ground-speed-kt
 * /instrumentation/dme/indicated-time-kt
 */
class DME : public FGSubsystem
{

public:

    DME ();
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
    Point3D _transmitter;
    double _transmitter_elevation_ft;
    double _transmitter_range_nm;

};


#endif // __INSTRUMENTS_DME_HXX
