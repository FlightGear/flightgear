// heading_indicator_dg.cxx - a Directional Gyro (DG) compass.
// Based on the vacuum driven Heading Indicator Written by David Megginson,
// started 2002.
//
// Written by Vivian Meazza, started 2005.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/SGMath.hxx>
#include <iostream>
#include <string>
#include <sstream>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "heading_indicator_dg.hxx"

/** Macro calculating x^6 (faster than super-slow math/pow). */
#define POW6(x) (x*x*x*x*x*x)

HeadingIndicatorDG::HeadingIndicatorDG ( SGPropertyNode *node ) :
    name("heading-indicator-dg"),
    num(0)
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        std::string cname = child->getName();
        std::string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_WARN, "Error in DG heading-indicator config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}

HeadingIndicatorDG::~HeadingIndicatorDG ()
{
}

void
HeadingIndicatorDG::init ()
{
    std::string branch;
    branch = "/instrumentation/" + name;

    _heading_in_node = fgGetNode("/orientation/heading-deg", true);
    _yaw_rate_node   = fgGetNode("/orientation/yaw-rate-degps", true);
     _g_node         = fgGetNode("/accelerations/pilot-g", true);

    SGPropertyNode *node    = fgGetNode(branch.c_str(), num, true );
    _offset_node            = node->getChild("offset-deg", 0, true);
    _serviceable_node       = node->getChild("serviceable", 0, true);
    _heading_bug_error_node = node->getChild("heading-bug-error-deg", 0, true);
    _error_node             = node->getChild("error-deg", 0, true);
    _nav1_error_node        = node->getChild("nav1-course-error-deg", 0, true);
    _heading_out_node       = node->getChild("indicated-heading-deg", 0, true);
    _align_node             = node->getChild("align-deg", 0, true);

    _electrical_node = fgGetNode("/systems/electrical/outputs/DG", true);

    reinit();
}

void
HeadingIndicatorDG::bind ()
{
    std::ostringstream temp;
    std::string branch;
    temp << num;
    branch = "/instrumentation/" + name + "[" + temp.str() + "]";

    fgTie((branch + "/serviceable").c_str(),
          &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    fgTie((branch + "/spin").c_str(),
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
HeadingIndicatorDG::unbind ()
{
    std::ostringstream temp;
    std::string branch;
    temp << num;
    branch = "/instrumentation/" + name + "[" + temp.str() + "]";

    fgUntie((branch + "/serviceable").c_str());
    fgUntie((branch + "/spin").c_str());
}

void
HeadingIndicatorDG::reinit (void)
{
    // reset errors/drift values
    _align_node->setDoubleValue(0.0);
    _error_node->setDoubleValue(0.0);
    _offset_node->setDoubleValue(0.0);

    _last_heading_deg = _heading_in_node->getDoubleValue();
    _last_indicated_heading_dg = _last_heading_deg;

    _gyro.reinit();
}

void
HeadingIndicatorDG::update (double dt)
{
                                // Get the spin from the gyro
    _gyro.set_power_norm(_electrical_node->getDoubleValue());

    _gyro.update(dt);

    // read inputs
    double spin = _gyro.get_spin_norm();
    double heading = _heading_in_node->getDoubleValue();
    double offset = _offset_node->getDoubleValue();

    // calculate scaling factor
    double factor = POW6(spin);

    // calculate time-based precession (scaled by spin factor, since
    // there is no precession when the gyro is stuck).
    offset -= dt * (0.25 / 60.0) * factor; // 360deg/day

    // indication should get more and more stuck at low gyro spins
    if (spin < 0.9)
    {
        // when gyro spin is low, then any heading change results in
        // increasing the offset
        double diff = SGMiscd::normalizePeriodic(-180.0, 180.0, _last_heading_deg - heading);
        // scaled by 1-factor, so indication is fully stuck at spin==0 (offset compensates
        // any heading change)
        offset += diff * (1.0-factor);
    }
    _last_heading_deg = heading;

    // normalize offset
    offset = SGMiscd::normalizePeriodic(-180.0,180.0,offset);
    _offset_node->setDoubleValue(offset);

                                // No magvar - set the alignment manually
    double align = _align_node->getDoubleValue();

                                // Movement-induced error
    double yaw_rate = _yaw_rate_node->getDoubleValue();
    double error = _error_node->getDoubleValue();
    double g = _g_node->getDoubleValue();
    if ( fabs ( yaw_rate ) > 5 ) {
        error += 0.033 * -yaw_rate * dt * factor;
    }

    if ( g > 1.5 || g < -0.5){
        error += 0.033 * g * dt * factor;
    }
    _error_node->setDoubleValue(error);

                                // Now, we have to get the current
                                // heading and the last heading into
                                // the same range.
    while ((heading - _last_indicated_heading_dg) > 180)
        _last_indicated_heading_dg += 360;
    while ((heading - _last_indicated_heading_dg) < -180)
        _last_indicated_heading_dg -= 360;
    heading = fgGetLowPass(_last_indicated_heading_dg, heading, dt * 100);
    _last_indicated_heading_dg = heading;

    heading += offset + align + error;
    heading = SGMiscd::normalizePeriodic(0.0,360.0,heading);

    _heading_out_node->setDoubleValue(heading);

                                 // calculate the difference between the indicated heading
                                 // and the selected heading for use with an autopilot
    SGPropertyNode *bnode
        = fgGetNode( "/autopilot/settings/heading-bug-deg", false );
    if ( bnode ) {
        double diff = bnode->getDoubleValue() - heading;
        if ( diff < -180.0 ) { diff += 360.0; }
        if ( diff > 180.0 ) { diff -= 360.0; }
            _heading_bug_error_node->setDoubleValue( diff );
    }
                                 // calculate the difference between the indicated heading
                                 // and the selected nav1 radial for use with an autopilot
    SGPropertyNode *nnode
        = fgGetNode( "/instrumentation/nav/radials/selected-deg", false );
    if ( nnode ) {
        double diff = nnode->getDoubleValue() - heading;
        if ( diff < -180.0 ) { diff += 360.0; }
        if ( diff > 180.0 ) { diff -= 360.0; }
        _nav1_error_node->setDoubleValue( diff );
    }
}

// end of heading_indicator_dg.cxx
