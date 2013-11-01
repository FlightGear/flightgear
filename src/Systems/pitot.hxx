// pitot.hxx - the pitot air system.
// Written by David Megginson, started 2002.
//
// Last modified by Eric van den Berg, 01 Nov 2013
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
 * /environment/pressure-inhg
 * /velocities/mach
 *
 * Output properties:
 *
 * /systems/"name"/total-pressure-inhg
 * /systems/"name"/measured-total-pressure-inhg
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
    double _stall_factor;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _mach_node;
    SGPropertyNode_ptr _total_pressure_node;
    SGPropertyNode_ptr _measured_total_pressure_node;
    SGPropertyNode_ptr _alpha_deg_node;
    SGPropertyNode_ptr _beta_deg_node;
};

#endif // __SYSTEMS_PITOT_HXX
