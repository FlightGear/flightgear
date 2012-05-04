// heading_indicator_fg.cxx - a flux_gate compass.
// Based on the vacuum driven Heading Indicator Written by David Megginson, started 2002.
//
// Written by Vivian Meazza, started 2005.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/compiler.h>
#include <iostream>
#include <string>
#include <sstream>

#include <simgear/magvar/magvar.hxx>
#include <simgear/math/SGMath.hxx>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "heading_indicator_fg.hxx"

using std::string;

HeadingIndicatorFG::HeadingIndicatorFG ( SGPropertyNode *node )
    :
    name("heading-indicator-fg"),	 
    num(0)
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_WARN, "Error in flux-gate heading-indicator config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}

HeadingIndicatorFG::HeadingIndicatorFG ()
{
}

HeadingIndicatorFG::~HeadingIndicatorFG ()
{
}

void
HeadingIndicatorFG::init ()
{
    string branch;
    branch = "/instrumentation/" + name;
    
	_heading_in_node = fgGetNode("/orientation/heading-deg", true);

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    if( NULL == (_offset_node = node->getChild("offset-deg", 0, false)) ) {
      _offset_node = node->getChild("offset-deg", 0, true);
      _offset_node->setDoubleValue( -globals->get_mag()->get_magvar() * SGD_RADIANS_TO_DEGREES );
    }
    _serviceable_node = node->getChild("serviceable", 0, true);
	_error_node = node->getChild("heading-bug-error-deg", 0, true);
	_nav1_error_node = node->getChild("nav1-course-error-deg", 0, true);
    _heading_out_node = node->getChild("indicated-heading-deg", 0, true);
    _off_node         = node->getChild("off-flag", 0, true);

    _last_heading_deg = (_heading_in_node->getDoubleValue() +
                         _offset_node->getDoubleValue());
	_electrical_node = fgGetNode("/systems/electrical/outputs/DG", true);
}

void
HeadingIndicatorFG::bind ()
{
    std::ostringstream temp;
    string branch;
    temp << num;
    branch = "/instrumentation/" + name + "[" + temp.str() + "]";

    fgTie((branch + "/serviceable").c_str(),
          &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    fgTie((branch + "/spin").c_str(),
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
	
}

void
HeadingIndicatorFG::unbind ()
{
    std::ostringstream temp;
    string branch;
    temp << num;
    branch = "/instrumentation/" + name + "[" + temp.str() + "]";

    fgUntie((branch + "/serviceable").c_str());
    fgUntie((branch + "/spin").c_str());
}

void
HeadingIndicatorFG::update (double dt)
{
                                // Get the spin from the gyro
	 _gyro.set_power_norm(_electrical_node->getDoubleValue());
	_gyro.update(dt);
    double spin = _gyro.get_spin_norm();

    if ( _electrical_node->getDoubleValue() > 0 && spin >= 0.25) {
        _off_node->setBoolValue(false);
    } else {
        _off_node->setBoolValue(true);
        return;
    }

                                // No time-based precession	for a flux gate compass
	                            // We just use offset to get the magvar
	double offset = _offset_node->getDoubleValue();
	   
                                // TODO: movement-induced error

                                // Next, calculate the indicated heading,
                                // introducing errors.
    double factor = 100 * (spin * spin * spin * spin * spin * spin);
    double heading = _heading_in_node->getDoubleValue();

                                // Now, we have to get the current
                                // heading and the last heading into
                                // the same range.
    if ((heading - _last_heading_deg) > 180)
        _last_heading_deg += 360;
    if ((heading - _last_heading_deg) < -180)
        _last_heading_deg -= 360;

    heading = fgGetLowPass(_last_heading_deg, heading, dt * factor);
    _last_heading_deg = heading;

	heading += offset;

    if (heading < 0)
        heading += 360;
    if (heading > 360)
        heading -= 360;

    _heading_out_node->setDoubleValue(heading);

	                             // calculate the difference between the indicated heading
	                             // and the selected heading for use with an autopilot
	static SGPropertyNode *bnode
        = fgGetNode( "/autopilot/settings/heading-bug-deg", false );
	double diff = 0;
	if ( bnode ){
		diff = bnode->getDoubleValue() - heading;
		if ( diff < -180.0 ) { diff += 360.0; }
		if ( diff > 180.0 ) { diff -= 360.0; }
		_error_node->setDoubleValue( diff );
	}   
	                             // calculate the difference between the indicated heading
	                             // and the selected nav1 radial for use with an autopilot
	SGPropertyNode *nnode
        = fgGetNode( "/instrumentation/nav/radials/selected-deg", true );
	double ndiff = 0;
	if ( nnode ){
		ndiff = nnode->getDoubleValue() - heading;
		if ( ndiff < -180.0 ) { ndiff += 360.0; }
		if ( ndiff > 180.0 ) { ndiff -= 360.0; }
		_nav1_error_node->setDoubleValue( ndiff );
	}   


}

// end of heading_indicator_fg.cxx
