// static.cxx - the static air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "static.hxx"

#include <string>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>


StaticSystem::StaticSystem ( SGPropertyNode *node )
    :
    _name(node->getStringValue("name", "static")),
    _num(node->getIntValue("number", 0)),
    _tau(node->getDoubleValue("tau", 1))

{
}

StaticSystem::~StaticSystem ()
{
}

void
StaticSystem::init ()
{
    std::string branch = "/systems/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _pressure_in_node = fgGetNode("/environment/pressure-inhg", true);
    _pressure_out_node = node->getChild("pressure-inhg", 0, true);
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
        double trat = _tau ? dt/_tau : 100;
        double target = _pressure_in_node->getDoubleValue();
        double current = _pressure_out_node->getDoubleValue();
        // double delta = target - current;
        _pressure_out_node->setDoubleValue(fgGetLowPass(current, target, trat));
    }
}

// end of static.cxx
