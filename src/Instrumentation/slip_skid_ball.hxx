// slip_skid_ball.hxx - an slip-skid ball.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_SLIP_SKID_BALL_HXX
#define __INSTRUMENTS_SLIP_SKID_BALL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a slip-skid ball.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * /accelerations/pilot/y-accel-fps_sec
 * /accelerations/pilot/z-accel-fps_sec
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-slip-skid
 */
class SlipSkidBall : public SGSubsystem
{

public:

    SlipSkidBall ( SGPropertyNode *node );
    virtual ~SlipSkidBall ();

    virtual void init ();
    virtual void update (double dt);

private:

    double _last_pos;

    std::string _name;
    int _num;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _y_accel_node;
    SGPropertyNode_ptr _z_accel_node;
    SGPropertyNode_ptr _out_node;
    SGPropertyNode_ptr _override_node;
   
};

#endif // __INSTRUMENTS_SLIP_SKID_BALL_HXX
