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
    last_pause( false ),
    last_volume( 0.0 ),
    _pause( fgGetNode("/sim/sound/pause") ),
    _volume( fgGetNode("/sim/sound/volume") )
{
    SGSampleGroup::_smgr = smgr;
    SGSampleGroup::_refname = refname;
    SGSampleGroup::_smgr->add(this, refname);
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
    SGPath path( globals->get_fg_root() );
    if (path_str.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "No path in /sim/sound/path");
        return;
    }

    path.append(path_str.c_str());
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
                            globals->get_fg_root());
  
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
    bool new_pause = _pause->getBoolValue();
    if ( new_pause != last_pause ) {
        if ( new_pause ) {
            suspend();
        } else {
            resume();
        }
        last_pause = new_pause;
    }

    if ( !new_pause ) {
        double volume = _volume->getDoubleValue();
        if ( volume != last_volume ) {
            set_volume( volume );        
            last_volume = volume;
        }

        // update sound effects if not paused
        for ( unsigned int i = 0; i < _sound.size(); i++ ) {
            _sound[i]->update(dt);
        }

        SGSampleGroup::update(dt);
    }
}

// end of fg_fx.cxx
