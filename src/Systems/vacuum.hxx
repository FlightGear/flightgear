// vacuum.hxx - a vacuum pump connected to the aircraft engine.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEMS_VACUUM_HXX
#define __SYSTEMS_VACUUM_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a vacuum-pump system.
 *
 * This first, simple draft is hard-wired to engine #1.
 *
 * Input properties:
 *
 * "rpm"
 * /environment/pressure-inhg
 * /systems/"name"/serviceable
 *
 * Output properties:
 *
 * /systems/"name"/suction-inhg
 */
class VacuumSystem : public SGSubsystem
{

public:

    VacuumSystem( SGPropertyNode *node );
    VacuumSystem( int i );
    virtual ~VacuumSystem ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    string name;
    int num;
    string rpm;
    double scale;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _rpm_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _suction_node;
    
};

#endif // __SYSTEMS_VACUUM_HXX
