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
#include <simgear/scene/sky/sky.hxx>
#include <simgear/environment/visual_enviro.hxx>

#include <Main/main.hxx>
#include <Main/fg_props.hxx>
#include <Aircraft/aircraft.hxx>

#include "environment.hxx"
#include "environment_ctrl.hxx"
#include "environment_mgr.hxx"
#include "fgclouds.hxx"

class SGSky;
extern SGSky *thesky;



FGEnvironmentMgr::FGEnvironmentMgr ()
  : _environment(new FGEnvironment)
{

   if (fgGetBool("/environment/params/real-world-weather-fetch") == true)
       _controller = new FGMetarEnvironmentCtrl;
   else
       _controller = new FGInterpolateEnvironmentCtrl;

  _controller->setEnvironment(_environment);
  set_subsystem("controller", _controller, 0.5);
  fgClouds = new FGClouds;
}

FGEnvironmentMgr::~FGEnvironmentMgr ()
{
  delete _environment;
  delete _controller;
  delete fgClouds;
}

void
FGEnvironmentMgr::init ()
{
  SG_LOG( SG_GENERAL, SG_INFO, "Initializing environment subsystem");
  SGSubsystemGroup::init();
  _update_fdm();
}

void
FGEnvironmentMgr::reinit ()
{
  SG_LOG( SG_GENERAL, SG_INFO, "Reinitializing environment subsystem");
  SGSubsystemGroup::reinit();
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
	&FGEnvironment::get_temperature_degc); // FIXME: read-only for now
  fgTie("/environment/temperature-degf", _environment,
	&FGEnvironment::get_temperature_degf); // FIXME: read-only for now
  fgTie("/environment/dewpoint-sea-level-degc", _environment,
	&FGEnvironment::get_dewpoint_sea_level_degc,
	&FGEnvironment::set_dewpoint_sea_level_degc);
  fgSetArchivable("/environment/dewpoint-sea-level-degc");
  fgTie("/environment/dewpoint-degc", _environment,
	&FGEnvironment::get_dewpoint_degc); // FIXME: read-only for now
  fgTie("/environment/pressure-sea-level-inhg", _environment,
	&FGEnvironment::get_pressure_sea_level_inhg,
	&FGEnvironment::set_pressure_sea_level_inhg);
  fgSetArchivable("/environment/pressure-sea-level-inhg");
  fgTie("/environment/pressure-inhg", _environment,
	&FGEnvironment::get_pressure_inhg); // FIXME: read-only for now
  fgTie("/environment/density-slugft3", _environment,
	&FGEnvironment::get_density_slugft3); // read-only
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
  fgTie("/environment/turbulence/magnitude-norm", _environment,
        &FGEnvironment::get_turbulence_magnitude_norm,
        &FGEnvironment::set_turbulence_magnitude_norm);
  fgSetArchivable("/environment/turbulence/magnitude-norm");
  fgTie("/environment/turbulence/rate-hz", _environment,
        &FGEnvironment::get_turbulence_rate_hz,
        &FGEnvironment::set_turbulence_rate_hz);
  fgSetArchivable("/environment/turbulence/rate-hz");

  for (int i = 0; i < MAX_CLOUD_LAYERS; i++) {
    char buf[128];
    sprintf(buf, "/environment/clouds/layer[%d]/span-m", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_span_m,
	  &FGEnvironmentMgr::set_cloud_layer_span_m);
    fgSetArchivable(buf);
    sprintf(buf, "/environment/clouds/layer[%d]/elevation-ft", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_elevation_ft,
	  &FGEnvironmentMgr::set_cloud_layer_elevation_ft);
    fgSetArchivable(buf);
    sprintf(buf, "/environment/clouds/layer[%d]/thickness-ft", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_thickness_ft,
	  &FGEnvironmentMgr::set_cloud_layer_thickness_ft);
    fgSetArchivable(buf);
    sprintf(buf, "/environment/clouds/layer[%d]/transition-ft", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_transition_ft,
	  &FGEnvironmentMgr::set_cloud_layer_transition_ft);
    fgSetArchivable(buf);
    sprintf(buf, "/environment/clouds/layer[%d]/coverage", i);
    fgTie(buf, this, i,
	  &FGEnvironmentMgr::get_cloud_layer_coverage,
	  &FGEnvironmentMgr::set_cloud_layer_coverage);
    fgSetArchivable(buf);
  }
  fgTie("/sim/rendering/clouds3d-enable", &sgEnviro,
	  &SGEnviro::get_clouds_enable_state,
	  &SGEnviro::set_clouds_enable_state);
  fgTie("/sim/rendering/clouds3d-vis-range", &sgEnviro,
	  &SGEnviro::get_clouds_visibility,
	  &SGEnviro::set_clouds_visibility);
  fgTie("/sim/rendering/clouds3d-density", &sgEnviro,
	  &SGEnviro::get_clouds_density,
	  &SGEnviro::set_clouds_density);
  fgTie("/sim/rendering/clouds3d-cache-size", &sgEnviro,
	  &SGEnviro::get_clouds_CacheSize,
	  &SGEnviro::set_clouds_CacheSize);
  fgTie("/sim/rendering/clouds3d-cache-resolution", &sgEnviro,
	  &SGEnviro::get_CacheResolution,
	  &SGEnviro::set_CacheResolution);
  fgTie("/sim/rendering/precipitation-enable", &sgEnviro,
	  &SGEnviro::get_precipitation_enable_state,
	  &SGEnviro::set_precipitation_enable_state);
  fgTie("/environment/rebuild_layers", fgClouds,
      &FGClouds::get_update_event,
      &FGClouds::set_update_event);
  fgTie("/sim/rendering/lightning-enable", &sgEnviro,
      &SGEnviro::get_lightning_enable_state,
      &SGEnviro::set_lightning_enable_state);
  fgTie("/environment/turbulence/use-cloud-turbulence", &sgEnviro,
      &SGEnviro::get_turbulence_enable_state,
      &SGEnviro::set_turbulence_enable_state);
}

void
FGEnvironmentMgr::unbind ()
{
  fgUntie("/environment/visibility-m");
  fgUntie("/environment/temperature-sea-level-degc");
  fgUntie("/environment/temperature-degc");
  fgUntie("/environment/dewpoint-sea-level-degc");
  fgUntie("/environment/dewpoint-degc");
  fgUntie("/environment/pressure-sea-level-inhg");
  fgUntie("/environment/pressure-inhg");
  fgUntie("/environment/density-inhg");
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
  fgUntie("/sim/rendering/clouds3d-enable");
  fgUntie("/sim/rendering/clouds3d-vis-range");
  fgUntie("/sim/rendering/clouds3d-density");
  fgUntie("/sim/rendering/clouds3d-cache-size");
  fgUntie("/sim/rendering/clouds3d-cache-resolution");
  fgUntie("/sim/rendering/precipitation-enable");
  fgUntie("/environment/rebuild_layers");
  fgUntie("/sim/rendering/lightning-enable");
  fgUntie("/environment/turbulence/use-cloud-turbulence");
}

void
FGEnvironmentMgr::update (double dt)
{
  SGSubsystemGroup::update(dt);

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
			       * (_environment->get_temperature_degc() + 273.15));
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
  FGEnvironment env = *_environment;
  env.set_elevation_ft(elevation_ft);

  thesky->get_cloud_layer(index)
    ->setElevation_m(elevation_ft * SG_FEET_TO_METER);

  thesky->get_cloud_layer(index)
    ->setSpeed(env.get_wind_speed_kt() * 0.5151);	// 1 kt = 0.5151 m/s

  thesky->get_cloud_layer(index)
    ->setDirection(env.get_wind_from_heading_deg());
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
FGEnvironmentMgr::get_cloud_layer_coverage (int index) const
{
  switch (thesky->get_cloud_layer(index)->getCoverage()) {
  case SGCloudLayer::SG_CLOUD_OVERCAST:
    return "overcast";
  case SGCloudLayer::SG_CLOUD_BROKEN:
    return "broken";
  case SGCloudLayer::SG_CLOUD_SCATTERED:
    return "scattered";
  case SGCloudLayer::SG_CLOUD_FEW:
    return "few";
  case SGCloudLayer::SG_CLOUD_CIRRUS:
    return "cirrus";
  case SGCloudLayer::SG_CLOUD_CLEAR:
    return "clear";
  default:
    return "unknown";
  }
}

void
FGEnvironmentMgr::set_cloud_layer_coverage (int index,
                                            const char * coverage_name)
{
  SGCloudLayer::Coverage coverage;
  if (!strcmp(coverage_name, "overcast"))
    coverage = SGCloudLayer::SG_CLOUD_OVERCAST;
  else if (!strcmp(coverage_name, "broken"))
    coverage = SGCloudLayer::SG_CLOUD_BROKEN;
  else if (!strcmp(coverage_name, "scattered"))
    coverage = SGCloudLayer::SG_CLOUD_SCATTERED;
  else if (!strcmp(coverage_name, "few"))
    coverage = SGCloudLayer::SG_CLOUD_FEW;
  else if (!strcmp(coverage_name, "cirrus"))
    coverage = SGCloudLayer::SG_CLOUD_CIRRUS;
  else if (!strcmp(coverage_name, "clear"))
    coverage = SGCloudLayer::SG_CLOUD_CLEAR;
  else {
    SG_LOG(SG_INPUT, SG_WARN, "Unknown cloud type " << coverage_name);
    coverage = SGCloudLayer::SG_CLOUD_CLEAR;
  }
  thesky->get_cloud_layer(index)->setCoverage(coverage);
}



// end of environment-mgr.cxx
