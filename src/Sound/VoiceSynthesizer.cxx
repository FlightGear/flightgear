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
#include <simgear/misc/sg_path.hxx>
#include <simgear/threads/SGThread.hxx>

#include <flite_hts_engine.h>

using std::string;

static const char * VOICE_FILES[] = {
  "cmu_us_arctic_slt.htsvoice",
  "cstr_uk_female-1.0.htsvoice"
};

class FLITEVoiceSynthesizer::WorkerThread : public SGThread
{
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

    // marker value indicating termination requested
    if ((request.speed < 0.0) && (request.volume < 0.0)) {
      SG_LOG(SG_SOUND, SG_DEBUG, "FLITE synthesis thread exiting");
      return;
    }

    if ( NULL != request.listener) {
      SGSharedPtr<SGSoundSample> sample = _synthesizer->synthesize(request.text, request.volume, request.speed, request.pitch);
      request.listener->SoundSampleReady( sample );
    }
  }
}

SynthesizeRequest SynthesizeRequest::cancelThreadRequest()
{
  SynthesizeRequest marker;
  marker.volume = -999.0;
  marker.speed = -999.0;
  return marker;
}

string FLITEVoiceSynthesizer::getVoicePath( voice_t voice )
{
  if( voice < 0 || voice >= VOICE_UNKNOWN ) return string("");
  SGPath voicePath = globals->get_fg_root() / "ATC" /  VOICE_FILES[voice];
  return voicePath.utf8Str();
}

string FLITEVoiceSynthesizer::getVoicePath( const string & voice )
{
  if( voice == "cmu_us_arctic_slt" ) return getVoicePath(CMU_US_ARCTIC_SLT);
  if( voice == "cstr_uk_female" ) return getVoicePath(CSTR_UK_FEMALE);
  return getVoicePath(VOICE_UNKNOWN);
}


void FLITEVoiceSynthesizer::synthesize( SynthesizeRequest & request)
{
  _requests.push(request);
}

FLITEVoiceSynthesizer::FLITEVoiceSynthesizer(const std::string & voice)
    // REVIEW: Memory Leak - 1,696 bytes in 4 blocks are definitely lost in loss record 6,145 of 6,440
    : _engine(new Flite_HTS_Engine), _worker(new FLITEVoiceSynthesizer::WorkerThread(this)), _volume(6.0)
{
  _volume = fgGetDouble("/sim/sound/voice-synthesizer/volume", _volume );
  Flite_HTS_Engine_initialize(_engine);
  Flite_HTS_Engine_load(_engine, voice.c_str());
  _worker->start();
}

FLITEVoiceSynthesizer::~FLITEVoiceSynthesizer()
{
  // push the special marker value
  _requests.push(SynthesizeRequest::cancelThreadRequest());
  _worker->join();
  Flite_HTS_Engine_clear(_engine);
}

SGSoundSample * FLITEVoiceSynthesizer::synthesize(const std::string & text, double volume, double speed, double pitch )
{
  SG_CLAMP_RANGE( volume, 0.0, 1.0 );
  SG_CLAMP_RANGE( speed, 0.0, 10.0 );
  SG_CLAMP_RANGE( pitch, 0.0, 10.0 );
  HTS_Engine_set_volume( &_engine->engine, _volume );
  HTS_Engine_set_speed( &_engine->engine, 0.8 + 0.4 * speed );
  HTS_Engine_add_half_tone(&_engine->engine, -4.0 + 8.0 * pitch );

    
  void* data;
  int rate, count;
  if ( FALSE == Flite_HTS_Engine_synthesize_samples_mono16(_engine, text.c_str(), &data, &count, &rate)) return NULL;

  auto buf = std::unique_ptr<unsigned char, decltype(free)*>{
    reinterpret_cast<unsigned char*>( data ),
    free
  };
  return new SGSoundSample(buf,
                           count * sizeof(short),
                           rate,
                           SG_SAMPLE_MONO16);
}

