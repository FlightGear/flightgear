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

#include <simgear/timing/timestamp.hxx>

#include <Main/fgfs.hxx>

SG_USING_STD(map);
SG_USING_STD(string);


// manages everything we need to know for an individual sound sample
class FGSimpleSound {

private:

    slSample *sample;
    slEnvelope *pitch_envelope;
    slEnvelope *volume_envelope;
    double pitch;
    double volume;

public:

    FGSimpleSound( string file );
    FGSimpleSound( unsigned char *buffer, int len );
    ~FGSimpleSound();

    void play( slScheduler *sched, bool looped );
    void stop( slScheduler *sched, bool quick = true );

    inline void play_once( slScheduler *sched ) { play( sched, false); }
    inline void play_looped( slScheduler *sched ) { play( sched, true); }
    inline bool is_playing( ) {
        return ( sample->getPlayCount() > 0 );
    }

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


typedef struct {
	int n;
	slSample *sample;
} sample_ref;

typedef map < string, sample_ref * > sample_map;
typedef sample_map::iterator sample_map_iterator;
typedef sample_map::const_iterator const_sample_map_iterator;


typedef map < string, FGSimpleSound * > sound_map;
typedef sound_map::iterator sound_map_iterator;
typedef sound_map::const_iterator const_sound_map_iterator;


class FGSoundMgr : public FGSubsystem
{

    slScheduler *audio_sched;
    smMixer *audio_mixer;

    sound_map sounds;
    sample_map samples;

    SGTimeStamp last;
    double safety;

public:

    FGSoundMgr();
    ~FGSoundMgr();


    /**
     * Initialize the sound manager.
     */
    void init();


    /**
     * Bind properties for the sound manager.
     */
    void bind ();


    /**
     * Unbind properties for the sound manager.
     */
    void unbind ();


    /**
     * Run the audio scheduler.
     */
    void update(int dt);


    /**
     * Pause all sounds.
     */
    void pause ();


    /**
     * Resume all sounds.
     */
    void resume ();


    // is audio working?
    inline bool is_working() const { return !audio_sched->notWorking(); }

    // add a sound effect, return true if successful
    bool add( FGSimpleSound *sound, const string& refname);

    // add a sound file, return the sample if successful, else return NULL
    FGSimpleSound *add( const string& refname, const string& file = "" );

    // remove a sound effect, return true if successful
    bool remove( const string& refname );

    // return true of the specified sound exists in the sound manager system
    bool exists( const string& refname );

    // return a pointer to the FGSimpleSound if the specified sound
    // exists in the sound manager system, otherwise return NULL
    FGSimpleSound *find( const string& refname );

    // tell the scheduler to play the indexed sample in a continuous
    // loop
    bool play_looped( const string& refname );

    // tell the scheduler to play the indexed sample once
    bool play_once( const string& refname );

    // return true of the specified sound is currently being played
    bool is_playing( const string& refname );

    // immediate stop playing the sound
    bool stop( const string& refname );

    // return the audio scheduler 
    inline slScheduler *get_scheduler( ) { return audio_sched; };
};


#endif // _SOUNDMGR_HXX


