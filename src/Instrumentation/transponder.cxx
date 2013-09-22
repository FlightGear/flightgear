// transponder.cxx -- class to impliment a transponder
//
// Written by Roy Vegard Ovesen, started September 2004.
//
// Copyright (C) 2004  Roy Vegard Ovesen - rvovesen@tiscali.no
// Copyright (C) 2013  Clement de l'Hamaide - clemaez@hotmail.fr
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
//
// Example invocation, in the instrumentation.xml file:
//      <transponder>
//        <name>encoder</name>
//        <number>0</number>
//        <mode>0</mode>  // Mode A = 0, Mode C = 1, Mode S = 2
//      </altimeter>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "transponder.hxx"

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>

#include <Main/fg_props.hxx>

#include <cstdio>

using std::string;

const double IDENT_TIMEOUT = 18.0; // 18 seconds
const int INVALID_ALTITUDE = -9999;
const int INVALID_ID = -9999;

Transponder::Transponder(SGPropertyNode *node)
    :
    _identMode(false),
    _name(node->getStringValue("name", "transponder")),
    _num(node->getIntValue("number", 0)),
    _mode((Mode) node->getIntValue("mode", 1)),
    _listener_active(0)
{
    _requiredBusVolts = node->getDoubleValue("bus-volts", 8.0);
    _altitudeSourcePath = node->getStringValue("encoder-path", "/instrumentation/altimeter");
    _kt70Compat = node->getBoolValue("kt70-compatibility", false);
}


Transponder::~Transponder()
{
}


void Transponder::init()
{
    SGPropertyNode *node = fgGetNode("/instrumentation/" + _name, _num, true );

    // Inputs
    _busPower_node = fgGetNode("/systems/electrical/outputs/transponder", true);
    _pressureAltitude_node = fgGetNode(_altitudeSourcePath, true);

    SGPropertyNode *in_node = node->getChild("inputs", 0, true);
    for (int i=0; i<4;++i) {
        _digit_node[i] = in_node->getChild("digit", i, true);
        _digit_node[i]->addChangeListener(this);
    }
   
    _knob_node = in_node->getChild("knob-mode", 0, true);
    if (!_knob_node->hasValue()) {
        _knob = KNOB_ON;
        // default to, if aircraft wants to start dark, it can do this
        // in its -set.xml
        _knob_node->setIntValue(_knob);
    } else {
        _knob = static_cast<KnobPosition>(_knob_node->getIntValue());
    }
    
    _knob_node->addChangeListener(this);
    
    _mode_node = in_node->getChild("mode", 0, true);
    _mode_node->setIntValue(_mode);
    _mode_node->addChangeListener(this);
    
    _identBtn_node = in_node->getChild("ident-btn", 0, true);
    _identBtn_node->setBoolValue(false);
    _identBtn_node->addChangeListener(this);
    
    _serviceable_node = node->getChild("serviceable", 0, true);
    _serviceable_node->setBoolValue(true);
    
    _idCode_node = node->getChild("id-code", 0, true);
    _idCode_node->addChangeListener(this);
    // set default, but don't overwrite value from preferences.xml or -set.xml
    if (!_idCode_node->hasValue()) { 
        _idCode_node->setIntValue(1200);
    }
    
    // Outputs
    _altitude_node = node->getChild("altitude", 0, true);
    _altitudeValid_node = node->getChild("altitude-valid", 0, true);
    _ident_node = node->getChild("ident", 0, true);
    _transmittedId_node = node->getChild("transmitted-id", 0, true);
    
    if (_kt70Compat) {
        // alias the properties through
        SGPropertyNode_ptr output = node->getChild("outputs", 0, true);
        output->getChild("flight-level", 0, true)->alias(_altitude_node);
        output->getChild("id-code", 0, true)->alias(_idCode_node);
        in_node->getChild("func-knob", 0, true)->alias(_knob_node);
    }
}

void Transponder::bind()
{
    if (_kt70Compat) {
        SGPropertyNode *node = fgGetNode("/instrumentation/" + _name, _num, true );
        _tiedProperties.setRoot(node);
        
        _tiedProperties.Tie("annunciators/fl", this,
                            &Transponder::getFLAnnunciator );
        _tiedProperties.Tie("annunciators/alt", this,
                            &Transponder::getAltAnnunciator );
        _tiedProperties.Tie("annunciators/gnd", this,
                            &Transponder::getGroundAnnuciator );
        _tiedProperties.Tie("annunciators/on", this,
                            &Transponder::getOnAnnunciator );
        _tiedProperties.Tie("annunciators/sby", this,
                            &Transponder::getStandbyAnnunciator );
        _tiedProperties.Tie("annunciators/reply", this,
                            &Transponder::getReplyAnnunciator );
    } // of kt70 backwards compatibility
}

void Transponder::unbind()
{
    _tiedProperties.Untie();
}


void Transponder::update(double dt)
{
    if (has_power() && _serviceable_node->getBoolValue())
    {
        // Mode C & S send also altitude
        Mode effectiveMode = (_knob == KNOB_ALT) ? _mode : MODE_A;
        SGPropertyNode* altitudeSource = NULL;
        
        switch (effectiveMode) {
        case MODE_C:
            altitudeSource = _pressureAltitude_node->getChild("mode-c-alt-ft");
            break;
        case MODE_S:
            altitudeSource = _pressureAltitude_node->getChild("mode-s-alt-ft");
            break;
        default:
            break;
        }
        
        int alt = INVALID_ALTITUDE;
        if (effectiveMode != MODE_A) {
            if (altitudeSource) {
                alt = altitudeSource->getIntValue();
            } else {
                // warn if altitude input is misconfigured
                SG_LOG(SG_INSTR, SG_INFO, "transponder altitude input for mode " << _mode << " is missing");
            }
        }
        
        _altitude_node->setIntValue(alt);
        _altitudeValid_node->setBoolValue(alt != INVALID_ALTITUDE);

        if ( _identMode ) {
            _identTime += dt;
            if ( _identTime > IDENT_TIMEOUT ) {
                // reset ident mode
                _ident_node->setBoolValue(false);
                _identMode = false;
            }
        }
        
        if (_knob >= KNOB_ON) {
            _transmittedId_node->setIntValue(_idCode_node->getIntValue());
        } else {
            _transmittedId_node->setIntValue(INVALID_ID);
        }
    }
    else
    {
      _altitude_node->setIntValue(INVALID_ALTITUDE);
      _altitudeValid_node->setBoolValue(false);
      _ident_node->setBoolValue(false);
      _transmittedId_node->setIntValue(INVALID_ID);
    }
}

static int powersOf10[4] = {1, 10, 100, 1000};

static int extractCodeDigit(int code, int index)
{
    return (code / powersOf10[index]) % 10;
}

static int modifyCodeDigit(int code, int index, int digitValue)
{
    assert(digitValue >= 0 && digitValue < 8);
    int p = powersOf10[index];
    int codeWithoutDigit = code - (extractCodeDigit(code, index) * p);
    return codeWithoutDigit + (digitValue * p);
}

void Transponder::valueChanged(SGPropertyNode *prop)
{
    // Ident button pressed
    if (prop == _identBtn_node) {
        if (prop->getBoolValue()) {
            _identTime = 0.0;
            _ident_node->setBoolValue(true);
            _identMode = true;
        } else {
            // don't cancel state on release
        }
        return;
    }
    
    if (prop == _mode_node) {
        _mode = static_cast<Mode>(prop->getIntValue());
        return;
    }
    
    if (prop == _knob_node) {
        _knob = static_cast<KnobPosition>(prop->getIntValue());
        return;
    }
    
    if (_listener_active)
        return;

    _listener_active++;

    if (prop == _idCode_node) {
        // keep the digits in-sync
        for (int i=0; i<4; ++i) {
            _digit_node[i]->setIntValue(extractCodeDigit(prop->getIntValue(), i));
        }
    } else {
        // digit node
        int index = prop->getIndex();
        int digitValue = prop->getIntValue();
        SG_CLAMP_RANGE<int>(digitValue, 0, 7);
        _idCode_node->setIntValue(modifyCodeDigit(_idCode_node->getIntValue(), index, digitValue));
        prop->setIntValue(digitValue);
    }
    
    _listener_active--;
}

bool Transponder::has_power() const
{
    return (_knob_node->getIntValue() > KNOB_STANDBY) && (_busPower_node->getDoubleValue() > _requiredBusVolts);
}

bool Transponder::getFLAnnunciator() const
{
    return (_knob == KNOB_ALT) || (_knob == KNOB_GROUND) || (_knob == KNOB_TEST);
}

bool Transponder::getAltAnnunciator() const
{
    return (_knob == KNOB_ALT) || (_knob == KNOB_TEST);
}

bool Transponder::getGroundAnnuciator() const
{
    return (_knob == KNOB_GROUND) || (_knob == KNOB_TEST);
}

bool Transponder::getOnAnnunciator() const
{
    return (_knob == KNOB_ON) || (_knob == KNOB_TEST);
}

bool Transponder::getStandbyAnnunciator() const
{
    return (_knob == KNOB_STANDBY) || (_knob == KNOB_TEST);
}

bool Transponder::getReplyAnnunciator() const
{
    return _identMode || (_knob == KNOB_TEST);
}

