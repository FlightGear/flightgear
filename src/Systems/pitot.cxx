// pitot.cxx - the pitot air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include "pitot.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


PitotSystem::PitotSystem ()
{
}

PitotSystem::~PitotSystem ()
{
}

void
PitotSystem::init ()
{
    _serviceable_node = fgGetNode("/systems/pitot[0]/serviceable", true);
    _pressure_node = fgGetNode("/environment/pressure-inhg", true);
    _density_node = fgGetNode("/environment/density-slugft3", true);
    _velocity_node = fgGetNode("/velocities/uBody-fps", true);
    _total_pressure_node =
        fgGetNode("/systems/pitot[0]/total-pressure-inhg", true);
}

void
PitotSystem::bind ()
{
}

void
PitotSystem::unbind ()
{
}

#ifndef INHGTOPSF
# define INHGTOPSF (2116.217/29.9212)
#endif

void
PitotSystem::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
                                // The pitot tube sees the forward
                                // velocity in the body axis.
        double p = _pressure_node->getDoubleValue(); // static
        double r = _density_node->getDoubleValue();
        double v = _velocity_node->getDoubleValue();
        double q = 0.5 * r * v * v / INHGTOPSF; // dynamic
        _total_pressure_node->setDoubleValue(p + q);
    }
}

// end of pitot.cxx
