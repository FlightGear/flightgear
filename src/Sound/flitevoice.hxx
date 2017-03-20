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
// $Id$

#ifndef _FLITEVOICE_HXX
#define _FLITEVOICE_HXX

#include "voice.hxx"
#include <simgear/sound/soundmgr.hxx>
#include <simgear/sound/sample.hxx>
#include <simgear/threads/SGQueue.hxx>

class VoiceSynthesizer;

class FGFLITEVoice: public FGVoiceMgr::FGVoice {
public:
  FGFLITEVoice(FGVoiceMgr *, const SGPropertyNode_ptr, const char * sampleGroupRefName = "flite-voice");
  virtual ~FGFLITEVoice();
  virtual void speak(const std::string & msg);
  virtual void update(double dt);

private:
  FGFLITEVoice(const FGFLITEVoice & other);
  FGFLITEVoice & operator =(const FGFLITEVoice & other);

  SGSharedPtr<SGSampleGroup> _sgr;
  VoiceSynthesizer * _synthesizer;
  SGLockedQueue<SGSharedPtr<SGSoundSample> > _sampleQueue;
  std::string _sampleName;
  double _seconds_to_run;
};

#endif // _FLITEVOICE_HXX
