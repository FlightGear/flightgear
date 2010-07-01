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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/environment/visual_enviro.hxx>
#include <simgear/scene/model/particles.hxx>

#include <Main/main.hxx>
#include <Main/fg_props.hxx>
#include <FDM/flight.hxx>

#include "environment.hxx"
#include "environment_mgr.hxx"
#include "environment_ctrl.hxx"
#include "fgclouds.hxx"
#include "precipitation_mgr.hxx"

class SGSky;
extern SGSky *thesky;



FGEnvironmentMgr::FGEnvironmentMgr ()
  : _environment(new FGEnvironment)
{

  _controller = new FGInterpolateEnvironmentCtrl;
  _controller->setEnvironment(_environment);
  set_subsystem("controller", _controller, 0.1 );

  fgClouds = new FGClouds();

  _metarcontroller = new FGMetarCtrl(_controller );
  set_subsystem("metarcontroller", _metarcontroller, 0.1 );

  _metarfetcher = new FGMetarFetcher();
  set_subsystem("metarfetcher", _metarfetcher, 1.0 );

  _precipitationManager = new FGPrecipitationMgr;
  set_subsystem("precipitation", _precipitationManager);
}

FGEnvironmentMgr::~FGEnvironmentMgr ()
{
  remove_subsystem("precipitation");
  delete _precipitationManager;

  remove_subsystem("metarcontroller");
  delete _metarfetcher;

  remove_subsystem("metarfetcher");
  delete _metarcontroller;

  delete fgClouds;

  remove_subsystem("controller");
  delete _controller;

  delete _environment;
}

void
FGEnvironmentMgr::init ()
{
  SG_LOG( SG_GENERAL, SG_INFO, "Initializing environment subsystem");
  SGSubsystemGroup::init();
  //_update_fdm();
}

void
FGEnvironmentMgr::reinit ()
{
  SG_LOG( SG_GENERAL, SG_INFO, "Reinitializing environment subsystem");
  SGSubsystemGroup::reinit();
  //_update_fdm();
}

void
FGEnvironmentMgr::bind ()
{
  SGSubsystemGroup::bind();
  fgTie("/environment/visibility-m", _environment,
	&FGEnvironment::get_visibility_m, &FGEnvironment::set_visibility_m);
  fgSetArchivable("/environment/visibility-m");
  fgTie("/environment/effective-visibility-m", thesky,
       &SGSky::get_visibility );
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
  fgTie("/environment/relative-humidity", _environment,
	&FGEnvironment::get_relative_humidity); //ro
  fgTie("/environment/atmosphere/density-tropo-avg", _environment,
	&FGEnvironment::get_density_tropo_avg_kgm3); //ro
  fgTie("/environment/atmosphere/altitude-half-to-sun", _environment,
	&FGEnvironment::get_altitude_half_to_sun_m, 
	&FGEnvironment::set_altitude_half_to_sun_m);
  fgTie("/environment/atmosphere/altitude-troposphere-top", _environment,
        &FGEnvironment::get_altitude_tropo_top_m,
        &FGEnvironment::set_altitude_tropo_top_m);
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

  fgTie("/environment/thermal-lift-fps", _environment,
	&FGEnvironment::get_thermal_lift_fps,
	&FGEnvironment::set_thermal_lift_fps);
  fgSetArchivable("/environment/thermal-lift-fps");
  fgTie("/environment/ridge-lift-fps", _environment,
	&FGEnvironment::get_ridge_lift_fps,
	&FGEnvironment::set_ridge_lift_fps);	
  fgSetArchivable("/environment/ridge-lift-fps");
  
    fgTie("/environment/local-weather-lift", _environment,
	&FGEnvironment::get_local_weather_lift_fps); //read-only
     
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

  fgTie("/environment/metar/data", _metarcontroller,
          &FGMetarCtrl::get_metar, &FGMetarCtrl::set_metar );

  fgTie("/sim/rendering/clouds3d-enable", fgClouds,
	  &FGClouds::get_3dClouds,
	  &FGClouds::set_3dClouds);
  fgTie("/sim/rendering/clouds3d-density", thesky,
	  &SGSky::get_3dCloudDensity,
	  &SGSky::set_3dCloudDensity);
  fgTie("/sim/rendering/clouds3d-vis-range", thesky,
        &SGSky::get_3dCloudVisRange,
        &SGSky::set_3dCloudVisRange);
  
  fgTie("/sim/rendering/precipitation-enable", &sgEnviro,
	  &SGEnviro::get_precipitation_enable_state, 
	  &SGEnviro::set_precipitation_enable_state);
  fgTie("/environment/rebuild-layers", fgClouds,
      &FGClouds::get_update_event,
      &FGClouds::set_update_event);
  fgTie("/sim/rendering/lightning-enable", &sgEnviro,
      &SGEnviro::get_lightning_enable_state,
      &SGEnviro::set_lightning_enable_state);
  fgTie("/environment/turbulence/use-cloud-turbulence", &sgEnviro,
      &SGEnviro::get_turbulence_enable_state,
      &SGEnviro::set_turbulence_enable_state);
  sgEnviro.config(fgGetNode("/sim/rendering/precipitation"));
}

void
FGEnvironmentMgr::unbind ()
{
  fgUntie("/environment/visibility-m");
  fgUntie("/environment/effective-visibility-m");
  fgUntie("/environment/temperature-sea-level-degc");
  fgUntie("/environment/temperature-degc");
  fgUntie("/environment/dewpoint-sea-level-degc");
  fgUntie("/environment/dewpoint-degc");
  fgUntie("/environment/pressure-sea-level-inhg");
  fgUntie("/environment/pressure-inhg");
  fgUntie("/environment/density-inhg");
  fgUntie("/environment/relative-humidity");
  fgUntie("/environment/atmosphere/density-tropo-avg");
  fgUntie("/environment/wind-from-north-fps");
  fgUntie("/environment/wind-from-east-fps");
  fgUntie("/environment/wind-from-down-fps");

  fgUntie("/environment/thermal-lift-fps");
  fgUntie("/environment/ridge-lift-fps");
  fgUntie("/environment/local-weather-lift");

  fgUntie("/environment/atmosphere/altitude-half-to-sun");
  fgUntie("/environment/atmosphere/altitude-troposphere-top");
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
  fgUntie("/sim/rendering/precipitation-enable");
  fgUntie("/environment/rebuild-layers");
  fgUntie("/environment/weather-scenario");
  fgUntie("/sim/rendering/lightning-enable");
  fgUntie("/environment/turbulence/use-cloud-turbulence");
  SGSubsystemGroup::unbind();
}

void
FGEnvironmentMgr::update (double dt)
{
  SGSubsystemGroup::update(dt);
  
  _environment->set_elevation_ft(fgGetDouble("/position/altitude-ft"));
  _environment->set_local_weather_lift_fps(fgGetDouble("/local-weather/current/thermal-lift"));
  osg::Vec3 windVec(-_environment->get_wind_from_north_fps(),
                    -_environment->get_wind_from_east_fps(),
                    _environment->get_wind_from_down_fps());
  simgear::Particles::setWindVector(windVec * SG_FEET_TO_METER);
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

FGEnvironment
FGEnvironmentMgr::getEnvironment(const SGGeod& aPos) const
{
  // Always returns the same environment
  // for now; we'll make it interesting
  // later.
  FGEnvironment env = *_environment;
  env.set_elevation_ft(aPos.getElevationFt());
  return env;

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
