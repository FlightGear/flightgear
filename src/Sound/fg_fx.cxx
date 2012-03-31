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
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/sound/xmlsound.hxx>

FGFX::FGFX ( SGSoundMgr *smgr, const string &refname, SGPropertyNode *props ) :
    _props( props )
{
    if (!props) {
        _is_aimodel = false;
        _props = globals->get_props();
        _enabled = fgGetNode("/sim/sound/effects/enabled", true);
        _volume = fgGetNode("/sim/sound/effects/volume", true);
    } else {
        _is_aimodel = true;
        _enabled = _props->getNode("/sim/sound/aimodels/enabled", true);
        _enabled->setBoolValue(fgGetBool("/sim/sound/effects/enabled"));
         _volume = _props->getNode("/sim/sound/aimodels/volume", true);
        _volume->setFloatValue(fgGetFloat("/sim/sound/effects/volume"));
    }

    _avionics_enabled = _props->getNode("sim/sound/avionics/enabled", true);
    _avionics_volume = _props->getNode("sim/sound/avionics/volume", true);
    _avionics_ext = _props->getNode("sim/sound/avionics/external-view", true);
    _internal = _props->getNode("sim/current-view/internal", true);

    SGSampleGroup::_smgr = smgr;
    SGSampleGroup::_refname = refname;
    SGSampleGroup::_smgr->add(this, refname);

    if (!_is_aimodel)
    {
        _avionics = _smgr->find("avionics", true);
        _avionics->tie_to_listener();
    }
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
    SGPropertyNode *node = _props->getNode("sim/sound", true);

    string path_str = node->getStringValue("path");
    if (path_str.empty()) {
        SG_LOG(SG_SOUND, SG_ALERT, "No path in sim/sound/path");
        return;
    }
    
    SGPath path = globals->resolve_aircraft_path(path_str);
    if (path.isNull())
    {
        SG_LOG(SG_SOUND, SG_ALERT,
               "File not found: '" << path_str);
        return;
    }
    SG_LOG(SG_SOUND, SG_INFO, "Reading sound " << node->getName()
           << " from " << path.str());

    SGPropertyNode root;
    try {
        readProperties(path.str(), &root);
    } catch (const sg_exception &) {
        SG_LOG(SG_SOUND, SG_ALERT,
               "Error reading file '" << path.str() << '\'');
        return;
    }

    node = root.getNode("fx");
    if(node) {
        for (int i = 0; i < node->nChildren(); ++i) {
            SGXmlSound *soundfx = new SGXmlSound();
  
            try {
                soundfx->init( _props, node->getChild(i), this, _avionics,
                               path.dir() );
                _sound.push_back( soundfx );
            } catch ( sg_exception &e ) {
                SG_LOG(SG_SOUND, SG_ALERT, e.getFormattedMessage());
                delete soundfx;
            }
        }
    }
}


void
FGFX::reinit()
{
    for ( unsigned int i = 0; i < _sound.size(); i++ ) {
        delete _sound[i];
    }
    _sound.clear();
    init();
};


void
FGFX::update (double dt)
{
    if ( _enabled->getBoolValue() ) {
        if ( _avionics_enabled->getBoolValue())
        {
            if (_avionics_ext->getBoolValue() || _internal->getBoolValue()) {
                // avionics sound is enabled
                _avionics->resume(); // no-op if already in resumed state
                _avionics->set_volume( _avionics_volume->getFloatValue() );
            }
            else
                _avionics->suspend();
        }

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
