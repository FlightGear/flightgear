/*
 * VoiceSynthesizer.cxx
 *
 *  Created on: Apr 24, 2014
 *      Author: flightgear
 */

#include "VoiceSynthesizer.hxx"
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/sound/readwav.hxx>
#include <simgear/misc/sg_path.hxx>
#include <OpenThreads/Thread>
#include <flite_hts_engine.h>

class ScopedTempfile {
public:
  ScopedTempfile( bool keep = false ) : _keep(keep)
  {
    _name = ::tempnam(globals->get_fg_home().c_str(), "fgvox");

  }
  ~ScopedTempfile()
  {
    if (_name && !_keep) ::unlink(_name);
    ::free(_name);
  }

  const char * getName() const
  {
    return _name;
  }
  SGPath getPath()
  {
    return SGPath(_name);
  }
private:
  char * _name;
  bool _keep;
};

class FLITEVoiceSynthesizer::WorkerThread: public OpenThreads::Thread {
public:
  WorkerThread(FLITEVoiceSynthesizer * synthesizer)
      : _synthesizer(synthesizer)
  {
  }
  virtual void run();
private:
  FLITEVoiceSynthesizer * _synthesizer;
};

void FLITEVoiceSynthesizer::WorkerThread::run()
{
  for (;;) {
    SynthesizeRequest request = _synthesizer->_requests.pop();
    if ( NULL != request.listener) {
      SGSharedPtr<SGSoundSample> sample = _synthesizer->synthesize(request.text);
      request.listener->SoundSampleReady( sample );
    }
  }
}

void FLITEVoiceSynthesizer::synthesize( SynthesizeRequest & request)
{
  _requests.push(request);
}

FLITEVoiceSynthesizer::FLITEVoiceSynthesizer(const std::string & voice)
    : _engine(new Flite_HTS_Engine), _worker(new FLITEVoiceSynthesizer::WorkerThread(this)), _volume(6.0), _keepScratchFile(false)
{
  _volume = fgGetDouble("/sim/sound/voice-synthesizer/volume", _volume );
  _keepScratchFile = fgGetBool("/sim/sound/voice-synthesizer/keep-scratch-file", _keepScratchFile);
  Flite_HTS_Engine_initialize(_engine);
  Flite_HTS_Engine_load(_engine, voice.c_str());
  _worker->start();
}

FLITEVoiceSynthesizer::~FLITEVoiceSynthesizer()
{
  _worker->cancel();
  _worker->join();
  Flite_HTS_Engine_clear(_engine);
}

SGSoundSample * FLITEVoiceSynthesizer::synthesize(const std::string & text)
{
  ScopedTempfile scratch(_keepScratchFile);
  HTS_Engine_set_volume( &_engine->engine, _volume );

  if ( FALSE == Flite_HTS_Engine_synthesize(_engine, text.c_str(), scratch.getName())) return NULL;

  SG_LOG(SG_SOUND, SG_ALERT, "created wav at " << scratch.getPath());

  ALenum format;
  ALsizei size;
  ALfloat freqf;
  ALvoid * data = simgear::loadWAVFromFile(scratch.getPath(), format, size, freqf);
  SG_LOG(SG_ALL, SG_ALERT, "loaded wav at " << freqf << "Hz size=" << size << " format=" << format);

  if (data == NULL) {
    SG_LOG(SG_SOUND, SG_ALERT, "Failed to load wav file " << scratch.getPath());
  }

  if (format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16) {
    free(data);
    SG_LOG(SG_SOUND, SG_ALERT, "Warning: STEREO files are not supported for 3D audio effects: " << scratch.getPath());
  }

  return new SGSoundSample(&data, size, (ALsizei) freqf, format);
}

