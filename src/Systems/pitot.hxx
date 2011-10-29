// pitot.hxx - the pitot air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEMS_PITOT_HXX
#define __SYSTEMS_PITOT_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <string>
using std::string;

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
 * /systems/"name"/serviceable
 * /environment/pressure-slugft3
 * /environment/density-slugft3
 * /velocities/airspeed-kt
 *
 * Output properties:
 *
 * /systems/"name"/total-pressure-inhg
 */
class PitotSystem : public SGSubsystem
{

public:

    PitotSystem ( SGPropertyNode *node );
    virtual ~PitotSystem ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    std::string _name;
    int _num;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _density_node;
    SGPropertyNode_ptr _velocity_node;
    SGPropertyNode_ptr _slip_angle;
    SGPropertyNode_ptr _total_pressure_node;
};

#endif // __SYSTEMS_PITOT_HXX
