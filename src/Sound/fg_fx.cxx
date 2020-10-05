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

#include <algorithm>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Sound/soundmanager.hxx>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sound/xmlsound.hxx>

FGFX::FGFX ( const std::string &refname, SGPropertyNode *props ) :
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

    _atc_enabled = _props->getNode("sim/sound/atc/enabled", true);
    _atc_volume = _props->getNode("sim/sound/atc/volume", true);
    _atc_ext = _props->getNode("sim/sound/atc/external-view", true);

    _smgr = globals->get_subsystem<FGSoundManager>();
    if (!_smgr) {
      return;
    }
    _active = _smgr->is_active();
  
    _refname = refname;
    _smgr->add(this, refname);

    if (!_is_aimodel) // only for the main aircraft
    {
        _avionics = _smgr->find("avionics", true);
        _avionics->tie_to_listener();
        
        _atc = _smgr->find("atc", true);
        _atc->tie_to_listener();
    }
}

void FGFX::unbind()
{
    if (_smgr)
    {
        _smgr->remove(_refname);
    }

    // because SGXmlSound has an owning ref back to us, we need to
    // clear these here, or we will never get destroyed
    std::for_each(_sound.begin(), _sound.end(), [](const SGXmlSound* snd) { delete snd; });
    _sound.clear();
}

FGFX::~FGFX ()
{
}

void
FGFX::init()
{
    if (!_smgr) {
        return;
    }
  
    SGPropertyNode *node = _props->getNode("sim/sound", true);

    std::string path_str = node->getStringValue("path");
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
           << " from " << path);

    SGPropertyNode root;
    try {
        readProperties(path, &root);
    } catch (const sg_exception &) {
        SG_LOG(SG_SOUND, SG_ALERT,
               "Error reading file '" << path << '\'');
        return;
    }

    node = root.getNode("fx");
    if(node) {
        for (int i = 0; i < node->nChildren(); ++i) {
            std::unique_ptr<SGXmlSound> soundfx{new SGXmlSound};
  
            try {
                bool ok = soundfx->init( _props, node->getChild(i), this, _avionics,
                               path.dir() );
                if (ok) {
                    // take the pointer out of the unique ptr so it's not deleted
                    _sound.push_back( soundfx.release() );
                }
            } catch ( sg_exception &e ) {
                SG_LOG(SG_SOUND, SG_ALERT, e.getFormattedMessage());
            }
        }
    }
}


void
FGFX::reinit()
{
    std::for_each(_sound.begin(), _sound.end(), [](const SGXmlSound* snd) { delete snd; });
    _sound.clear();
    init();
}


void
FGFX::update (double dt)
{
    if (!_smgr) {
        return;
    }

    if (!_active && _smgr->is_active())
    {
        _active = true;
        for ( unsigned int i = 0; i < _sound.size(); i++ ) {
            _sound[i]->start();
        }
    }

      
    if ( _enabled->getBoolValue() ) {
        if ( _avionics)
        {
            const bool e = _avionics_enabled->getBoolValue();
            if (e && (_avionics_ext->getBoolValue() || _internal->getBoolValue())) {
                // avionics sound is enabled
                _avionics->resume(); // no-op if already in resumed state
                _avionics->set_volume( _avionics_volume->getFloatValue() );
            }
            else
                _avionics->suspend();
        }
        
        if ( _atc)
        {
            const bool e = _atc_enabled->getBoolValue();
            if (e && (_atc_ext->getBoolValue() || _internal->getBoolValue())) {
                // ATC sound is enabled
                _atc->resume(); // no-op if already in resumed state
                _atc->set_volume( _atc_volume->getFloatValue() );
            }
            else
                _atc->suspend();
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
