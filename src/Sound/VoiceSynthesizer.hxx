/*
 * VoiceSynthesizer.hxx - wraps flite+hts_engine
 * Copyright (C) 2014  Torsten Dreyer - torsten (at) t3r (dot) de
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef VOICESYNTHESIZER_HXX_
#define VOICESYNTHESIZER_HXX_

#include <simgear/sound/sample.hxx>
#include <simgear/threads/SGQueue.hxx>

#include <string>
struct _Flite_HTS_Engine;

/**
 * A Voice Synthesizer Interface
 */
class VoiceSynthesizer {
public:
  virtual ~VoiceSynthesizer() {};
  virtual SGSoundSample * synthesize( const std::string & text, double volume, double speed, double pitch ) = 0;
};

class SoundSampleReadyListener {
public:
  virtual ~SoundSampleReadyListener() {}
  virtual void SoundSampleReady( SGSharedPtr<SGSoundSample> ) = 0;
};

struct SynthesizeRequest {
  SynthesizeRequest() {
    speed = 0.5;
    volume = 1.0;
    pitch = 0.5;
    listener = NULL;
  }
  SynthesizeRequest( const SynthesizeRequest & other ) {
    text = other.text;
    speed = other.speed;
    volume = other.volume;
    pitch = other.pitch;
    listener = other.listener;
  }

  SynthesizeRequest & operator = ( const SynthesizeRequest & other ) {
    text = other.text;
    speed = other.speed;
    volume = other.volume;
    pitch = other.pitch;
    listener = other.listener;
    return *this;
  }

  // return a special marker request used to indicate the synthesis thread
  // should be exited.
  static SynthesizeRequest cancelThreadRequest();

  std::string text;
  double speed;
  double volume;
  double pitch;
  SoundSampleReadyListener * listener;
};

/**
 * A Voice Synthesizer using FLITE+HTS
 */
class FLITEVoiceSynthesizer : public VoiceSynthesizer {
public:

  typedef enum {
    CMU_US_ARCTIC_SLT = 0,
    CSTR_UK_FEMALE,

    VOICE_UNKNOWN // keep this at the end
  } voice_t;

  static std::string getVoicePath( voice_t voice );
  static std::string getVoicePath( const std::string & voice );

  FLITEVoiceSynthesizer( const std::string & voice );
  ~FLITEVoiceSynthesizer();
  virtual SGSoundSample * synthesize( const std::string & text, double volume, double speed, double pitch  );

  virtual void synthesize( SynthesizeRequest & request );
private:
  struct _Flite_HTS_Engine * _engine;

  class WorkerThread;
  WorkerThread * _worker;

  typedef SGBlockingQueue<SynthesizeRequest> SynthesizeRequestList;
  SynthesizeRequestList _requests;

  double _volume;
};

#endif /* VOICESYNTHESIZER_HXX_ */
