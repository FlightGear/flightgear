// soundmgr.cxx -- Sound effect management class
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


#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>

#include <Main/globals.hxx>

#include "soundmgr.hxx"


// constructor
FGSimpleSound::FGSimpleSound( string file ) {
    FGPath slfile( globals->get_fg_root() );
    slfile.append( file );
    sample = new slSample ( (char *)slfile.c_str() );
    pitch_envelope = new slEnvelope( 1, SL_SAMPLE_ONE_SHOT );
    volume_envelope = new slEnvelope( 1, SL_SAMPLE_ONE_SHOT );
    pitch_envelope->setStep ( 0, 0.01, 1.0 );
    volume_envelope->setStep ( 0, 0.01, 1.0 );
}

FGSimpleSound::FGSimpleSound( unsigned char *buffer, int len ) {
    sample = new slSample ( buffer, len );
    pitch_envelope = new slEnvelope( 1, SL_SAMPLE_ONE_SHOT );
    volume_envelope = new slEnvelope( 1, SL_SAMPLE_ONE_SHOT );
    pitch_envelope->setStep ( 0, 0.01, 1.0 );
    volume_envelope->setStep ( 0, 0.01, 1.0 );
}

// destructor
FGSimpleSound::~FGSimpleSound() {
    delete pitch_envelope;
    delete volume_envelope;
    delete sample;
}


// constructor
FGSoundMgr::FGSoundMgr() {
    audio_sched = new slScheduler( 8000 );
    audio_mixer = new smMixer;

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Rate = " << audio_sched->getRate()
	    << "  Bps = " << audio_sched->getBps()
	    << "  Stereo = " << audio_sched->getStereo() );
}

// destructor
FGSoundMgr::~FGSoundMgr() {
    sound_map_iterator current = sounds.begin();
    sound_map_iterator end = sounds.end();
    for ( ; current != end; ++current ) {
	FGSimpleSound *s = current->second;
	delete s->get_sample();
	delete s;
    }

    delete audio_sched;
    delete audio_mixer;
}


// initialize the sound manager
bool FGSoundMgr::init() {
    audio_mixer -> setMasterVolume ( 80 ) ;  /* 80% of max volume. */
    audio_sched -> setSafetyMargin ( 1.0 ) ;

    sound_map_iterator current = sounds.begin();
    sound_map_iterator end = sounds.end();
    for ( ; current != end; ++current ) {
	FGSimpleSound *s = current->second;
	delete s->get_sample();
	delete s;
    }
    sounds.clear();

    if ( audio_sched->not_working() ) {
	return false;
    } else {
	return true;
    }
}


// run the audio scheduler
bool FGSoundMgr::update() {
    if ( !audio_sched->not_working() ) {
	audio_sched -> update();
	return true;
    } else {
	return false;
    }
}


// add a sound effect, return true if successful
bool FGSoundMgr::add( FGSimpleSound *sound, const string& refname  ) {
    sounds[refname] = sound;

    return true;
}


// remove a sound effect, return true if successful
bool FGSoundMgr::remove( const string& refname ) {

#if defined ( PLIB_AUDIO_IS_BROKEN )
    // if PLIB_AUDIO_IS_BROKEN, we can't reliably remove sounds that
    // are currently being played. :-( So, let's just not remove them
    // and return false.  The effects of this are that the sound
    // sample will continue to finish playing (or continue to loop
    // forever.)  And the sound sample will remain registered in the
    // plib audio system.  This is a memory leak, and eventually this
    // could cause us to max out the total number of allowed sound
    // samples in plib, but what are you going to do?  Hopefully the
    // plib team will do a new stable relase with these problems
    // fixed.

    cout << "plib broken audio, skipping remove" << endl;

    return false;
#endif

    sound_map_iterator it = sounds.find( refname );
    if ( it != sounds.end() ) {
	// first stop the sound from playing (so we don't bomb the
	// audio scheduler)
	FGSimpleSound *sample = it->second;

	cout << "Playing "
	     << sample->get_sample()->getPlayCount() << " instances!" << endl;

	audio_sched->stopSample( sample->get_sample() );
	audio_sched->addSampleEnvelope( sample->get_sample(), 0, 0, 
					NULL,
					SL_PITCH_ENVELOPE );
	audio_sched->addSampleEnvelope( sample->get_sample(), 0, 1, 
					NULL,
					SL_VOLUME_ENVELOPE );

#if defined ( PLIB_AUDIO_IS_BROKEN )
	// if PLIB_AUDIO_IS_BROKEN, we can't reliably remove sounds
	// that are currently being played. :-( So, let's just not
	// remove them and return false.  The effects of this are that
	// the sound sample will continue to finish playing (or
	// continue to loop forever.)  And the sound sample will
	// remain registered in the plib audio system.  This is a
	// memory leak, and eventually this could cause us to max out
	// the total number of allowed sound samples in plib, but what
	// are you going to do?  Hopefully the plib team will do a new
	// stable relase with these problems fixed.

	cout << "plib broken audio, skipping actual remove" << endl;

	return false;
#else
	// must call audio_sched->update() after stopping the sound
	// but before deleting it.
	audio_sched -> update();
	cout << "Still playing "
	     << sample->get_sample()->getPlayCount() << " instances!" << endl;

	delete sample;
        sounds.erase( it );

	return true;
#endif
   } else {
	return false;
    }
}


// return true of the specified sound exists in the sound manager system
bool FGSoundMgr::exists( const string& refname ) {
    sound_map_iterator it = sounds.find( refname );
    if ( it != sounds.end() ) {
	return true;
   } else {
	return false;
    }
}


// tell the scheduler to play the indexed sample in a continuous
// loop
bool FGSoundMgr::play_looped( const string& refname ) {
    sound_map_iterator it = sounds.find( refname );
    if ( it != sounds.end() ) {
	FGSimpleSound *sample = it->second;
	audio_sched->loopSample( sample->get_sample() );
	audio_sched->addSampleEnvelope( sample->get_sample(), 0, 0, 
					sample->get_pitch_envelope(),
					SL_PITCH_ENVELOPE );
	audio_sched->addSampleEnvelope( sample->get_sample(), 0, 1, 
					sample->get_volume_envelope(),
					SL_VOLUME_ENVELOPE );
	
	return true;
    } else {
	return false;
    }
}


// tell the scheduler to play the indexed sample once
bool FGSoundMgr::FGSoundMgr::play_once( const string& refname ) {
    sound_map_iterator it = sounds.find( refname );
    if ( it != sounds.end() ) {
	FGSimpleSound *sample = it->second;
	audio_sched->stopSample( sample->get_sample() );
	audio_sched->playSample( sample->get_sample() );
	audio_sched->addSampleEnvelope( sample->get_sample(), 0, 0, 
					sample->get_pitch_envelope(),
					SL_PITCH_ENVELOPE );
	audio_sched->addSampleEnvelope( sample->get_sample(), 0, 1, 
					sample->get_volume_envelope(),
					SL_VOLUME_ENVELOPE );
	
	return true;
    } else {
	return false;
    }
}


// return true of the specified sound is currently being played
bool FGSoundMgr::is_playing( const string& refname ) {
    sound_map_iterator it = sounds.find( refname );
    if ( it != sounds.end() ) {
	FGSimpleSound *sample = it->second;
	return (sample->get_sample()->getPlayCount() > 0 );
	return true;
    } else {
	return false;
    }
}


// immediate stop playing the sound
bool FGSoundMgr::stop( const string& refname ) {
    sound_map_iterator it = sounds.find( refname );
    if ( it != sounds.end() ) {
	FGSimpleSound *sample = it->second;
	audio_sched->stopSample( sample->get_sample() );
	return true;
    } else {
	return false;
    }
}
