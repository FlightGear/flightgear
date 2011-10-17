// turn_indicator.hxx - an electric-powered turn indicator.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_TURN_INDICATOR_HXX
#define __INSTRUMENTS_TURN_INDICATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "gyro.hxx"


/**
 * Model an electric-powered turn indicator.
 *
 * This class does not model the slip/skid ball; that is properly 
 * a separate instrument.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/spin
 * /orientation/roll-rate-degps
 * /orientation/yaw-rate-degps
 * /systems/electrical/outputs/turn-coordinator
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-turn-rate
 */
class TurnIndicator : public SGSubsystem
{

public:

    TurnIndicator ( SGPropertyNode *node );
    virtual ~TurnIndicator ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    Gyro _gyro;
    double _last_rate;

    std::string _name;
    int _num;

    SGPropertyNode_ptr _roll_rate_node;
    SGPropertyNode_ptr _yaw_rate_node;
    SGPropertyNode_ptr _electric_current_node;
    SGPropertyNode_ptr _rate_out_node;
    
};

#endif // __INSTRUMENTS_TURN_INDICATOR_HXX
