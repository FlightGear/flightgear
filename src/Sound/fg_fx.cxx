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

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/sound/xmlsound.hxx>

#include <Main/fg_props.hxx>

#include "fg_fx.hxx"


FGFX::FGFX () :
    last_pause( true ),
    last_volume( 0.0 ),
    _pause( fgGetNode("/sim/sound/pause") ),
    _volume( fgGetNode("/sim/sound/volume") )
{
}

FGFX::~FGFX ()
{
    unsigned int i;
    for ( i = 0; i < _sound.size(); i++ ) {
        delete _sound[i];
    }
    _sound.clear();

    while ( _samplequeue.size() > 0 ) {
        delete _samplequeue.front();
        _samplequeue.pop();
    }
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
                sound->init(globals->get_props(), node->getChild(i),
                            globals->get_soundmgr(), globals->get_fg_root());
  
                _sound.push_back(sound);
            } catch ( sg_io_exception &e ) {
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
FGFX::bind ()
{
}

void
FGFX::unbind ()
{
}

void
FGFX::update (double dt)
{
    SGSoundMgr *smgr = globals->get_soundmgr();

    if (smgr->is_working() == false) {
        return;
    }

    // command sound manger
    bool pause = _pause->getBoolValue();
    if ( pause != last_pause ) {
        if ( pause ) {
            smgr->pause();
        } else {
            smgr->resume();
        }
        last_pause = pause;
    }

    // process mesage queue
    const string msgid = "Sequential Audio Message";
    bool is_playing = false;
    if ( smgr->exists( msgid ) ) {
        if ( smgr->is_playing( msgid ) ) {
            // still playing, do nothing
            is_playing = true;
        } else {
            // current message finished, stop and remove
            smgr->stop( msgid );   // removes source
            smgr->remove( msgid ); // removes buffer
        }
    }
    if ( !is_playing ) {
        // message queue idle, add next sound if we have one
        if ( _samplequeue.size() > 0 ) {
            smgr->add( _samplequeue.front(), msgid );
            _samplequeue.pop();
            smgr->play_once( msgid );
        }
    }

    double volume = _volume->getDoubleValue();
    if ( volume != last_volume ) {
        smgr->set_volume( volume );        
        last_volume = volume;
    }

    if ( !pause ) {
        // update sound effects if not paused
        for ( unsigned int i = 0; i < _sound.size(); i++ ) {
            _sound[i]->update(dt);
        }
    }
}

/**
 * add a sound sample to the message queue which is played sequentially
 * in order.
 */
void
FGFX::play_message( SGSoundSample *_sample )
{
    _samplequeue.push( _sample );
}
void
FGFX::play_message( const string path, const string fname, double volume )
{
    if (globals->get_soundmgr()->is_working() == false) {
        return;
    }
    SGSoundSample *sample;
    sample = new SGSoundSample( path.c_str(), fname.c_str() );
    sample->set_volume( volume );
    play_message( sample );
}


// end of fg_fx.cxx
