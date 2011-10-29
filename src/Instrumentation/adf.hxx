// adf.hxx - automatic direction finder.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_ADF_HXX
#define __INSTRUMENTS_ADF_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <string>

#include <simgear/props/props.hxx>

#include <simgear/structure/subsystem_mgr.hxx>

using std::string;


class SGSampleGroup;

/**
 * Model an ADF radio.
 *
 * Input properties:
 *
 * /position/longitude-deg
 * /position/latitude-deg
 * /position/altitude-ft
 * /orientation/heading-deg
 * /systems/electrical/outputs/adf
 * /instrumentation/adf/serviceable
 * /instrumentation/adf/error-deg
 * /instrumentation/adf/frequencies/selected-khz
 * /instrumentation/adf/mode
 * /instrumentation/adf/ident-audible
 * /instrumentation/adf/volume-norm
 *
 * Output properties:
 *
 * /instrumentation/adf/in-range
 * /instrumentation/adf/indicated-bearing-deg
 * /instrumentation/adf/ident
 */
class ADF : public SGSubsystem
{

public:

    ADF ( SGPropertyNode *node );
    virtual ~ADF ();

    virtual void init ();
    virtual void update (double delta_time_sec);

private:

    void set_bearing (double delta_time_sec, double bearing);

    void search (double frequency, double longitude_rad,
                 double latitude_rad, double altitude_m);

    std::string _name;
    unsigned int _num;

    SGPropertyNode_ptr _longitude_node;
    SGPropertyNode_ptr _latitude_node;
    SGPropertyNode_ptr _altitude_node;
    SGPropertyNode_ptr _heading_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _error_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _frequency_node;
    SGPropertyNode_ptr _mode_node;

    SGPropertyNode_ptr _in_range_node;
    SGPropertyNode_ptr _bearing_node;
    SGPropertyNode_ptr _ident_node;
    SGPropertyNode_ptr _ident_audible_node;
    SGPropertyNode_ptr _volume_node;
    SGPropertyNode_ptr _power_btn_node;

    double _time_before_search_sec;

    int _last_frequency_khz;
    bool _transmitter_valid;
    string _last_ident;
    SGGeod _transmitter_pos;
    SGVec3d _transmitter_cart;
    double _transmitter_range_nm;

    int _ident_count;
    time_t _last_ident_time;
    float _last_volume;
    string _adf_ident;

    SGSharedPtr<SGSampleGroup> _sgr;
};


#endif // __INSTRUMENTS_ADF_HXX
