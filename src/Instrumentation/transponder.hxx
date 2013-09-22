// transponder.hxx -- class to impliment a transponder
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

#ifndef TRANSPONDER_HXX
#define TRANSPONDER_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <Main/fg_props.hxx>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>

class Transponder : public SGSubsystem, public SGPropertyChangeListener
{
public:
    Transponder(SGPropertyNode *node);
    virtual ~Transponder();

    virtual void init ();
    virtual void update (double dt);
    virtual void bind();
    virtual void unbind();
    
private:
    enum Mode
    {
        MODE_A = 0,
        MODE_C,
        MODE_S
    };
    
    enum KnobPosition
    {
        KNOB_OFF = 0,
        KNOB_STANDBY,
        KNOB_TEST,
        KNOB_GROUND,
        KNOB_ON,
        KNOB_ALT
    };
    
    // annunciators, for KT-70 compatibility only
    // these should be replaced with conditionals in the instrument
    bool getFLAnnunciator() const;
    bool getAltAnnunciator() const;
    bool getGroundAnnuciator() const;
    bool getOnAnnunciator() const;
    bool getStandbyAnnunciator() const;
    bool getReplyAnnunciator() const;
    
    // Inputs
    SGPropertyNode_ptr _pressureAltitude_node;
    SGPropertyNode_ptr _busPower_node;
    SGPropertyNode_ptr _serviceable_node;

    SGPropertyNode_ptr _mode_node;
    SGPropertyNode_ptr _knob_node;
    SGPropertyNode_ptr _idCode_node;
    SGPropertyNode_ptr _digit_node[4];
    
    simgear::TiedPropertyList _tiedProperties;
    
    SGPropertyNode_ptr _identBtn_node;
    bool _identMode;
    bool _kt70Compat;
    
    // Outputs
    SGPropertyNode_ptr _altitude_node;
    SGPropertyNode_ptr _altitudeValid_node;
    SGPropertyNode_ptr _transmittedId_node;
    SGPropertyNode_ptr _ident_node;

    // Internal
    std::string _name;
    int _num;
    Mode _mode;
    KnobPosition _knob;
    double _identTime;
    int _listener_active;
    double _requiredBusVolts;
    std::string _altitudeSourcePath;
    
    void valueChanged (SGPropertyNode *);
    int setMinMax(int val);
    bool has_power() const;
};

#endif // TRANSPONDER_HXX
