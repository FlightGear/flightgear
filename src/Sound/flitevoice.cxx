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

FGFLITEVoice::FGFLITEVoice(FGVoiceMgr * mgr, const SGPropertyNode_ptr node)
    : FGVoice(mgr), _synthesizer( NULL )
{
  string voice = globals->get_fg_root() + "/ATC/cmu_us_arctic_slt.htsvoice";
  _synthesizer = new FLITEVoiceSynthesizer( voice.c_str() );

  SGSoundMgr *smgr = globals->get_soundmgr();
  _sgr = smgr->find("atc", true);
  _sgr->tie_to_listener();

  node->getNode("text", true)->addChangeListener(this);

  SG_LOG(SG_ALL, SG_ALERT, "FLITEVoice initialized");
}

FGFLITEVoice::~FGFLITEVoice()
{
  delete _synthesizer;
  SG_LOG(SG_ALL, SG_ALERT, "FLITEVoice dtor()");
}

void FGFLITEVoice::speak(const string & msg)
{
  SG_LOG(SG_ALL, SG_ALERT, "FLITEVoice speak(" << msg << ")");

  _sgr->remove("flite");

  string s = simgear::strutils::strip( msg );
  if( false == s.empty() ) {
    SGSoundSample * sample = _synthesizer->synthesize( msg );
    _sgr->add(sample, "flite");
    _sgr->set_volume(1.0);
    _sgr->resume();
    _sgr->play("flite", true);
  }
}

void FGFLITEVoice::update()
{

}

