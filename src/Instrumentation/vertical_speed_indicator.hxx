// vertical_speed_indicator.hxx - a regular VSI tied to the static port.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX
#define __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a non-instantaneous VSI tied to the static port.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * "static_port"/pressure-inhg
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-speed-fpm
 */
class VerticalSpeedIndicator : public SGSubsystem
{

public:

    VerticalSpeedIndicator ( SGPropertyNode *node );
    virtual ~VerticalSpeedIndicator ();

    virtual void init ();
    virtual void update (double dt);

private:

    double _internal_pressure_inhg;

    std::string _name;
    int _num;
    std::string _static_pressure;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _speed_node;
    SGPropertyNode_ptr _speed_up_node;
    
};

#endif // __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX
