// submodel.hxx - models a releasable submodel.
// Written by Dave Culp, started Aug 2004
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEMS_SUBMODEL_HXX
#define __SYSTEMS_SUBMODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <AIModel/AIManager.hxx>


class SubmodelSystem : public SGSubsystem
{

public:

    SubmodelSystem ();
    ~SubmodelSystem ();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);
    bool release (double dt);

private:

    double x_offset, y_offset, z_offset;
    double pitch_offset, yaw_offset;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _trigger_node;
    SGPropertyNode_ptr _amount_node;

    SGPropertyNode_ptr _user_lat_node;
    SGPropertyNode_ptr _user_lon_node;
    SGPropertyNode_ptr _user_heading_node;
    SGPropertyNode_ptr _user_alt_node;
    SGPropertyNode_ptr _user_pitch_node;
    SGPropertyNode_ptr _user_roll_node;
    SGPropertyNode_ptr _user_yaw_node;
    SGPropertyNode_ptr _user_speed_node;

    double elapsed_time;
    FGAIManager* ai;
    double initial_velocity;
    bool firing;
};

#endif // __SYSTEMS_SUBMODEL_HXX
