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
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>

#include "soundmgr.hxx"

#define FG_SOUND_SAFETY_MULT 3
#define FG_MAX_SOUND_SAFETY ( 1.0 / FG_SOUND_SAFETY_MULT )

//
// SimpleSound
//

// constructor
FGSimpleSound::FGSimpleSound( string file )
  : pitch(1.0),
    volume(1.0),
    requests(0)
{
    SGPath slfile( globals->get_fg_root() );
    slfile.append( file );
    sample = new slSample ( (char *)slfile.c_str() );
    pitch_envelope = new slEnvelope( 1, SL_SAMPLE_ONE_SHOT );
    volume_envelope = new slEnvelope( 1, SL_SAMPLE_ONE_SHOT );
    pitch_envelope->setStep ( 0, 0.01, 1.0 );
    volume_envelope->setStep ( 0, 0.01, 1.0 );
}

FGSimpleSound::FGSimpleSound( unsigned char *buffer, int len )
  : pitch(1.0),
    volume(1.0),
    requests(0)
{
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

void FGSimpleSound::play( slScheduler *sched, bool looped ) {
    
    requests++;

    // make sure sound isn't already playing
    if ( sample->getPlayCount() > 0 ) {
        return;
    }

    // sched->stopSample(sample);
    if ( looped ) {
        sched->loopSample(sample);
    } else {
        sched->playSample(sample);
    }
    
    sched->addSampleEnvelope(sample, 0, 0, pitch_envelope, SL_PITCH_ENVELOPE);
    sched->addSampleEnvelope(sample, 0, 1, volume_envelope, SL_VOLUME_ENVELOPE);
}

void FGSimpleSound::stop( slScheduler *sched, bool quick ) {

    if ( quick ) {
        requests = 0;
    } else {
        if ( --requests < 0 ) {
            requests = 0;
        }
    }

    if ( requests > 0 ) {
       return;
    }

    sched->stopSample( sample );
}

//
// Sound Manager
//

// constructor
FGSoundMgr::FGSoundMgr() {
    audio_sched = new slScheduler( 8000 );
    if ( audio_sched->notWorking() ) {
	SG_LOG( SG_GENERAL, SG_ALERT, "Audio initialization failed!" );
    } else {
	audio_sched -> setMaxConcurrent ( 6 ); 

	audio_mixer = new smMixer;

	SG_LOG( SG_GENERAL, SG_INFO,
		"Rate = " << audio_sched->getRate()
		<< "  Bps = " << audio_sched->getBps()
		<< "  Stereo = " << audio_sched->getStereo() );
    }
}

// destructor

FGSoundMgr::~FGSoundMgr() {

    //
    // Remove the samples from the sample manager.
    //
    sample_map_iterator sample_current = samples.begin();
    sample_map_iterator sample_end = samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
	sample_ref *sr = sample_current->second;
	delete sr->sample;
	delete sr;
    }

    //
    // Remove the sounds from the sound manager.
    //
    sound_map_iterator sound_current = sounds.begin();
    sound_map_iterator sound_end = sounds.end();
    for ( ; sound_current != sound_end; ++sound_current ) {
        FGSimpleSound *s = sound_current->second;
        delete s;
    }

    delete audio_sched;
    delete audio_mixer;
}


// initialize the sound manager
void FGSoundMgr::init() {
    last.stamp();
    safety = FG_MAX_SOUND_SAFETY;

    // audio_mixer -> setMasterVolume ( 80 ) ;  /* 80% of max volume. */
    audio_sched -> setSafetyMargin ( FG_SOUND_SAFETY_MULT * safety ) ;


    //
    // Remove the samples from the sample manager.
    //
    sample_map_iterator sample_current = samples.begin();
    sample_map_iterator sample_end = samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        sample_ref *sr = sample_current->second;
        delete sr->sample;
        delete sr;
    }
    samples.clear();
 
    //
    // Remove the sounds from the sound manager.
    //
    sound_map_iterator sound_current = sounds.begin();
    sound_map_iterator sound_end = sounds.end();
    for ( ; sound_current != sound_end; ++sound_current ) {
	FGSimpleSound *s = sound_current->second;
	delete s->get_sample();
	delete s;
    }
    sounds.clear();

}

void FGSoundMgr::bind ()
{
  // no properties yet
}

void FGSoundMgr::unbind ()
{
  // no properties yet
}


// run the audio scheduler
void FGSoundMgr::update(int dt) {
    SGTimeStamp current;
    current.stamp();

    double elapsed = (double)(current - last) / 1000000.0;
    last = current;

    if ( elapsed > safety ) {
	safety = elapsed;
    } else {
	safety = safety * 0.99 + elapsed * 0.01;
    }
    if ( safety > FG_MAX_SOUND_SAFETY ) {
	safety = FG_MAX_SOUND_SAFETY;
    }
    // cout << "safety = " << safety << endl;
    audio_sched -> setSafetyMargin ( FG_SOUND_SAFETY_MULT * safety ) ;

    if ( !audio_sched->not_working() )
	audio_sched -> update();
}


void
FGSoundMgr::pause ()
{
  audio_sched->pauseSample(0, 0);
}


void
FGSoundMgr::resume ()
{
  audio_sched->resumeSample(0, 0);
}


// add a sound effect, return true if successful
bool FGSoundMgr::add( FGSimpleSound *sound, const string& refname ) {

    sound_map_iterator sound_it = sounds.find( refname );
    if ( sound_it != sounds.end() ) {
        // sound already exists
        return false;
    }

    sample_map_iterator sample_it = samples.find( refname );
    if ( sample_it != samples.end() ) {
        // this sound has existed in the past and it's sample is still
        // here, delete the sample so we can replace it.
        samples.erase( sample_it );
    }

    sample_ref *sr = new sample_ref;

    sr->n=1;
    sr->sample = sound->get_sample();
    samples[refname] = sr;

    sounds[refname] = sound;

    return true;
}


// add a sound from a file, return the sample if successful, else return NULL
FGSimpleSound *FGSoundMgr::add( const string& refname, const string &file ) {
     FGSimpleSound *sound;

    if (file.empty())
       return NULL;

    sample_map_iterator it = samples.find(file);
    if (it == samples.end()) {
       sound = new FGSimpleSound(file);
       sounds[refname] = sound;

       sample_ref *sr = new sample_ref;

       sr->n=1;
       sr->sample = sound->get_sample();
       samples[file] = sr;

    } else {
       sample_ref *sr = it->second;

       sr->n++;
       sound =
           new FGSimpleSound(sr->sample->getBuffer(), sr->sample->getLength());
       sounds[refname] = sound;

    }

    return sound;
}


// remove a sound effect, return true if successful
bool FGSoundMgr::remove( const string& refname ) {

    sound_map_iterator it = sounds.find( refname );
    if ( it != sounds.end() ) {
	// first stop the sound from playing (so we don't bomb the
	// audio scheduler)
	FGSimpleSound *sample = it->second;

	// cout << "Playing " << sample->get_sample()->getPlayCount()
	//      << " instances!" << endl;

	audio_sched->stopSample( sample->get_sample() );
	audio_sched->addSampleEnvelope( sample->get_sample(), 0, 0, 
					NULL,
					SL_PITCH_ENVELOPE );
	audio_sched->addSampleEnvelope( sample->get_sample(), 0, 1, 
					NULL,
					SL_VOLUME_ENVELOPE );

	// must call audio_sched->update() after stopping the sound
	// but before deleting it.
	audio_sched -> update();
	// cout << "Still playing " << sample->get_sample()->getPlayCount()
	//      << " instances!" << endl;

        //
        // FIXME:
        // Due to the change in the sound manager, samples live
	// until the sound manager gets removed.
        //
	// delete sample;
        sounds.erase( it );

        // cout << "sndmgr: removed -> " << refname << endl;
	return true;
    } else {
        // cout << "sndmgr: failed remove -> " << refname << endl;
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


// return a pointer to the FGSimpleSound if the specified sound exists
// in the sound manager system, otherwise return NULL
FGSimpleSound *FGSoundMgr::find( const string& refname ) {
    sound_map_iterator it = sounds.find( refname );
    if ( it != sounds.end() ) {
	return it->second;
    } else {
	return NULL;
    }
}


// tell the scheduler to play the indexed sample in a continuous
// loop
bool FGSoundMgr::play_looped( const string& refname ) {
   FGSimpleSound *sample;

    if ((sample = find( refname )) == NULL)
       return false;

    sample->play(audio_sched, true);
    return true;
}


// tell the scheduler to play the indexed sample once
bool FGSoundMgr::play_once( const string& refname ) {
    FGSimpleSound *sample;

    if ((sample = find( refname )) == NULL)
       return false;

    sample->play(audio_sched, false);
    return true;
}


// return true of the specified sound is currently being played
bool FGSoundMgr::is_playing( const string& refname ) {
    FGSimpleSound *sample;

    if ((sample = find( refname )) == NULL)
       return false;

    return (sample->get_sample()->getPlayCount() > 0 );
}


// immediate stop playing the sound
bool FGSoundMgr::stop( const string& refname ) {
    FGSimpleSound *sample;

    if ((sample = find( refname )) == NULL)
       return false;

    audio_sched->stopSample( sample->get_sample() );
    return true;
}
