// fg_fx.cxx -- Sound effect management class implementation
//
// Started by David Megginson, October 2001
// (Reuses some code from main.cxx, probably by Curtis Olson)
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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
// $Id$

#ifdef _MSC_VER
#pragma warning (disable: 4786)
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "fg_fx.hxx"

#include <Main/fg_props.hxx>

#include <simgear/props/props.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/sound/xmlsound.hxx>

FGFX::FGFX ( SGSoundMgr *smgr, const string &refname ) :
    _enabled( fgGetNode("/sim/sound/effects/enabled", true) ),
    _volume( fgGetNode("/sim/sound/effects/volume", true) ),
    _avionics_enabled( fgGetNode("/sim/sound/avionics/enabled", true) ),
    _avionics_volume( fgGetNode("/sim/sound/avionics/volume", true) ),
    _avionics_external( fgGetNode("/sim/sound/avionics/external-view", true) ),
    _internal( fgGetNode("/sim/current-view/internal", true) )
{
    SGSampleGroup::_smgr = smgr;
    SGSampleGroup::_refname = refname;
    SGSampleGroup::_smgr->add(this, refname);
    _avionics = _smgr->find("avionics", true);
    _avionics->tie_to_listener();
}


FGFX::~FGFX ()
{
    for (unsigned int i = 0; i < _sound.size(); i++ ) {
        delete _sound[i];
    }
    _sound.clear();
}


void
FGFX::init()
{
    SGPropertyNode *node = fgGetNode("/sim/sound", true);

    string path_str = node->getStringValue("path");
    if (path_str.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "No path in /sim/sound/path");
        return;
    }
    
    SGPath path = globals->resolve_aircraft_path(path_str);
    SG_LOG(SG_GENERAL, SG_INFO, "Reading sound " << node->getName()
           << " from " << path.str());

    SGPropertyNode root;
    try {
        readProperties(path.str(), &root);
    } catch (const sg_exception &) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "Error reading file '" << path.str() << '\'');
        return;
    }

    node = root.getNode("fx");
    if(node) {
        for (int i = 0; i < node->nChildren(); ++i) {
            SGXmlSound *sound = new SGXmlSound();
  
            try {
                sound->init(globals->get_props(), node->getChild(i), this,
                            _avionics, globals->get_fg_root());
  
                _sound.push_back(sound);
            } catch ( sg_exception &e ) {
                SG_LOG(SG_GENERAL, SG_ALERT, e.getFormattedMessage());
                delete sound;
            }
        }
    }
}


void
FGFX::reinit()
{
    _sound.clear();
    init();
};


void
FGFX::update (double dt)
{
    bool active = _avionics_external->getBoolValue() ||
                  _internal->getBoolValue();

    if ( active && _avionics_enabled->getBoolValue() )
        _avionics->resume(); // no-op if already in resumed state
    else
        _avionics->suspend();
    _avionics->set_volume( _avionics_volume->getFloatValue() );

    if ( _enabled->getBoolValue() ) {
        set_volume( _volume->getDoubleValue() );
        resume();

        // update sound effects if not paused
        for ( unsigned int i = 0; i < _sound.size(); i++ ) {
            _sound[i]->update(dt);
        }

        SGSampleGroup::update(dt);
    }
    else
        suspend();
}

// end of fg_fx.cxx
