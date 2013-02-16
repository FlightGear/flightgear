// pitot.cxx - the pitot air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/constants.h>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "pitot.hxx"


PitotSystem::PitotSystem ( SGPropertyNode *node )
    :
    _name(node->getStringValue("name", "pitot")),
    _num(node->getIntValue("number", 0))
{
}

PitotSystem::~PitotSystem ()
{
}

void
PitotSystem::init ()
{
    string branch;
    branch = "/systems/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _pressure_node = fgGetNode("/environment/pressure-inhg", true);
    _density_node = fgGetNode("/environment/density-slugft3", true);
    _velocity_node = fgGetNode("/velocities/airspeed-kt", true);
    _slip_angle = fgGetNode("/orientation/side-slip-rad", true);
    _total_pressure_node = node->getChild("total-pressure-inhg", 0, true);
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

#ifndef PSFTOINHG
# define PSFTOINHG (1/INHGTOPSF)
#endif


void
PitotSystem::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
                                // The pitot tube sees the forward
                                // velocity in the body axis.
        double p = _pressure_node->getDoubleValue() * INHGTOPSF;
        double r = _density_node->getDoubleValue();
        double v = _velocity_node->getDoubleValue() * SG_KT_TO_FPS;
        v *= cos(_slip_angle->getDoubleValue());
        if (v < 0.0)
            v = 0.0;
        double q = 0.5 * r * v * v; // dynamic
        _total_pressure_node->setDoubleValue((p + q) * PSFTOINHG);
    }
}

// end of pitot.cxx
