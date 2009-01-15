// vertical_speed_indicator.cxx - a regular VSI.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/math/interpolater.hxx>
#include <simgear/math/SGMath.hxx>
#include "vertical_speed_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


VerticalSpeedIndicator::VerticalSpeedIndicator ( SGPropertyNode *node )
    : _internal_pressure_inhg(29.92),
      _name(node->getStringValue("name", "vertical-speed-indicator")),
      _num(node->getIntValue("number", 0)),
      _static_pressure(node->getStringValue("static-pressure", "/systems/static/pressure-inhg"))
{
}

VerticalSpeedIndicator::~VerticalSpeedIndicator ()
{
}

void
VerticalSpeedIndicator::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _pressure_node = fgGetNode(_static_pressure.c_str(), true);
    _speed_node = node->getChild("indicated-speed-fpm", 0, true);

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
        double new_pressure = _pressure_node->getDoubleValue();
        if (dt > 0.0 && 
              fabs(_internal_pressure_inhg - new_pressure)/dt > 50.) {
          SG_LOG( SG_COCKPIT, SG_DEBUG, "VSI snap: " << dt 
              << "  internal: " << _internal_pressure_inhg
              << "  new: " << new_pressure);
          _internal_pressure_inhg = new_pressure;
        } else {
          _internal_pressure_inhg =
              fgGetLowPass(_internal_pressure_inhg, new_pressure, dt/6.0);
        }
    }
}

// end of vertical_speed_indicator.cxx
