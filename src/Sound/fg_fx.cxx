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

#include <FDM/flight.hxx>	// FIXME: remove direct dependencies
#include <Main/fg_props.hxx>

#include "fg_fx.hxx"


static const char * engine_names[FGFX::MAX_ENGINES] = {
  "engine0",
  "engine1"
};

static const char * crank_names[FGFX::MAX_ENGINES] = {
  "crank0",
  "crank1"
};


FGFX::FGFX ()
  : _old_flap_position(0),
    _old_gear_position(0),
    _wind(0),
    _stall(0),
    _rumble(0),
    _flaps(0),
    _gear_up(0),
    _gear_dn(0),
    _squeal(0),
    _click(0),
    _stall_warning_prop(0),
    _flaps_prop(0),
    _gear_prop(0)
{
  for (int i = 0; i < MAX_ENGINES; i++) {
    _engine[i] = 0;
    _crank[i] = 0;
    _engine_running_prop[i] = 0;
    _engine_cranking_prop[i] = 0;
  }
}

FGFX::~FGFX ()
{
				// FIXME: is this right, or does the
				// sound manager assume pointer ownership?
  for (int i = 0; i < MAX_ENGINES; i++) {
    delete _engine[i];
    delete _crank[i];
  }
  delete _wind;
  delete _stall;
  delete _rumble;

  delete _flaps;
  delete _gear_up;
  delete _gear_dn;
  delete _squeal;
  delete _click;
}


void
FGFX::init ()
{
  FGSoundMgr * mgr = globals->get_soundmgr();
  int i;

  //
  // Create and add engine-related sounds.
  //
  for (i = 0; i < MAX_ENGINES; i++) {
				// Engine
    _engine[i] =
      new FGSimpleSound(fgGetString("/sim/sounds/engine/path",
				    "Sounds/wasp.wav"));
    _engine[i]->set_volume(fgGetFloat("/sim/sounds/engine/volume", 1.0));
    _engine[i]->set_pitch(fgGetFloat("/sim/sounds/engine/pitch", 1.0));
    mgr->add(_engine[i], engine_names[i]);

				// Starter
    _crank[i] = new FGSimpleSound(fgGetString("/sim/sounds/cranking/path",
					      "Sounds/cranking.wav"));
    _crank[i]->set_volume(fgGetFloat("/sim/sounds/cranking/volume", 0.175));
    _crank[i]->set_pitch(fgGetFloat("/sim/sounds/cranking/pitch", 1.25));
    mgr->add(_crank[i], crank_names[i]);
  }


  //
  // Create and add the wind noise.
  //
  _wind = new FGSimpleSound(fgGetString("/sim/sounds/wind/path",
					"Sounds/wind.wav"));
  _wind->set_volume(fgGetFloat("/sim/sounds/wind/volume", 1.0));
  _wind->set_pitch(fgGetFloat("/sim/sounds/wind/pitch", 1.0));
  mgr->add(_wind, "wind");


  //
  // Create and add the stall noise.
  //
  _stall = new FGSimpleSound(fgGetString("/sim/sounds/stall/path",
					 "Sounds/stall.wav"));
  _stall->set_volume(fgGetFloat("/sim/sounds/stall/volume", 1.0));
  _stall->set_pitch(fgGetFloat("/sim/sounds/stall/pitch", 1.0));
  mgr->add(_stall, "stall");

  //
  // Create and add the rumble noise.
  //
  _rumble = new FGSimpleSound(fgGetString("/sim/sounds/rumble/path",
					  "Sounds/rumble.wav"));
  _rumble->set_volume(fgGetFloat("/sim/sounds/rumble/volume", 1.0));
  _rumble->set_pitch(fgGetFloat("/sim/sounds/rumble/pitch", 1.0));
  mgr->add(_rumble, "rumble");


  //
  // Create and add the flaps noise
  //
  _flaps = new FGSimpleSound(fgGetString("/sim/sounds/flaps/path",
					 "Sounds/flaps.wav"));
  _flaps->set_volume(fgGetFloat("/sim/sounds/flaps/volume", 0.5));
  _flaps->set_pitch(fgGetFloat("/sim/sounds/flaps/pitch", 1.0));
  mgr->add(_flaps, "flaps");

  //
  // Create and add the gear noises.
  //
  _gear_up = new FGSimpleSound(fgGetString("/sim/sounds/gear-up/path",
					   "Sounds/gear-up.wav"));
  _gear_dn = new FGSimpleSound(fgGetString("/sim/sounds/gear-down/path",
					   "Sounds/gear-dn.wav"));
  _gear_up->set_volume(fgGetFloat("/sim/sounds/gear-up/volume", 1.0));
  _gear_dn->set_volume(fgGetFloat("/sim/sounds/gear-down/volume", 1.0));
  _gear_up->set_pitch(fgGetFloat("/sim/sounds/gear-up/pitch", 1.0));
  _gear_dn->set_pitch(fgGetFloat("/sim/sounds/gear-down/pitch", 1.0));
  mgr->add(_gear_up, "gear-up");
  mgr->add(_gear_dn, "gear-down");

  //
  // Create and add the squeal noise.
  //
  _squeal = new FGSimpleSound(fgGetString("/sim/sounds/squeal/path",
					  "Sounds/squeal.wav"));
  _squeal->set_volume(fgGetFloat("/sim/sounds/squeal/volume", 1.0));
  _squeal->set_pitch(fgGetFloat("/sim/sounds/squeal/pitch", 1.0));
  mgr->add(_squeal, "squeal");

  //
  // Create and add the click noise.
  _click = new FGSimpleSound(fgGetString("/sim/sounds/click/path",
					 "Sounds/click.wav"));
  _flaps->set_volume(fgGetFloat("/sim/sounds/click/volume", 1.0));
  _flaps->set_pitch(fgGetFloat("/sim/sounds/click/pitch", 1.0));
  mgr->add(_click, "click");


  ////////////////////////////////////////////////////////////////////
  // Grab some properties.
  ////////////////////////////////////////////////////////////////////

  for (i = 0; i < MAX_ENGINES; i++) {
    char buf[100];
    sprintf(buf, "/engines/engine[%d]/running", i);
    _engine_running_prop[i] = fgGetNode(buf, true);
    sprintf(buf, "/engines/engine[%d]/cranking", i);
    _engine_cranking_prop[i] = fgGetNode(buf, true);
  }
  _stall_warning_prop = fgGetNode("/sim/aero/alarms/stall-warning", true);
  _vc_prop = fgGetNode("/velocities/airspeed-kt", true);
  _flaps_prop = fgGetNode("/controls/flaps", true);
  _gear_prop = fgGetNode("/controls/gear-down", true);
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
  int i;

  ////////////////////////////////////////////////////////////////////
  // Update the engine sound.
  ////////////////////////////////////////////////////////////////////
  
  for (i = 0; i < MAX_ENGINES; i++) {

    if (cur_fdm_state->get_num_engines() > 0 &&
	_engine_running_prop[i]->getBoolValue()) {
	  // pitch corresponds to rpm
	  // volume corresponds to manifold pressure

      double rpm_factor;
      if ( cur_fdm_state->get_num_engines() > 0 )
	rpm_factor = cur_fdm_state->get_engine(i)->get_RPM() / 2500.0;
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
      if ( cur_fdm_state->get_num_engines() > 0 )
	mp_factor =
	  cur_fdm_state->get_engine(i)->get_Manifold_Pressure() / 100;
      else
	mp_factor = 0.3;

      double volume = 0.15 + mp_factor / 2.0;

      if (volume < 0.15)
	volume = 0.15;
      if (volume > 0.5)
	volume = 0.5;

      _engine[i]->set_pitch( pitch );
      _engine[i]->set_volume( volume );
      set_playing(engine_names[i], true);
    } else {
      set_playing(engine_names[i], false);
    }

				// FIXME
    set_playing(crank_names[i], _engine_cranking_prop[i]->getBoolValue());
  }


  ////////////////////////////////////////////////////////////////////
  // Update the wind noise.
  ////////////////////////////////////////////////////////////////////

  float rel_wind = cur_fdm_state->get_V_rel_wind(); // FPS
  float airspeed_kt = cur_fdm_state->get_V_equiv_kts();
  if (rel_wind > 60.0) {	// a little off 30kt
    // float volume = rel_wind/600.0;	// FIXME!!!
    float volume = rel_wind/937.0;      // FIXME!!!
    double pitch = 1.0+(airspeed_kt/113.0);
    _wind->set_volume(volume);
    _wind->set_pitch(pitch);
    set_playing("wind", true);
  } else {
    set_playing("wind", false);
  }


  ////////////////////////////////////////////////////////////////////
  // Update the stall horn.
  ////////////////////////////////////////////////////////////////////

  double stall = _stall_warning_prop->getDoubleValue();
  double vc = _vc_prop->getDoubleValue();
  if (stall > 0.0 && vc > 30.0) {
    _stall->set_volume(stall);
    set_playing("stall", true);
  } else {
    set_playing("stall", false);
  }


  ////////////////////////////////////////////////////////////////////
  // Update the rumble.
  ////////////////////////////////////////////////////////////////////

  float totalGear = min(cur_fdm_state->get_num_gear(), int(MAX_GEAR));
  float gearOnGround = 0;


				// Calculate whether a squeal is
				// required, and set the volume.
				// Currently, the squeal volume is the
				// current local down velocity in feet
				// per second divided by 10.0, and
				// will not be played if under 0.1.

				// FIXME: take rotational velocities
				// into account as well.
  for (i = 0; i < totalGear; i++) {
    if (cur_fdm_state->get_gear_unit(i)->GetWoW()) {
      gearOnGround++;
      if (!_gear_on_ground[i]) {
        // 3 parts horizontal velocity + 1 part vertical velocity
	double squeal_volume = 0.75 * cur_fdm_state->get_V_equiv_kts() / 90.0 +
            0.25 * cur_fdm_state->get_V_down() / 5.0;
	if (squeal_volume > 0.1) {
	  _squeal->set_volume(squeal_volume);
	  _squeal->set_pitch(1.25);
	  mgr->play_once("squeal");
	}
	_gear_on_ground[i] = true;
      }
    } else {
      _gear_on_ground[i] = false;
    }
  }

				// Now, if any of the gear is in
				// contact with the ground play the
				// rumble sound.  The volume is the
				// absolute velocity in knots divided
				// by 120.0.  No rumble will be played
				// if the velocity is under 6kt.
  double speed = cur_fdm_state->get_V_equiv_kts();
  if (gearOnGround > 0 && speed >= 6.0) {
    double volume = 2.0 * (gearOnGround/totalGear) * (speed/60.0);
    _rumble->set_volume(volume);
    set_playing("rumble", true);
  } else {
    set_playing("rumble", false);
  }


  ////////////////////////////////////////////////////////////////////
  // Check for flap movement.
  ////////////////////////////////////////////////////////////////////

  double flap_position = _flaps_prop->getDoubleValue();
  if (fabs(flap_position - _old_flap_position) > 0.1) {
    mgr->play_once("flaps");
    _old_flap_position = flap_position;
  }


  ////////////////////////////////////////////////////////////////////
  // Check for gear movement.
  ////////////////////////////////////////////////////////////////////
  
  double gear_position = _gear_prop->getDoubleValue();
  if (gear_position != _old_gear_position) {
    if (gear_position < _old_gear_position) {
      mgr->play_once("gear-up");
    } else {
      mgr->play_once("gear-down");
    }
    _old_gear_position = gear_position;
  }

  // TODO: click

}


void
FGFX::set_playing (const char * soundName, bool state)
{
  FGSoundMgr * mgr = globals->get_soundmgr();
  bool playing = mgr->is_playing(soundName);
  if (state && !playing)
    mgr->play_looped(soundName);
  else if (!state && playing)
    mgr->stop(soundName);
}

// end of fg_fx.cxx
