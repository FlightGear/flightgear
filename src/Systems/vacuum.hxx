// vacuum.hxx - a vacuum pump connected to the aircraft engine.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEMS_VACUUM_HXX
#define __SYSTEMS_VACUUM_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/math/sg_types.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a vacuum-pump system.
 *
 * Multiple pumps (i.e. for a multiengine aircraft) can be specified.
 *
 * Input properties:
 *
 * "rpm1"
 * "rpm2"
 * "..."
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

    std::string _name;
    int _num;
    string_list _rpms;
    double _scale;
    SGPropertyNode_ptr _serviceable_node;
    std::vector<SGPropertyNode_ptr> _rpm_nodes;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _suction_node;
    
};

#endif // __SYSTEMS_VACUUM_HXX
