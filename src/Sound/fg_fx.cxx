// fgfx.cxx -- Sound effect management class implementation
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

#include "fg_fx.hxx"

// FIXME: remove direct dependencies
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <Main/fg_props.hxx>


FGFX::FGFX ()
  : _is_cranking(false),
    _is_stalling(false),
    _is_rumbling(false),
    _engine(0),
    _crank(0),
    _wind(0),
    _stall(0),
    _rumble(0),
    _flaps(0),
    _squeal(0),
    _click(0)
{
}

FGFX::~FGFX ()
{
				// FIXME: is this right, or does the
				// sound manager assume pointer ownership?
  delete _engine;
  delete _crank;
  delete _wind;
  delete _stall;
  delete _rumble;

  delete _flaps;
  delete _squeal;
  delete _click;
}


void
FGFX::init ()
{
  FGSoundMgr * mgr = globals->get_soundmgr();

  //
  // Create and add the engine sound
  //
  _engine =
    new FGSimpleSound(fgGetString("/sim/sounds/engine", "Sounds/wasp.wav"));
  mgr->add(_engine, "engine loop");
  mgr->play_looped("engine loop");

  SG_LOG( SG_GENERAL, SG_INFO,
	  "Rate = " << _engine->get_sample()->getRate()
	  << "  Bps = " << _engine->get_sample()->getBps()
	  << "  Stereo = " << _engine->get_sample()->getStereo() );


  //
  // Create and add the cranking sound.
  //
  _crank =
    new FGSimpleSound(fgGetString("/sim/sounds/cranking",
				  "Sounds/cranking.wav"));
  mgr->add(_crank, "crank");
  _crank->set_pitch(1.5);
  _crank->set_volume(0.25);


  //
  // Create and add the wind noise.
  //
  _wind =
    new FGSimpleSound(fgGetString("/sim/sounds/wind", "Sounds/wind.wav"));
  mgr->add(_wind, "wind");
  mgr->play_looped("wind");


  //
  // Create and add the stall noise.
  //
  _stall = new FGSimpleSound(fgGetString("/sim/sounds/stall",
					 "Sounds/stall.wav"));
  mgr->add(_stall, "stall");

  //
  // Create and add the rumble noise.
  //
  _rumble = new FGSimpleSound(fgGetString("/sim/sounds/rumble",
					  "Sounds/rumble.wav"));
  mgr->add(_rumble, "rumble");

  //
  // Create and add the squeal noise.
  //
  _squeal = new FGSimpleSound(fgGetString("/sim/sounds/squeal",
					  "Sounds/squeal.wav"));
  mgr->add(_squeal, "squeal");

  //
  // Create and add the click noise.
  _click = new FGSimpleSound(fgGetString("/sim/sounds/click",
					 "Sounds/click.wav"));
  mgr->add(_click, "click");
}

void
FGFX::bind ()
{
}

void
FGFX::unbind ()
{
}

void
FGFX::update ()
{
  FGSoundMgr * mgr = globals->get_soundmgr();


  ////////////////////////////////////////////////////////////////////
  // Update the engine sound.
  ////////////////////////////////////////////////////////////////////

  if (fgGetBool("/engines/engine[0]/running")) { // FIXME
	  // pitch corresponds to rpm
	  // volume corresponds to manifold pressure

    double rpm_factor;
    if (cur_fdm_state->get_engine(0) != NULL)
      rpm_factor = cur_fdm_state->get_engine(0)->get_RPM() / 2500.0;
    else
      rpm_factor = 1.0;

    double pitch = 0.3 + rpm_factor * 3.0;

    // don't run at absurdly slow rates -- not realistic
    // and sounds bad to boot.  :-)
    if (pitch < 0.7)
      pitch = 0.7;
    if (pitch > 5.0)
      pitch = 5.0;

    double mp_factor;
    if (cur_fdm_state->get_engine(0) != NULL)
      mp_factor = cur_fdm_state->get_engine(0)->get_Manifold_Pressure() / 100;
    else
      mp_factor = 0.3;

    double volume = 0.15 + mp_factor / 2.0;

    if (volume < 0.15)
      volume = 0.15;
    if (volume > 0.5)
      volume = 0.5;

    _engine->set_pitch( pitch );
    _engine->set_volume( volume );
  } else {
    _engine->set_pitch(0.0);
    _engine->set_volume(0.0);
  }


  ////////////////////////////////////////////////////////////////////
  // Update the cranking sound.
  ////////////////////////////////////////////////////////////////////

  if (fgGetBool("/engines/engine[0]/cranking")) { // FIXME
    if(!_is_cranking) {
      globals->get_soundmgr()->play_looped("crank");
      _is_cranking = true;
    }
  } else {
    if(_is_cranking) {
      globals->get_soundmgr()->stop("crank");
      _is_cranking = false;
    }
  }


  ////////////////////////////////////////////////////////////////////
  // Update the wind noise.
  ////////////////////////////////////////////////////////////////////

  float rel_wind = cur_fdm_state->get_V_rel_wind();
  float volume = rel_wind/300.0;	// FIXME!!!
  _wind->set_volume(volume);


  // TODO: stall

  // TODO: rumble

  // TODO: flaps

  // TODO: squeal

  // TODO: click

}

// end of fg_fx.cxx
