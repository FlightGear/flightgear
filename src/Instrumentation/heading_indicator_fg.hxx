// heading_indicator.hxx - a vacuum-powered heading indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_HEADING_INDICATOR_FG_HXX
#define __INSTRUMENTS_HEADING_INDICATOR_FG_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "gyro.hxx"


/**
 * Model an electically-powered fluxgate compass
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
class HeadingIndicatorFG : public SGSubsystem
{

public:

    HeadingIndicatorFG ( SGPropertyNode *node );
    HeadingIndicatorFG ();
    virtual ~HeadingIndicatorFG ();

    virtual void init ();   
    virtual void bind (); 
    virtual void unbind ();
    virtual void update (double dt);

private:

    Gyro _gyro;
    double _last_heading_deg;

    std::string name;
    int num;
    
    SGPropertyNode_ptr _offset_node;
    SGPropertyNode_ptr _heading_in_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _heading_out_node;
	SGPropertyNode_ptr _electrical_node;
	SGPropertyNode_ptr _error_node;
	SGPropertyNode_ptr _nav1_error_node;
    SGPropertyNode_ptr _off_node;


    
};

#endif // __INSTRUMENTS_HEADING_INDICATOR_HXX
