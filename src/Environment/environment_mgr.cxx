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
#include <simgear/sky/sky.hxx>

#include <Main/fg_props.hxx>
#include <Aircraft/aircraft.hxx>

#include "environment.hxx"
#include "environment_ctrl.hxx"
#include "environment_mgr.hxx"

extern SGSky *thesky;		// FIXME: from main.cxx


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

  for (int i = 0; i < MAX_CLOUD_LAYERS; i++) {
    char buf[128];
    sprintf(buf, "/environment/clouds/layer[%d]/span-m", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_span_m,
	  &FGEnvironmentMgr::set_cloud_layer_span_m);
    sprintf(buf, "/environment/clouds/layer[%d]/elevation-ft", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_elevation_ft,
	  &FGEnvironmentMgr::set_cloud_layer_elevation_ft);
    sprintf(buf, "/environment/clouds/layer[%d]/thickness-ft", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_thickness_ft,
	  &FGEnvironmentMgr::set_cloud_layer_thickness_ft);
    sprintf(buf, "/environment/clouds/layer[%d]/transition-ft", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_transition_ft,
	  &FGEnvironmentMgr::set_cloud_layer_transition_ft);
    sprintf(buf, "/environment/clouds/layer[%d]/type", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_type,
	  &FGEnvironmentMgr::set_cloud_layer_type);
  }
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
  for (int i = 0; i < MAX_CLOUD_LAYERS; i++) {
    char buf[128];
    sprintf(buf, "/environment/clouds/layer[%d]/span-m", i);
    fgUntie(buf);
    sprintf(buf, "/environment/clouds/layer[%d]/elevation-ft", i);
    fgUntie(buf);
    sprintf(buf, "/environment/clouds/layer[%d]/thickness-ft", i);
    fgUntie(buf);
    sprintf(buf, "/environment/clouds/layer[%d]/transition-ft", i);
    fgUntie(buf);
    sprintf(buf, "/environment/clouds/layer[%d]/type", i);
    fgUntie(buf);
  }
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

double
FGEnvironmentMgr::get_cloud_layer_span_m (int index) const
{
  return thesky->get_cloud_layer(index)->getSpan_m();
}

void
FGEnvironmentMgr::set_cloud_layer_span_m (int index, double span_m)
{
  thesky->get_cloud_layer(index)->setSpan_m(span_m);
}

double
FGEnvironmentMgr::get_cloud_layer_elevation_ft (int index) const
{
  return thesky->get_cloud_layer(index)->getElevation_m() * SG_METER_TO_FEET;
}

void
FGEnvironmentMgr::set_cloud_layer_elevation_ft (int index, double elevation_ft)
{
  thesky->get_cloud_layer(index)
    ->setElevation_m(elevation_ft * SG_FEET_TO_METER);
}

double
FGEnvironmentMgr::get_cloud_layer_thickness_ft (int index) const
{
  return thesky->get_cloud_layer(index)->getThickness_m() * SG_METER_TO_FEET;
}

void
FGEnvironmentMgr::set_cloud_layer_thickness_ft (int index, double thickness_ft)
{
  thesky->get_cloud_layer(index)
    ->setThickness_m(thickness_ft * SG_FEET_TO_METER);
}

double
FGEnvironmentMgr::get_cloud_layer_transition_ft (int index) const
{
  return thesky->get_cloud_layer(index)->getTransition_m() * SG_METER_TO_FEET;
}

void
FGEnvironmentMgr::set_cloud_layer_transition_ft (int index,
						 double transition_ft)
{
  thesky->get_cloud_layer(index)
    ->setTransition_m(transition_ft * SG_FEET_TO_METER);
}

const char *
FGEnvironmentMgr::get_cloud_layer_type (int index) const
{
  switch (thesky->get_cloud_layer(index)->getType()) {
  case SGCloudLayer::SG_CLOUD_OVERCAST:
    return "overcast";
  case SGCloudLayer::SG_CLOUD_MOSTLY_CLOUDY:
    return "mostly-cloudy";
  case SGCloudLayer::SG_CLOUD_MOSTLY_SUNNY:
    return "mostly-sunny";
  case SGCloudLayer::SG_CLOUD_CIRRUS:
    return "cirrus";
  case SGCloudLayer::SG_CLOUD_CLEAR:
    return "clear";
  default:
    return "unknown";
  }
}

void
FGEnvironmentMgr::set_cloud_layer_type (int index, const char * type_name)
{
  SGCloudLayer::Type type;
  if (!strcmp(type_name, "overcast"))
    type = SGCloudLayer::SG_CLOUD_OVERCAST;
  else if (!strcmp(type_name, "mostly-cloudy"))
    type = SGCloudLayer::SG_CLOUD_MOSTLY_CLOUDY;
  else if (!strcmp(type_name, "mostly-sunny"))
    type = SGCloudLayer::SG_CLOUD_MOSTLY_SUNNY;
  else if (!strcmp(type_name, "cirrus"))
    type = SGCloudLayer::SG_CLOUD_CIRRUS;
  else if (!strcmp(type_name, "clear"))
    type = SGCloudLayer::SG_CLOUD_CLEAR;
  else {
    SG_LOG(SG_INPUT, SG_WARN, "Unknown cloud type " << type_name);
    type = SGCloudLayer::SG_CLOUD_CLEAR;
  }
  thesky->get_cloud_layer(index)->setType(type);
}



// end of environment-mgr.cxx
