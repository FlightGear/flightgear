// vertical_speed_indicator.hxx - a regular VSI tied to the static port.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX
#define __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>


/**
 * Model a non-instantaneous VSI tied to the static port.
 *
 * Input properties:
 *
 * /instrumentation/vertical-speed-indicator/serviceable
 * /systems/static[0]/pressure-inhg
 *
 * Output properties:
 *
 * /instrumentation/vertical-speed-indicator/indicated-speed-fpm
 */
class VerticalSpeedIndicator : public FGSubsystem
{

public:

    VerticalSpeedIndicator ();
    virtual ~VerticalSpeedIndicator ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    double _internal_pressure_inhg;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _speed_node;
    
};

#endif // __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX
