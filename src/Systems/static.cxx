// static.cxx - the static air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include "static.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


StaticSystem::StaticSystem ( SGPropertyNode *node )
    :
    name("static"),
    num(0)
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_AUTOPILOT, SG_WARN, "Error in systems config logic" );
            if ( name.length() ) {
                SG_LOG( SG_AUTOPILOT, SG_WARN, "Section = " << name );
            }
        }
    }
}

StaticSystem::StaticSystem ( int i )
{
    name = "static";
    num = i;
}

StaticSystem::~StaticSystem ()
{
}

void
StaticSystem::init ()
{
    string branch;
    branch = "/systems/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _pressure_in_node = fgGetNode("/environment/pressure-inhg", true);
    _pressure_out_node = node->getChild("pressure-inhg", 0, true);

    _serviceable_node->setBoolValue(true);
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
