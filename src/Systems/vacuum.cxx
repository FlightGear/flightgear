// vacuum.cxx - a vacuum pump connected to the aircraft engine.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include "vacuum.hxx"
#include <Main/fg_props.hxx>


VacuumSystem::VacuumSystem ()
{
}

VacuumSystem::~VacuumSystem ()
{
}

void
VacuumSystem::init ()
{
                                // TODO: allow index of pump and engine
                                // to be configured.
    _serviceable_node = fgGetNode("/systems/vacuum[0]/serviceable", true);
    _rpm_node = fgGetNode("/engines/engine[0]/rpm", true);
    _pressure_node = fgGetNode("/environment/pressure-inhg", true);
    _suction_node = fgGetNode("/systems/vacuum[0]/suction-inhg", true);
}

void
VacuumSystem::bind ()
{
}

void
VacuumSystem::unbind ()
{
}

void
VacuumSystem::update (double dt)
{
    // Model taken from steam.cxx

    double suction;

    if (!_serviceable_node->getBoolValue()) {
        suction = 0.0;
    } else {
        double rpm = _rpm_node->getDoubleValue();
        double pressure = _pressure_node->getDoubleValue();
        suction = pressure * rpm / (rpm + 10000.0);
        if (suction > 5.0)
            suction = 5.0;
    }
    _suction_node->setDoubleValue(suction);
}

// end of vacuum.cxx
