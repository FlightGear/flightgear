// static.cxx - the static air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include "static.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


StaticSystem::StaticSystem ()
{
}

StaticSystem::~StaticSystem ()
{
}

void
StaticSystem::init ()
{
    _serviceable_node = fgGetNode("/systems/static[0]/serviceable", true);
    _pressure_in_node = fgGetNode("/environment/pressure-inhg", true);
    _pressure_out_node = fgGetNode("/systems/static[0]/pressure-inhg", true);
}

void
StaticSystem::bind ()
{
}

void
StaticSystem::unbind ()
{
}

void
StaticSystem::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
        
        double target = _pressure_in_node->getDoubleValue();
        double current = _pressure_out_node->getDoubleValue();
        // double delta = target - current;
        _pressure_out_node->setDoubleValue(fgGetLowPass(current, target, dt));
    }
}

// end of static.cxx
