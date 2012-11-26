// pitot.cxx - the pitot air system.
// Written by David Megginson, started 2002.
//
// Last modified by Eric van den Berg, 24 Nov 2012
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
    _mach_node = fgGetNode("/velocities/mach", true);
    _total_pressure_node = node->getChild("total-pressure-inhg", 0, true);
    _measured_total_pressure_node = node->getChild("measured-total-pressure-inhg", 0, true);
}

void
PitotSystem::bind ()
{
}

void
PitotSystem::unbind ()
{
}

void
PitotSystem::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
        double p = _pressure_node->getDoubleValue();
        double mach = _mach_node->getDoubleValue();
        mach = std::max( mach , 0.0 );
        double p_t = p * pow(1 + 0.2 * mach*mach, 3.5 );    // true total pressure around aircraft
        _total_pressure_node->setDoubleValue(p_t);
        double p_t_meas = p_t;
        if (mach > 1) {    
          p_t_meas = p * pow( 1.2 * mach*mach, 3.5 ) * pow( 2.8/2.4*mach*mach - 0.4 / 2.4 , -2.5 );    // measured total pressure by pitot tube (Rayleigh formula, at Mach>1, normal shockwave in front of pitot tube)     
        }
        _measured_total_pressure_node->setDoubleValue(p_t_meas);
    }
}

// end of pitot.cxx
