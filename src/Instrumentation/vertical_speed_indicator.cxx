// vertical_speed_indicator.cxx - a regular VSI.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/math/interpolater.hxx>

#include "vertical_speed_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


VerticalSpeedIndicator::VerticalSpeedIndicator ()
    : _internal_pressure_inhg(29.92)
{
}

VerticalSpeedIndicator::~VerticalSpeedIndicator ()
{
}

void
VerticalSpeedIndicator::init ()
{
    _serviceable_node =
        fgGetNode("/instrumentation/vertical-speed-indicator/serviceable",
                  true);
    _pressure_node =
        fgGetNode("/systems/static/pressure-inhg", true);
    _speed_node =
        fgGetNode("/instrumentation/vertical-speed-indicator/indicated-speed-fpm",
                  true);
}

void
VerticalSpeedIndicator::bind ()
{
}

void
VerticalSpeedIndicator::unbind ()
{
}

void
VerticalSpeedIndicator::update (double dt)
{
                                // model take from steam.cxx, with change
                                // from 10000 to 10500 for manual factor
    if (_serviceable_node->getBoolValue()) {
        double pressure = _pressure_node->getDoubleValue();
        _speed_node
            ->setDoubleValue((_internal_pressure_inhg - pressure) * 10500);
        _internal_pressure_inhg =
            fgGetLowPass(_internal_pressure_inhg,
                         _pressure_node->getDoubleValue(),
                         dt/6.0);
    }
}

// end of vertical_speed_indicator.cxx
