/*
 * VoiceSynthesizer.hxx
 *
 *  Created on: Apr 24, 2014
 *      Author: flightgear
 */

#ifndef VOICESYNTHESIZER_HXX_
#define VOICESYNTHESIZER_HXX_

#include <simgear/sound/sample_openal.hxx>
#include <simgear/threads/SGQueue.hxx>

#include <string>
struct _Flite_HTS_Engine;

/**
 * A Voice Synthesizer Interface
 */
class VoiceSynthesizer {
public:
  virtual ~VoiceSynthesizer() {};
  virtual SGSoundSample * synthesize( const std::string & text ) = 0;
};

class SoundSampleReadyListener {
public:
  virtual ~SoundSampleReadyListener() {}
  virtual void SoundSampleReady( SGSharedPtr<SGSoundSample> ) = 0;
};

struct SynthesizeRequest {
  SynthesizeRequest() {
    speed = volume = pitch = 1.0;
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
  FLITEVoiceSynthesizer( const std::string & voice );
  ~FLITEVoiceSynthesizer();
  virtual SGSoundSample * synthesize( const std::string & text );

  virtual void synthesize( SynthesizeRequest & request );
private:
  struct _Flite_HTS_Engine * _engine;

  class WorkerThread;
  WorkerThread * _worker;

  typedef SGBlockingQueue<SynthesizeRequest> SynthesizeRequestList;
  SynthesizeRequestList _requests;
};

#endif /* VOICESYNTHESIZER_HXX_ */
