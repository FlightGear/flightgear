// static.hxx - the static air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEMS_STATIC_HXX
#define __SYSTEMS_STATIC_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a static air system.
 *
 * Input properties:
 *
 * /environment/pressure-inhg
 * /systems/"name"/serviceable
 *
 * Output properties:
 *
 * /systems/"name"/pressure-inhg
 *
 * TODO: support specific locations
 * TODO: support alternate air with errors
 */
class StaticSystem : public SGSubsystem
{

public:

    StaticSystem ( SGPropertyNode *node );
    StaticSystem ( int i );
    virtual ~StaticSystem ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    std::string _name;
    int _num;
    double _tau;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_in_node;
    SGPropertyNode_ptr _pressure_out_node;
    
};

#endif // __SYSTEMS_STATIC_HXX
