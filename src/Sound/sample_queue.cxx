// _samplequeue.cxx -- Sound effect management class implementation
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

#include "sample_queue.hxx"

#include <Main/fg_props.hxx>

#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/sound/sample_openal.hxx>

FGSampleQueue::FGSampleQueue ( SGSoundMgr *smgr, const string &refname ) :
    last_enabled( true ),
    last_volume( 0.0 ),
    _enabled( fgGetNode("/sim/sound/chatter/enabled", true) ),
    _volume( fgGetNode("/sim/sound/chatter/volume", true) )
{
    SGSampleGroup::_smgr = smgr;
    SGSampleGroup::_smgr->add(this, refname);
    SGSampleGroup::_refname = refname;
}


FGSampleQueue::~FGSampleQueue ()
{
    while ( _messages.size() > 0 ) {
        delete _messages.front();
        _messages.pop();
    }
}


void
FGSampleQueue::update (double dt)
{
    // command sound manger
    bool new_enabled = _enabled->getBoolValue();
    if ( new_enabled != last_enabled ) {
        if ( new_enabled ) {
            resume();
        } else {
            suspend();
        }
        last_enabled = new_enabled;
    }

    if ( new_enabled ) {
        double volume = _volume->getDoubleValue();
        if ( volume != last_volume ) {
            set_volume( volume );
            last_volume = volume;
        }

        // process message queue
        const string msgid = "Sequential Audio Message";
        bool now_playing = false;
        if ( exists( msgid ) ) {
            now_playing = is_playing( msgid );
            if ( !now_playing ) {
                // current message finished, stop and remove
                stop( msgid );   // removes source
                remove( msgid ); // removes buffer
            }
        }

        if ( !now_playing ) {
            // message queue idle, add next sound if we have one
            if ( _messages.size() > 0 ) {
                SGSampleGroup::add( _messages.front(), msgid );
                _messages.pop();
                play_once( msgid );
            }
        }

        SGSampleGroup::update(dt);
    }
}

// end of _samplequeue.cxx
