// heading_indicator.cxx - a vacuum-powered heading indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/compiler.h>
#include STL_IOSTREAM
#include STL_STRING
#include <sstream>

#include "heading_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


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
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _offset_node = node->getChild("offset-deg", 0, true);
    _heading_in_node = fgGetNode("/orientation/heading-deg", true);
    _suction_node = fgGetNode(_suction.c_str(), true);
    _heading_out_node = node->getChild("indicated-heading-deg", 0, true);
    _last_heading_deg = (_heading_in_node->getDoubleValue() +
                         _offset_node->getDoubleValue());
}

void
HeadingIndicator::bind ()
{
    std::ostringstream temp;
    string branch;
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
    string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";

    fgUntie((branch + "/serviceable").c_str());
    fgUntie((branch + "/spin").c_str());
}

void
HeadingIndicator::update (double dt)
{
    timingInfo.clear();
    stamp("begin");
                                // Get the spin from the gyro
    _gyro.set_power_norm(_suction_node->getDoubleValue()/5.0);
    //stamp("HDG_01");
    _gyro.update(dt);
    //stamp("HDG_02");
    double spin = _gyro.get_spin_norm();
    //stamp("HDG_03");

                                // Next, calculate time-based precession
    double offset = _offset_node->getDoubleValue();
    offset -= dt * (0.25 / 60.0); // 360deg/day
    while (offset < -360)
        offset += 360;
    while (offset > 360)
        offset -= 360;
    _offset_node->setDoubleValue(offset);

                                // TODO: movement-induced error

                                // Next, calculate the indicated heading,
                                // introducing errors.
    double factor = 0.01 / (spin * spin * spin * spin * spin * spin);
    double heading = _heading_in_node->getDoubleValue();

                                // Now, we have to get the current
                                // heading and the last heading into
                                // the same range.
    while ((heading - _last_heading_deg) > 180)
        _last_heading_deg += 360;
    while ((heading - _last_heading_deg) < -180)
        _last_heading_deg -= 360;
    //stamp("HDG_04");
    heading = fgGetLowPass(_last_heading_deg, heading, dt/factor);
    //stamp("HDG_05");
    _last_heading_deg = heading;

    heading += offset;
    while (heading < 0)
        heading += 360;
    while (heading > 360)
        heading -= 360;
    stamp("HDG_06");
    _heading_out_node->setDoubleValue(heading);
    stamp("HDG_07");
}

// end of heading_indicator.cxx
