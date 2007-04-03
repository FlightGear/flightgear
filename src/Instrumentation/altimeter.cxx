// altimeter.cxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
// Modified by John Denker in 2007 to use a two layer atmosphere
// model in src/Environment/atmosphere.?xx
//
// This file is in the Public Domain and comes with no warranty.

// Example invocation, in the instrumentation.xml file:
//      <altimeter>
//        <name>encoder</name>
//        <number>0</number>
//        <static-pressure>/systems/static/pressure-inhg</static-pressure>
//        <quantum>10</quantum>
//        <tau>0</tau>
//      </altimeter>
// Note non-default name, quantum, and tau values.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/interpolater.hxx>
#include <simgear/math/SGMath.hxx>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <Environment/atmosphere.hxx>

#include "altimeter.hxx"

Altimeter::Altimeter ( SGPropertyNode *node, double quantum )
    : _name(node->getStringValue("name", "altimeter")),
      _num(node->getIntValue("number", 0)),
      _static_pressure(node->getStringValue("static-pressure", "/systems/static/pressure-inhg")),
      _tau(node->getDoubleValue("tau", 0.1)),
      _quantum(node->getDoubleValue("quantum", quantum))
{}

Altimeter::~Altimeter ()
{}

void
Altimeter::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    raw_PA = 0.0;
    _pressure_node     = fgGetNode(_static_pressure.c_str(), true);
    _serviceable_node  = node->getChild("serviceable", 0, true);
    _setting_node      = node->getChild("setting-inhg", 0, true);
    _press_alt_node    = node->getChild("pressure-alt-ft", 0, true);
    _mode_c_node       = node->getChild("mode-c-alt-ft", 0, true);
    _altitude_node     = node->getChild("indicated-altitude-ft", 0, true);

    if (_setting_node->getDoubleValue() == 0)
        _setting_node->setDoubleValue(29.921260);
}

void
Altimeter::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
        double trat = _tau > 0 ? dt/_tau : 100;
        double pressure = _pressure_node->getDoubleValue();
        double setting = _setting_node->getDoubleValue();
        double press_alt = _press_alt_node->getDoubleValue();
        // The mechanism settles slowly toward new pressure altitude:
        raw_PA = fgGetLowPass(raw_PA, _altimeter.press_alt_ft(pressure), trat);
        _mode_c_node->setDoubleValue(100 * SGMiscd::round(raw_PA/100));
        _kollsman = fgGetLowPass(_kollsman, _altimeter.kollsman_ft(setting), trat);
        if (_quantum)
            press_alt = _quantum * SGMiscd::round(raw_PA/_quantum);
        else
            press_alt = raw_PA;
        _press_alt_node->setDoubleValue(press_alt);
        _altitude_node->setDoubleValue(press_alt - _kollsman);
    }
}

// end of altimeter.cxx
