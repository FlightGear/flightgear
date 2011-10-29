// mag_compass.hxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_MAG_COMPASS_HXX
#define __INSTRUMENTS_MAG_COMPASS_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a magnetic compass.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * /orientation/roll-deg
 * /orientation/pitch-deg
 * /orientation/heading-magnetic-deg
 * /orientation/side-slip-deg
 * /environment/magnetic-dip-deg
 * /accelerations/pilot/north-accel-fps_sec
 * /accelerations/pilot/east-accel-fps_sec
 * /accelerations/pilot/down-accel-fps_sec
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-heading-deg
 */
class MagCompass : public SGSubsystem
{

public:

    MagCompass ( SGPropertyNode *node);
    MagCompass ();
    virtual ~MagCompass ();

    virtual void init ();
    virtual void update (double dt);

private:

    double _error_deg;
    double _rate_degps;

    std::string _name;
    int _num;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _roll_node;
    SGPropertyNode_ptr _pitch_node;
    SGPropertyNode_ptr _heading_node;
    SGPropertyNode_ptr _beta_node;
    SGPropertyNode_ptr _dip_node;
    SGPropertyNode_ptr _x_accel_node;
    SGPropertyNode_ptr _y_accel_node;
    SGPropertyNode_ptr _z_accel_node;
    SGPropertyNode_ptr _out_node;

};

#endif // __INSTRUMENTS_MAG_COMPASS_HXX
