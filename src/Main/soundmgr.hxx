// soundmgr.hxx -- Sound effect management class
//
// Sound manager initially written by David Findlay
// <david_j_findlay@yahoo.com.au> 2001
//
// C++-ified by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _SOUNDMGR_HXX
#define _SOUNDMGR_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_STRING
#include <map>

#include <plib/sl.h>
#include <plib/sm.h>

FG_USING_STD(map);
FG_USING_STD(string);


// manages everything we need to know for an individual sound sample
class FGSimpleSound {

    slSample *sample;
    slEnvelope *pitch_envelope;
    slEnvelope *volume_envelope;
    double pitch;
    double volume;

public:

    FGSimpleSound( string file );
    ~FGSimpleSound();

    inline double get_pitch() const { return pitch; }
    inline void set_pitch( double p ) {
	pitch = p;
	pitch_envelope->setStep( 0, 0.01, pitch );
    }
    inline double get_volume() const { return volume; }
    inline void set_volume( double v ) {
	volume = v;
	volume_envelope->setStep( 0, 0.01, volume );
    }

    inline slSample *get_sample() { return sample; }
    inline slEnvelope *get_pitch_envelope() { return pitch_envelope; }
    inline slEnvelope *get_volume_envelope() { return volume_envelope; }
};


typedef map < string, FGSimpleSound * > sound_map;
typedef sound_map::iterator sound_map_iterator;
typedef sound_map::const_iterator const_sound_map_iterator;


class FGSoundMgr {

    slScheduler *audio_sched;
    smMixer *audio_mixer;
    sound_map sounds;

public:

    FGSoundMgr();
    ~FGSoundMgr();

    // initialize the sound manager
    bool init();

    // run the audio scheduler
    bool update();

    // is audio working?
    inline bool is_working() const { return !audio_sched->not_working(); }

    // add a sound effect, return the index of the sound
    bool add( FGSimpleSound *sound, const string& refname );

    // tell the scheduler to play the indexed sample in a continuous
    // loop
    bool play_looped( const string& refname );

    // tell the scheduler to play the indexed sample once
    bool play_once( const string& refname );
};


#endif // _SOUNDMGR_HXX


