// vacuum.cxx - a vacuum pump connected to the aircraft engine.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include "vacuum.hxx"
#include <Main/fg_props.hxx>


VacuumSystem::VacuumSystem( int i )
{
    num = i;
}

VacuumSystem::~VacuumSystem ()
{
}

void
VacuumSystem::init()
{
                                // TODO: allow index of engine to be
                                // configured.
    SGPropertyNode *node = fgGetNode("/systems/vacuum", num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _rpm_node = fgGetNode("/engines/engine[0]/rpm", true);
    _pressure_node = fgGetNode("/environment/pressure-inhg", true);
    _suction_node = node->getChild("suction-inhg", 0, true);
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
        // This magic formula yields about 4 inhg at 700 rpm
        suction = pressure * rpm / (rpm + 4875.0);

        // simple regulator model that clamps smoothly to about 5 inhg
        // over a normal rpm range
        double max = (rpm > 0 ? 5.39 - 1.0 / ( rpm * 0.00111 ) : 0);
        if ( suction < 0.0 ) suction = 0.0;
        if ( suction > max ) suction = max;
    }
    _suction_node->setDoubleValue(suction);
}

// end of vacuum.cxx
