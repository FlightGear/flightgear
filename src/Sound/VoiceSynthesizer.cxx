/*
 * VoiceSynthesizer.cxx - wraps flite+hts_engine
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
#include "VoiceSynthesizer.hxx"
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <simgear/sg_inlines.h>
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
      SGSharedPtr<SGSoundSample> sample = _synthesizer->synthesize(request.text, request.volume, request.speed, request.pitch);
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

SGSoundSample * FLITEVoiceSynthesizer::synthesize(const std::string & text, double volume, double speed, double pitch )
{
  ScopedTempfile scratch(_keepScratchFile);

  SG_CLAMP_RANGE( volume, 0.0, 1.0 );
  SG_CLAMP_RANGE( speed, 0.0, 1.0 );
  SG_CLAMP_RANGE( pitch, 0.0, 1.0 );
  HTS_Engine_set_volume( &_engine->engine, _volume );
  HTS_Engine_set_speed( &_engine->engine, 0.8 + 0.4 * speed );
  HTS_Engine_add_half_tone(&_engine->engine, -4.0 + 8.0 * pitch );

  if ( FALSE == Flite_HTS_Engine_synthesize(_engine, text.c_str(), scratch.getName())) return NULL;

  ALenum format;
  ALsizei size;
  ALfloat freqf;
  ALvoid * data = simgear::loadWAVFromFile(scratch.getPath(), format, size, freqf);

  if (data == NULL) {
    SG_LOG(SG_SOUND, SG_ALERT, "Failed to load wav file " << scratch.getPath());
  }

  if (format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16) {
    free(data);
    SG_LOG(SG_SOUND, SG_ALERT, "Warning: STEREO files are not supported for 3D audio effects: " << scratch.getPath());
  }

  return new SGSoundSample(&data, size, (ALsizei) freqf, format);
}

