// mag_compass.hxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_MAG_COMPASS_HXX
#define __INSTRUMENTS_MAG_COMPASS_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>


/**
 * Model a magnetic compass.
 *
 * Input properties:
 *
 * /instrumentation/magnetic-compass/serviceable
 * /orientation/heading-deg
 * /orientation/beta-deg
 * /environment/magnetic-variation-deg
 * /environment/magnetic-dip-deg
 * /accelerations/ned/north-accel-fps_sec
 * /accelerations/ned/east-accel-fps_sec
 * /accelerations/ned/down-accel-fps_sec
 *
 * Output properties:
 *
 * /instrumentation/magnetic-compass/indicated-heading-deg
 */
class MagCompass : public FGSubsystem
{

public:

    MagCompass ();
    virtual ~MagCompass ();

    virtual void init ();
    virtual void update (double dt);

private:

    double _error_deg;
    double _rate_degps;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _heading_node;
    SGPropertyNode_ptr _beta_node;
    SGPropertyNode_ptr _variation_node;
    SGPropertyNode_ptr _dip_node;
    SGPropertyNode_ptr _y_accel_node;
    SGPropertyNode_ptr _z_accel_node;
    SGPropertyNode_ptr _north_accel_node;
    SGPropertyNode_ptr _east_accel_node;
    SGPropertyNode_ptr _down_accel_node;
    SGPropertyNode_ptr _out_node;

};

#endif // __INSTRUMENTS_MAG_COMPASS_HXX
