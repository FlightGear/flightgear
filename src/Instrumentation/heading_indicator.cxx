// heading_indicator.cxx - a vacuum-powered heading indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include "heading_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


HeadingIndicator::HeadingIndicator ()
{
}

HeadingIndicator::~HeadingIndicator ()
{
}

void
HeadingIndicator::init ()
{
    _offset_node =
        fgGetNode("/instrumentation/heading-indicator/offset-deg", true);
    _heading_in_node = fgGetNode("/orientation/heading-deg", true);
    _suction_node = fgGetNode("/systems/vacuum[0]/suction-inhg", true);
    _heading_out_node =
        fgGetNode("/instrumentation/heading-indicator/indicated-heading-deg",
                  true);
    _last_heading_deg = (_heading_in_node->getDoubleValue() +
                         _offset_node->getDoubleValue());
}

void
HeadingIndicator::bind ()
{
    fgTie("/instrumentation/heading-indicator/serviceable",
          &_gyro, &Gyro::is_serviceable);
    fgTie("/instrumentation/heading-indicator/spin",
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
HeadingIndicator::unbind ()
{
    fgUntie("/instrumentation/heading-indicator/serviceable");
    fgUntie("/instrumentation/heading-indicator/spin");
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
