// fgfx.hxx -- Sound effect management class
//
// Started by David Megginson, October 2001
// (Reuses some code from main.cxx, probably by Curtis Olson)
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

#ifndef __FGFX_HXX
#define __FGFX_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <Main/fgfs.hxx>
#include <Main/globals.hxx>

#include "soundmgr.hxx"


/**
 * Generator for FlightGear sound effects.
 *
 * This module uses FGSoundMgr to generate sound effects based
 * on current flight conditions.  The sound manager must be initialized
 * before this object is.
 */
class FGFX : public FGSubsystem
{

public:

  enum {
    MAX_ENGINES = 2,		// TODO: increase later
    MAX_GEAR = 20,
  };

  FGFX ();
  virtual ~FGFX ();

  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (int dt);

private:

  void set_playing (const char * soundName, bool state = true);

  double _old_flap_position;
  double _old_gear_position;

  bool _gear_on_ground[MAX_GEAR];
  float _wheel_spin[MAX_GEAR];

				// looped sounds
  FGSimpleSound * _engine[MAX_ENGINES];
  FGSimpleSound * _crank[MAX_ENGINES];
  FGSimpleSound * _wind;
  FGSimpleSound * _stall;
  FGSimpleSound * _rumble;

				// one-off sounds
  FGSimpleSound * _flaps;
  FGSimpleSound * _gear_up;
  FGSimpleSound * _gear_dn;
  FGSimpleSound * _squeal;
  FGSimpleSound * _click;

				// Cached property nodes.
  const SGPropertyNode * _engine_running_prop[MAX_ENGINES];
  const SGPropertyNode * _engine_cranking_prop[MAX_ENGINES];
  const SGPropertyNode * _stall_warning_prop;
  const SGPropertyNode * _vc_prop;
  const SGPropertyNode * _flaps_prop;
  const SGPropertyNode * _gear_prop;

};


#endif

// end of fgfx.hxx
