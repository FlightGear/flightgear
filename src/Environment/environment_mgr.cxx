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

#include "environment_mgr.hxx"


FGEnvironmentMgr::FGEnvironmentMgr ()
{
  _environment = new FGEnvironment();
}

FGEnvironmentMgr::~FGEnvironmentMgr ()
{
  delete _environment;
}

void
FGEnvironmentMgr::init ()
{
  SG_LOG( SG_GENERAL, SG_INFO, "Initializing environment subsystem");
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
  fgTie("/environment/pressure-sea-level-inhg", _environment,
	&FGEnvironment::get_pressure_sea_level_inhg,
	&FGEnvironment::set_pressure_sea_level_inhg);
  fgSetArchivable("/environment/pressure-sea-level-inhg");
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
FGEnvironmentMgr::update (int dt)
{
				// FIXME: the FDMs should update themselves
  current_aircraft.fdm_state
    ->set_Velocities_Local_Airmass(_environment->get_wind_from_north_fps(),
				   _environment->get_wind_from_east_fps(),
				   _environment->get_wind_from_down_fps());
}

const FGEnvironment *
FGEnvironmentMgr::getEnvironment () const
{
  return _environment;
}

const FGEnvironment *
FGEnvironmentMgr::getEnvironment (double lat, double lon, double alt) const
{
				// Always returns the same environment
				// for now; we'll make it interesting
				// later.
  return _environment;
}

// end of environment-mgr.cxx
