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
    _serviceable_node =
        fgGetNode("/instrumentation/heading-indicator/serviceable", true);
    _spin_node =
        fgGetNode("/instrumentation/heading-indicator/spin", true);
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
}

void
HeadingIndicator::unbind ()
{
}

void
HeadingIndicator::update (double dt)
{
                                // First, calculate the bogo-spin from 0 to 1.
                                // All numbers are made up.

    double spin = _spin_node->getDoubleValue();
    spin -= 0.005 * dt;         // spin decays every 0.5% every second.

                                // spin increases up to 25% every second
                                // if suction is available and the gauge
                                // is serviceable.
    if (_serviceable_node->getBoolValue()) {
        double suction = _suction_node->getDoubleValue();
        double step = 0.25 * (suction / 5.0) * dt;
        if ((spin + step) <= (suction / 5.0))
            spin += step;
    }
    if (spin > 1.0)
        spin = 1.0;
    else if (spin < 0.0)
        spin = 0.0;
    _spin_node->setDoubleValue(spin);

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
