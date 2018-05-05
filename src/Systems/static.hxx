// static.hxx - the static air system.
// Written by David Megginson, started 2002.
//
// Last modified by Eric van den Berg, 09 November 2013
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
 * /orientation/alpha-deg
 * /orientation/side-slip-rad
 * /velocities/mach
 *
 * Output properties:
 *
 * /systems/"name"/pressure-inhg
 *
 * TODO: support alternate air with errors
 */
class StaticSystem : public SGSubsystem
{
public:
    StaticSystem ( SGPropertyNode *node );
    StaticSystem ( int i );
    virtual ~StaticSystem ();

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "static"; }

private:
    std::string _name;
    int _num;
    double _tau;
    double _error_factor;
    int _type;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_in_node;
    SGPropertyNode_ptr _pressure_out_node;
    SGPropertyNode_ptr _beta_node;
    SGPropertyNode_ptr _alpha_node;
    SGPropertyNode_ptr _mach_node;
};

#endif // __SYSTEMS_STATIC_HXX
