// environment-mgr.cxx -- manager for natural environment information.
//
// Written by David Megginson, started February 2002.
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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

#include <simgear/debug/logstream.hxx>

#include <Main/fg_props.hxx>
#include <Aircraft/aircraft.hxx>

#include "environment.hxx"
#include "environment_ctrl.hxx"
#include "environment_mgr.hxx"


FGEnvironmentMgr::FGEnvironmentMgr ()
  : _environment(new FGEnvironment),
    _controller(new FGUserDefEnvironmentCtrl)
{
}

FGEnvironmentMgr::~FGEnvironmentMgr ()
{
  delete _environment;
  delete _controller;
}

void
FGEnvironmentMgr::init ()
{
  SG_LOG( SG_GENERAL, SG_INFO, "Initializing environment subsystem");
  _controller->setEnvironment(_environment);
  _controller->init();
  _update_fdm();
}

void
FGEnvironmentMgr::bind ()
{
  fgTie("/environment/visibility-m", _environment,
	&FGEnvironment::get_visibility_m, &FGEnvironment::set_visibility_m);
  fgSetArchivable("/environment/visibility-m");
  fgTie("/environment/temperature-sea-level-degc", _environment,
	&FGEnvironment::get_temperature_sea_level_degc,
	&FGEnvironment::set_temperature_sea_level_degc);
  fgSetArchivable("/environment/temperature-sea-level-degc");
  fgTie("/environment/temperature-degc", _environment,
	&FGEnvironment::get_temperature_degc,
	&FGEnvironment::set_temperature_degc);
  fgSetArchivable("/environment/temperature-degc");
  fgTie("/environment/pressure-sea-level-inhg", _environment,
	&FGEnvironment::get_pressure_sea_level_inhg,
	&FGEnvironment::set_pressure_sea_level_inhg);
  fgSetArchivable("/environment/pressure-sea-level-inhg");
  fgTie("/environment/pressure-inhg", _environment,
	&FGEnvironment::get_pressure_inhg,
	&FGEnvironment::set_pressure_inhg);
  fgSetArchivable("/environment/pressure-inhg");
  fgTie("/environment/density-sea-level-slugft3", _environment,
	&FGEnvironment::get_density_sea_level_slugft3,
	&FGEnvironment::set_density_sea_level_slugft3);
  fgSetArchivable("/environment/density-sea-level-slugft3");
  fgTie("/environment/density-slugft3", _environment,
	&FGEnvironment::get_density_slugft3,
	&FGEnvironment::set_density_slugft3);
  fgSetArchivable("/environment/density-inhg");
  fgTie("/environment/wind-from-heading-deg", _environment,
	&FGEnvironment::get_wind_from_heading_deg,
	&FGEnvironment::set_wind_from_heading_deg);
  fgTie("/environment/wind-speed-kt", _environment,
	&FGEnvironment::get_wind_speed_kt, &FGEnvironment::set_wind_speed_kt);
  fgTie("/environment/wind-from-north-fps", _environment,
	&FGEnvironment::get_wind_from_north_fps,
	&FGEnvironment::set_wind_from_north_fps);
  fgSetArchivable("/environment/wind-from-north-fps");
  fgTie("/environment/wind-from-east-fps", _environment,
	&FGEnvironment::get_wind_from_east_fps,
	&FGEnvironment::set_wind_from_east_fps);
  fgSetArchivable("/environment/wind-from-east-fps");
  fgTie("/environment/wind-from-down-fps", _environment,
	&FGEnvironment::get_wind_from_down_fps,
	&FGEnvironment::set_wind_from_down_fps);
  fgSetArchivable("/environment/wind-from-down-fps");
}

void
FGEnvironmentMgr::unbind ()
{
  fgUntie("/environment/visibility-m");
  fgUntie("/environment/wind-from-heading-deg");
  fgUntie("/environment/wind-speed-kt");
  fgUntie("/environment/wind-from-north-fps");
  fgUntie("/environment/wind-from-east-fps");
  fgUntie("/environment/wind-from-down-fps");
}

void
FGEnvironmentMgr::update (double dt)
{
  _controller->update(dt);
				// FIXME: the FDMs should update themselves
  current_aircraft.fdm_state
    ->set_Velocities_Local_Airmass(_environment->get_wind_from_north_fps(),
				   _environment->get_wind_from_east_fps(),
				   _environment->get_wind_from_down_fps());
  _environment->set_elevation_ft(fgGetDouble("/position/altitude-ft"));

  _update_fdm();
}

FGEnvironment
FGEnvironmentMgr::getEnvironment () const
{
  return *_environment;
}

FGEnvironment
FGEnvironmentMgr::getEnvironment (double lat, double lon, double alt) const
{
				// Always returns the same environment
				// for now; we'll make it interesting
				// later.
  FGEnvironment env = *_environment;
  env.set_elevation_ft(alt);
  return env;
}

void
FGEnvironmentMgr::_update_fdm () const
{
  //
  // Pass atmosphere on to FDM
  // FIXME: have FDMs read properties directly.
  //
  if (fgGetBool("/environment/params/control-fdm-atmosphere")) {
				// convert from Rankine to Celsius
    cur_fdm_state
      ->set_Static_temperature((9.0/5.0)
			       * _environment->get_temperature_degc() + 492.0);
				// convert from inHG to PSF
    cur_fdm_state
      ->set_Static_pressure(_environment->get_pressure_inhg() * 70.726566);
				// keep in slugs/ft^3
    cur_fdm_state
      ->set_Density(_environment->get_density_slugft3());
  }
}

// end of environment-mgr.cxx
