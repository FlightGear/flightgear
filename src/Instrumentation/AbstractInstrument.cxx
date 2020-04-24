// Copyright (C) 2019 James Turner <james@flightgear.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "config.h"

#include <simgear/debug/logstream.hxx>

#include <Instrumentation/AbstractInstrument.hxx>
#include <Main/fg_props.hxx>

void AbstractInstrument::readConfig(SGPropertyNode* config,
                                    std::string defaultName)
{
    _name = config->getStringValue("name", defaultName.c_str());
    _index = config->getIntValue("number", 0);
    if (_powerSupplyPath.empty()) {
        _powerSupplyPath = "/systems/electrical/outputs/" + defaultName;
    }
    
    if (config->hasChild("power-supply")) {
        _powerSupplyPath = config->getStringValue("power-supply");
    }
    
    // the default output values are volts, but various places have been
    // treating the value as a bool, so we default to 1.0 as our minimum
    // supply volts
    _minimumSupplyVolts = config->getDoubleValue("minimum-supply-volts", 1.0);
}

std::string AbstractInstrument::nodePath() const
{
    return "/instrumentation/" + _name + "[" + std::to_string(_index) + "]";
}

void AbstractInstrument::initServicePowerProperties(SGPropertyNode* node)
{
    _serviceableNode = node->getNode("serviceable", 0, true);
    if (_serviceableNode->getType() == simgear::props::NONE)
        _serviceableNode->setBoolValue(true);
    
    _powerButtonNode = node->getChild("power-btn", 0, true);
    
    // if the user didn't define a node, default to true
    if (_powerButtonNode->getType() == simgear::props::NONE)
        _powerButtonNode->setBoolValue(true);
    
    if (_powerSupplyPath != "NO_DEFAULT") {
        _powerSupplyNode = fgGetNode(_powerSupplyPath, true);
    }
    
    node->tie( "operable", SGRawValueMethods<AbstractInstrument,bool>
               ( *this, &AbstractInstrument::isServiceableAndPowered ) );
}

void AbstractInstrument::unbind()
{
    auto nd = fgGetNode(nodePath());
    if (nd) {
        nd->untie("operable");
    }
}

bool AbstractInstrument::isServiceableAndPowered() const
{
    if (!_serviceableNode->getBoolValue() || !isPowerSwitchOn())
        return false;
    
    if (_powerSupplyNode && (_powerSupplyNode->getDoubleValue() < _minimumSupplyVolts))
        return false;
    
    return true;
}

void AbstractInstrument::setDefaultPowerSupplyPath(const std::string &p)
{
    _powerSupplyPath = p;
}

void AbstractInstrument::setMinimumSupplyVolts(double v)
{
    _minimumSupplyVolts = v;
}

bool AbstractInstrument::isPowerSwitchOn() const
{
    return _powerButtonNode->getBoolValue();
}
