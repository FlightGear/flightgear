// heading_indicator.hxx - a vacuum-powered heading indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_HEADING_INDICATOR_HXX
#define __INSTRUMENTS_HEADING_INDICATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>


/**
 * Model a vacuum-powered heading indicator.
 *
 * This first, simple draft is hard-wired to vacuum pump #1.
 *
 * Input properties:
 *
 * /instrumentation/heading-indicator/serviceable
 * /instrumentation/heading-indicator/offset-deg
 * /orientation/heading-deg
 * /systems/vacuum[0]/suction-inhg
 *
 * Output properties:
 *
 * /instrumentation/heading-indicator/indicated-heading-deg
 */
class HeadingIndicator : public FGSubsystem
{

public:

    HeadingIndicator ();
    virtual ~HeadingIndicator ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    double _spin;
    double _last_heading_deg;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _offset_node;
    SGPropertyNode_ptr _heading_in_node;
    SGPropertyNode_ptr _suction_node;
    SGPropertyNode_ptr _heading_out_node;
    
};

#endif // __INSTRUMENTS_HEADING_INDICATOR_HXX
