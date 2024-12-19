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
#include <Main/globals.hxx>
#include <Main/sentryIntegration.hxx>
#include <Sound/soundmanager.hxx>
#include <algorithm>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
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

    _machwave_active = _props->getNode("sim/sound/machwave/active", true);
    _machwave_volume = _props->getNode("sim/sound/machwave/offset-m", true);

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

void FGFX::shutdown()
{
    _smgr->remove(_refname);
    _xmlSounds.clear();
}

FGFX::~FGFX ()
{
    // check that shutdown has been called
    assert(!_smgr->exists(_refname));
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
        simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::AudioFX,
                               "Failed to find FX XML file:" + path_str, sg_location{path_str});
        SG_LOG(SG_SOUND, SG_ALERT,
               "File not found: '" << path_str);
        return;
    }
    SG_LOG(SG_SOUND, SG_INFO, "Reading sound " << node->getNameString()
           << " from " << path);

    SGPropertyNode root;
    try {
        readProperties(path, &root);
    } catch (const sg_exception& e) {
        simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::AudioFX,
                               "Failure loading FX XML:" + e.getFormattedMessage(), e.getLocation());
        return;
    }

    node = root.getNode("fx");
    if(node) {
        for (int i = 0; i < node->nChildren(); ++i) {
            SGXmlSoundRef soundfx{new SGXmlSound};

            try {
                bool ok = soundfx->init( _props, node->getChild(i), this, _avionics,
                               path.dir() );
                if (ok) {
                    _xmlSounds.push_back(soundfx);
                }
            } catch ( sg_exception &e ) {
                SG_LOG(SG_SOUND, SG_ALERT, e.getFormattedMessage());
                simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::AudioFX,
                                       "Failure creating Audio FX:" + e.getFormattedMessage(), path);
            }
        }
    }
}

void
FGFX::update (double dt)
{
    // FGFX::update is called via the SGSoundManager update(), which
    // calls update on all its sample-groups.
    if (!_smgr) {
        return;
    }

    if (!_active && _smgr->is_active())
    {
        _active = true;
        for (auto s : _xmlSounds) {
            s->start();
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

        _machwave_active->setBoolValue(_mInCone);
        _machwave_volume->setFloatValue(_mOffset_m);

        set_volume( _volume->getDoubleValue() );
        resume();

        // update sound effects if not paused
        for (auto xs : _xmlSounds) {
            xs->update(dt);
        }

        SGSampleGroup::update(dt);
    }
    else
        suspend();
}

// end of fg_fx.cxx
