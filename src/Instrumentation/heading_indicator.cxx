// heading_indicator.cxx - a vacuum-powered heading indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include "heading_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


HeadingIndicator::HeadingIndicator ( SGPropertyNode *node )
    :
    name("heading-indicator"),
    num(0),
    vacuum_system("/systems/vacuum")
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
        } else if ( cname == "vacuum-system" ) {
            vacuum_system = cval;
        } else {
            SG_LOG( SG_INSTR, SG_WARN, "Error in heading-indicator config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}

HeadingIndicator::HeadingIndicator ()
{
}

HeadingIndicator::~HeadingIndicator ()
{
}

void
HeadingIndicator::init ()
{
    string branch;
    branch = "/instrumentation/" + name;
    vacuum_system += "/suction-inhg";

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _offset_node = node->getChild("offset-deg", 0, true);
    _heading_in_node = fgGetNode("/orientation/heading-deg", true);
    _suction_node = fgGetNode(vacuum_system.c_str(), true);
    _heading_out_node = node->getChild("indicated-heading-deg", 0, true);
    _last_heading_deg = (_heading_in_node->getDoubleValue() +
                         _offset_node->getDoubleValue());

    //_serviceable_node->setBoolValue(true);
}

void
HeadingIndicator::bind ()
{
    string branch;
    branch = "/instrumentation/" + name + "/serviceable";
    fgTie(branch.c_str(),
          &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    branch = "/instrumentation/" + name + "/spin";
    fgTie(branch.c_str(),
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
HeadingIndicator::unbind ()
{
    string branch;
    branch = "/instrumentation/" + name + "/serviceable";
    fgUntie(branch.c_str());
    branch = "/instrumentation/" + name + "/spin";
    fgUntie(branch.c_str());
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

    heading = fgGetLowPass(_last_heading_deg, heading, dt/factor);
    _last_heading_deg = heading;

    heading += offset;
    while (heading < 0)
        heading += 360;
    while (heading > 360)
        heading -= 360;

    _heading_out_node->setDoubleValue(heading);
}

// end of heading_indicator.cxx
