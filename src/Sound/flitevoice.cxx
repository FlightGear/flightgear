// speech synthesis interface subsystem
//
// Written by Torsten Dreyer, started April 2014
//
// Copyright (C) 2014  Torsten Dreyer - torsten (at) t3r (dot) de
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "flitevoice.hxx"
#include <Main/fg_props.hxx>
#include <simgear/sound/sample_group.hxx>
#include <flite_hts_engine.h>

using std::string;

#include "VoiceSynthesizer.hxx"

FGFLITEVoice::FGFLITEVoice(FGVoiceMgr * mgr, const SGPropertyNode_ptr node, const char * sampleGroupRefName)
    : FGVoice(mgr), _synthesizer( NULL)
{

  _sampleName = node->getStringValue("desc", node->getPath().c_str());

  string voice = globals->get_fg_root() + "/ATC/" +
      node->getStringValue("htsvoice", "cmu_us_arctic_slt.htsvoice");

  _synthesizer = new FLITEVoiceSynthesizer(voice.c_str());

  SGSoundMgr *smgr = globals->get_soundmgr();
  _sgr = smgr->find(sampleGroupRefName, true);
  _sgr->tie_to_listener();

  node->getNode("text", true)->addChangeListener(this);

  SG_LOG(SG_SOUND, SG_INFO, "FLITEVoice initialized for sample-group '" << sampleGroupRefName
      << "'. Samples will be named '" << _sampleName << "' "
      << "voice is '" << voice << "'");
}

FGFLITEVoice::~FGFLITEVoice()
{
  delete _synthesizer;
}

void FGFLITEVoice::speak(const string & msg)
{
  // this is called from voice.cxx:FGVoiceMgr::FGVoiceThread::run
  string s = simgear::strutils::strip(msg);
  if (false == s.empty()) {
    _sampleQueue.push(_synthesizer->synthesize(msg, 1.0, 0.5, 0.5));
  }
}

void FGFLITEVoice::update()
{
  SGSharedPtr<SGSoundSample> sample = _sampleQueue.pop();
  if (sample.valid()) {
    _sgr->remove(_sampleName);
    _sgr->add(sample, _sampleName);
    _sgr->resume();
    _sgr->play(_sampleName, false);
  }
}

