// attitude_indicator.hxx - a vacuum-powered attitude indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_ATTITUDE_INDICATOR_HXX
#define __INSTRUMENTS_ATTITUDE_INDICATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>

#include "gyro.hxx"


/**
 * Model a vacuum-powered attitude indicator.
 *
 * This first, simple draft is hard-wired to vacuum pump #1.
 *
 * Input properties:
 *
 * /instrumentation/attitude-indicator/config/tumble-flag
 * /instrumentation/attitude-indicator/serviceable
 * /instrumentation/attitude-indicator/caged-flag
 * /instrumentation/attitude-indicator/tumble-norm
 * /orientation/pitch-deg
 * /orientation/roll-deg
 * /systems/vacuum[0]/suction-inhg
 *
 * Output properties:
 *
 * /instrumentation/attitude-indicator/indicated-pitch-deg
 * /instrumentation/attitude-indicator/indicated-roll-deg
 * /instrumentation/attitude-indicator/tumble-norm
 */
class AttitudeIndicator : public FGSubsystem
{

public:

    AttitudeIndicator ();
    virtual ~AttitudeIndicator ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    Gyro _gyro;

    SGPropertyNode_ptr _tumble_flag_node;
    SGPropertyNode_ptr _caged_node;
    SGPropertyNode_ptr _tumble_node;
    SGPropertyNode_ptr _pitch_in_node;
    SGPropertyNode_ptr _roll_in_node;
    SGPropertyNode_ptr _suction_node;
    SGPropertyNode_ptr _pitch_int_node;
    SGPropertyNode_ptr _roll_int_node;
    SGPropertyNode_ptr _pitch_out_node;
    SGPropertyNode_ptr _roll_out_node;
    
};

#endif // __INSTRUMENTS_ATTITUDE_INDICATOR_HXX
