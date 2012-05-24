// heading_indicator.cxx - a vacuum-powered heading indicator.
// Written by David Megginson, started 2002.
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

#include "heading_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include <simgear/magvar/magvar.hxx>

HeadingIndicator::HeadingIndicator ( SGPropertyNode *node )
    :
    _name(node->getStringValue("name", "heading-indicator")),
    _num(node->getIntValue("number", 0)),
    _suction(node->getStringValue("suction", "/systems/vacuum/suction-inhg"))
{
}

HeadingIndicator::~HeadingIndicator ()
{
}

void
HeadingIndicator::init ()
{
    std::string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    if( NULL == (_offset_node = node->getChild("offset-deg", 0, false)) ) {
      _offset_node = node->getChild("offset-deg", 0, true);
      _offset_node->setDoubleValue( -globals->get_mag()->get_magvar() * SGD_RADIANS_TO_DEGREES );
    }
    _heading_in_node = fgGetNode("/orientation/heading-deg", true);
    _suction_node = fgGetNode(_suction.c_str(), true);
    _heading_out_node = node->getChild("indicated-heading-deg", 0, true);
    _heading_bug_error_node = node->getChild("heading-bug-error-deg", 0, true);
    _heading_bug_node = node->getChild("heading-bug-deg", 0, true);
    _last_heading_deg = (_heading_in_node->getDoubleValue() +
                         _offset_node->getDoubleValue());
}

void
HeadingIndicator::bind ()
{
    std::ostringstream temp;
    std::string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";

    fgTie((branch + "/serviceable").c_str(),
          &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    fgTie((branch + "/spin").c_str(),
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
HeadingIndicator::unbind ()
{
    std::ostringstream temp;
    std::string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";

    fgUntie((branch + "/serviceable").c_str());
    fgUntie((branch + "/spin").c_str());
}

void
HeadingIndicator::update (double dt)
{
                                // Get the spin from the gyro
    _gyro.set_power_norm(_suction_node->getDoubleValue()/5.0);
    _gyro.update(dt);
    double spin = _gyro.get_spin_norm();

                                // Next, calculate time-based precession
    double offset = _offset_node->getDoubleValue();
    offset -= dt * (0.25 / 60.0); // 360deg/day
    SG_NORMALIZE_RANGE(offset, -360.0, 360.0);

                                // TODO: movement-induced error

                                // Next, calculate the indicated heading,
                                // introducing errors.
    double factor = 100 * (spin * spin * spin * spin * spin * spin);
    double heading = _heading_in_node->getDoubleValue();

                                // Now, we have to get the current
                                // heading and the last heading into
                                // the same range.
    while ((heading - _last_heading_deg) > 180)
        _last_heading_deg += 360;
    while ((heading - _last_heading_deg) < -180)
        _last_heading_deg -= 360;

    heading = fgGetLowPass(_last_heading_deg, heading, dt * factor);
    _last_heading_deg = heading;

    heading += offset;
    SG_NORMALIZE_RANGE(heading, 0.0, 360.0);

    _heading_out_node->setDoubleValue(heading);

    // Calculate heading bug error normalized to +/- 180.0
    double heading_bug = _heading_bug_node->getDoubleValue();
    double diff = heading_bug - heading;

    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    _heading_bug_error_node->setDoubleValue( diff );
}

// end of heading_indicator.cxx
