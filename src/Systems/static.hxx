// static.hxx - the static air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEMS_STATIC_HXX
#define __SYSTEMS_STATIC_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>


/**
 * Model a static air system.
 *
 * Input properties:
 *
 * /environment/pressure-inhg
 * /systems/static[0]/serviceable
 *
 * Output properties:
 *
 * /systems/static[0]/pressure-inhg
 *
 * TODO: support multiple static ports and specific locations
 * TODO: support alternate air with errors
 */
class StaticSystem : public FGSubsystem
{

public:

    StaticSystem ();
    virtual ~StaticSystem ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_in_node;
    SGPropertyNode_ptr _pressure_out_node;
    
};

#endif // __SYSTEMS_STATIC_HXX
