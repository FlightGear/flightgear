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

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _source_node;
    SGPropertyNode_ptr _frequency_node;

    SGPropertyNode_ptr _in_range_node;
    SGPropertyNode_ptr _distance_node;
    SGPropertyNode_ptr _speed_node;
    SGPropertyNode_ptr _time_node;
    SGPropertyNode_ptr _ident_btn_node;
    SGPropertyNode_ptr _volume_node;

    double _last_distance_nm;
    double _last_frequency_mhz;
    double _time_before_search_sec;

    FGNavRecord * _navrecord;

    std::string _name;
    int _num;

    class AudioIdent * _audioIdent;
};


#endif // __INSTRUMENTS_DME_HXX
