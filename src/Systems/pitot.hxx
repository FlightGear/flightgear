// pitot.hxx - the pitot air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEMS_PITOT_HXX
#define __SYSTEMS_PITOT_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a pitot air system.
 *
 * The output is the sum of static and dynamic pressure (not just the
 * dynamic pressure).
 *
 * Input properties:
 *
 * /systems/pitot[0]/serviceable
 * /environment/pressure-slugft3
 * /environment/density-slugft3
 * /velocities/airspeed-kt
 *
 * Output properties:
 *
 * /systems/pitot[0]/total-pressure-inhg
 */
class PitotSystem : public SGSubsystem
{

public:

    PitotSystem ();
    virtual ~PitotSystem ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _density_node;
    SGPropertyNode_ptr _velocity_node;
    SGPropertyNode_ptr _total_pressure_node;
    
};

#endif // __SYSTEMS_PITOT_HXX
