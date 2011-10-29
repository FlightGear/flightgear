// heading_indicator.hxx - a vacuum-powered heading indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_HEADING_INDICATOR_ELEC_HXX
#define __INSTRUMENTS_HEADING_INDICATOR_ELEC_HXX 1


#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/sg_random.h>

#include "gyro.hxx"


/**
 * Model an electrically-powered heading indicator.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/spin
 * /instrumentation/"name"/offset-deg
 * /orientation/heading-deg
 * /systems/electrical/outputs/DG
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-heading-deg
 */
class HeadingIndicatorDG : public SGSubsystem
{

public:

    HeadingIndicatorDG ( SGPropertyNode *node );
    HeadingIndicatorDG ();
    virtual ~HeadingIndicatorDG ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    Gyro _gyro;
    double _last_heading_deg;

    std::string name;
    int num;
    //string vacuum_system;

    SGPropertyNode_ptr _offset_node;
    SGPropertyNode_ptr _heading_in_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _heading_out_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _error_node;
    SGPropertyNode_ptr _nav1_error_node;
    SGPropertyNode_ptr _align_node;
    SGPropertyNode_ptr _yaw_rate_node;
    SGPropertyNode_ptr _heading_bug_error_node;
    SGPropertyNode_ptr _g_node;
};

#endif // __INSTRUMENTS_HEADING_INDICATOR_ELEC_HXX
