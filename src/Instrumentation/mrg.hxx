// attitude_indicator.hxx - a vacuum-powered attitude indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_MRG_HXX
#define __INSTRUMENTS_MRG_HXX 1


#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "gyro.hxx"


/**
 * Model an electrically-powered master reference gyro.
 *
 * Input properties:
 *
 * /instrumentation/"name"/config/tumble-flag
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/caged-flag
 * /instrumentation/"name"/tumble-norm
 * /orientation/pitch-deg
 * /orientation/roll-deg
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-pitch-deg
 * /instrumentation/"name"/indicated-roll-deg
 * /instrumentation/"name"/tumble-norm
 */

class MasterReferenceGyro : public SGSubsystem
{

public:

    MasterReferenceGyro ( SGPropertyNode *node );
    MasterReferenceGyro ();
    virtual ~MasterReferenceGyro ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    static const double gravity; //conversion factor

    std::string _name;
    int _num;

    double _last_roll;
    double _last_pitch;
    double _last_hdg;
    double _indicated_roll;
    double _indicated_pitch;
    double _indicated_hdg;
    double _indicated_pitch_rate;
    double _indicated_roll_rate;
    double _indicated_hdg_rate;
    double _last_roll_rate;
    double _last_pitch_rate;
    double _last_yaw_rate;
    double _last_g;
    double _erect_time;
    double _g_error;

    Gyro _gyro;

    SGPropertyNode_ptr _tumble_flag_node;
    SGPropertyNode_ptr _caged_node;
    SGPropertyNode_ptr _off_node;
    SGPropertyNode_ptr _tumble_node;
    SGPropertyNode_ptr _pitch_in_node;
    SGPropertyNode_ptr _roll_in_node;
    SGPropertyNode_ptr _hdg_in_node;
    SGPropertyNode_ptr _hdg_mag_in_node;
    SGPropertyNode_ptr _g_in_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _pitch_int_node;
    SGPropertyNode_ptr _roll_int_node;
    SGPropertyNode_ptr _hdg_int_node;
    SGPropertyNode_ptr _pitch_out_node;
    SGPropertyNode_ptr _roll_out_node;
    SGPropertyNode_ptr _hdg_out_node;
    SGPropertyNode_ptr _hdg_mag_out_node;
    SGPropertyNode_ptr _pitch_rate_out_node;
    SGPropertyNode_ptr _roll_rate_out_node;
    SGPropertyNode_ptr _hdg_rate_out_node;
    SGPropertyNode_ptr _error_out_node;
    SGPropertyNode_ptr _yaw_rate_node;
    SGPropertyNode_ptr _roll_rate_node;
    SGPropertyNode_ptr _pitch_rate_node;
    SGPropertyNode_ptr _responsiveness_node;
    SGPropertyNode_ptr _hdg_input_source_node;
    SGPropertyNode_ptr _fast_erect_node;
};

#endif // __INSTRUMENTS_MRG_HXX
