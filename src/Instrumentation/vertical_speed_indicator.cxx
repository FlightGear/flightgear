// vertical_speed_indicator.cxx - a regular VSI.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/math/interpolater.hxx>

#include "vertical_speed_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


VerticalSpeedIndicator::VerticalSpeedIndicator ( SGPropertyNode *node )
    : _internal_pressure_inhg(29.92),
      name("vertical-speed-indicator"),
      num(0),
      static_port("/systems/static")
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
        } else if ( cname == "static-port" ) {
            static_port = cval;
        } else {
            SG_LOG( SG_INSTR, SG_WARN, "Error in vertical-speed-indicator config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}

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
    string branch;
    branch = "/instrumentation/" + name;
    static_port += "/pressure-inhg";

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _pressure_node = fgGetNode(static_port.c_str(), true);
    _speed_node = node->getChild("indicated-speed-fpm", 0, true);

    _serviceable_node->setBoolValue(true);
                                // Initialize at ambient pressure
    _internal_pressure_inhg = _pressure_node->getDoubleValue();
}

void
VerticalSpeedIndicator::update (double dt)
{
                                // model taken from steam.cxx, with change
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
