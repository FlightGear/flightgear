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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstring>

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
#include "realwx_ctrl.hxx"
#include "fgclouds.hxx"
#include "precipitation_mgr.hxx"
#include "ridge_lift.hxx"
#include "terrainsampler.hxx"

class SGSky;
extern SGSky *thesky;

FGEnvironmentMgr::FGEnvironmentMgr () :
  _environment(new FGEnvironment()),
  fgClouds(new FGClouds()),
  _altitudeNode(fgGetNode("/position/altitude-ft", true)),
  _cloudLayersDirty(true)
{
  set_subsystem("controller", Environment::LayerInterpolateController::createInstance( fgGetNode("/environment/config", true ) ));
  set_subsystem("realwx", Environment::RealWxController::createInstance( fgGetNode("/environment/realwx", true ) ), 1.0 );

  set_subsystem("precipitation", new FGPrecipitationMgr);
  set_subsystem("terrainsampler", Environment::TerrainSampler::createInstance( fgGetNode("/environment/terrain", true ) ));
  set_subsystem("ridgelift", new FGRidgeLift);
}

FGEnvironmentMgr::~FGEnvironmentMgr ()
{
  SGSubsystem * subsys;

  subsys = get_subsystem( "ridgelift" );
  remove_subsystem( "ridgelift" );
  delete subsys;

  subsys = get_subsystem( "terrainsampler" );
  remove_subsystem( "terrainsampler" );
  delete subsys;

  subsys = get_subsystem( "precipitation" );
  remove_subsystem("precipitation");
  delete subsys;

  subsys = get_subsystem("realwx");
  remove_subsystem("realwx");
  delete subsys;

  subsys = get_subsystem("controller");
  remove_subsystem("controller");
  delete subsys;

  delete fgClouds;
  delete _environment;
}

void
FGEnvironmentMgr::init ()
{
  SG_LOG( SG_GENERAL, SG_INFO, "Initializing environment subsystem");
  SGSubsystemGroup::init();
}

void
FGEnvironmentMgr::reinit ()
{
  SG_LOG( SG_GENERAL, SG_INFO, "Reinitializing environment subsystem");
  SGSubsystemGroup::reinit();
}

void
FGEnvironmentMgr::bind ()
{
  SGSubsystemGroup::bind();
  _environment->Tie( fgGetNode("/environment", true ) );
  
  _tiedProperties.setRoot( fgGetNode( "/environment", true ) );

  _tiedProperties.Tie( "effective-visibility-m", thesky,
       &SGSky::get_visibility );

  _tiedProperties.Tie("rebuild-layers", fgClouds,
      &FGClouds::get_update_event,
      &FGClouds::set_update_event);

  _tiedProperties.Tie("turbulence/use-cloud-turbulence", &sgEnviro,
      &SGEnviro::get_turbulence_enable_state,
      &SGEnviro::set_turbulence_enable_state);

  for (int i = 0; i < MAX_CLOUD_LAYERS; i++) {
    SGPropertyNode_ptr layerNode = fgGetNode("/environment/clouds",true)->getChild("layer", i, true );

    _tiedProperties.Tie( layerNode->getNode("span-m",true), this, i, 
	  &FGEnvironmentMgr::get_cloud_layer_span_m,
	  &FGEnvironmentMgr::set_cloud_layer_span_m);

    _tiedProperties.Tie( layerNode->getNode("elevation-ft",true), this, i, 
	  &FGEnvironmentMgr::get_cloud_layer_elevation_ft,
	  &FGEnvironmentMgr::set_cloud_layer_elevation_ft);

    _tiedProperties.Tie( layerNode->getNode("thickness-ft",true), this, i, 
	  &FGEnvironmentMgr::get_cloud_layer_thickness_ft,
	  &FGEnvironmentMgr::set_cloud_layer_thickness_ft);

    _tiedProperties.Tie( layerNode->getNode("transition-ft",true), this, i, 
	  &FGEnvironmentMgr::get_cloud_layer_transition_ft,
	  &FGEnvironmentMgr::set_cloud_layer_transition_ft);

    _tiedProperties.Tie( layerNode->getNode("coverage",true), this, i, 
	  &FGEnvironmentMgr::get_cloud_layer_coverage,
	  &FGEnvironmentMgr::set_cloud_layer_coverage);

    _tiedProperties.Tie( layerNode->getNode("coverage-type",true), this, i, 
	  &FGEnvironmentMgr::get_cloud_layer_coverage_type,
	  &FGEnvironmentMgr::set_cloud_layer_coverage_type);

    _tiedProperties.Tie( layerNode->getNode( "visibility-m",true), this, i,
        &FGEnvironmentMgr::get_cloud_layer_visibility_m,
        &FGEnvironmentMgr::set_cloud_layer_visibility_m);

    _tiedProperties.Tie( layerNode->getNode( "alpha",true), this, i,
        &FGEnvironmentMgr::get_cloud_layer_maxalpha,
        &FGEnvironmentMgr::set_cloud_layer_maxalpha);
  }

  _tiedProperties.setRoot( fgGetNode("/sim/rendering", true ) );

  _tiedProperties.Tie( "clouds3d-enable", fgClouds,
	  &FGClouds::get_3dClouds,
	  &FGClouds::set_3dClouds);

  _tiedProperties.Tie( "clouds3d-density", thesky,
	  &SGSky::get_3dCloudDensity,
	  &SGSky::set_3dCloudDensity);

  _tiedProperties.Tie("clouds3d-vis-range", thesky,
        &SGSky::get_3dCloudVisRange,
        &SGSky::set_3dCloudVisRange);
  
  _tiedProperties.Tie("precipitation-enable", &sgEnviro,
	  &SGEnviro::get_precipitation_enable_state, 
	  &SGEnviro::set_precipitation_enable_state);

  _tiedProperties.Tie("lightning-enable", &sgEnviro,
      &SGEnviro::get_lightning_enable_state,
      &SGEnviro::set_lightning_enable_state);

  sgEnviro.config(fgGetNode("/sim/rendering/precipitation"));
}

void
FGEnvironmentMgr::unbind ()
{
  _tiedProperties.Untie();
  _environment->Untie();
  SGSubsystemGroup::unbind();
}

void
FGEnvironmentMgr::update (double dt)
{
  SGSubsystemGroup::update(dt);
  
  _environment->set_elevation_ft( _altitudeNode->getDoubleValue() );

  simgear::Particles::setWindFrom( _environment->get_wind_from_heading_deg(),
				   _environment->get_wind_speed_kt() );
  if( _cloudLayersDirty ) {
    _cloudLayersDirty = false;
    fgClouds->set_update_event( fgClouds->get_update_event()+1 );
  }
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
  return thesky->get_cloud_layer(index)->getCoverageString().c_str();
}

void
FGEnvironmentMgr::set_cloud_layer_coverage (int index,
                                            const char * coverage_name)
{
  if( thesky->get_cloud_layer(index)->getCoverageString() == coverage_name )
    return;

  thesky->get_cloud_layer(index)->setCoverageString(coverage_name);
  _cloudLayersDirty = true;
}

int
FGEnvironmentMgr::get_cloud_layer_coverage_type (int index) const
{
  return thesky->get_cloud_layer(index)->getCoverage();
}

double 
FGEnvironmentMgr::get_cloud_layer_visibility_m (int index) const
{
    return thesky->get_cloud_layer(index)->getVisibility_m();
}

void 
FGEnvironmentMgr::set_cloud_layer_visibility_m (int index, double visibility_m)
{
    thesky->get_cloud_layer(index)->setVisibility_m(visibility_m);
}

double 
FGEnvironmentMgr::get_cloud_layer_maxalpha (int index ) const
{
    return thesky->get_cloud_layer(index)->getMaxAlpha();
}

void 
FGEnvironmentMgr::set_cloud_layer_maxalpha (int index, double maxalpha)
{
    thesky->get_cloud_layer(index)->setMaxAlpha(maxalpha);
}

void 
FGEnvironmentMgr::set_cloud_layer_coverage_type (int index, int type )
{
  if( type < 0 || type >= SGCloudLayer::SG_MAX_CLOUD_COVERAGES ) {
    SG_LOG(SG_ALL,SG_WARN,"Unknown cloud layer type " << type << " ignored" );
    return;
  }

  if( static_cast<SGCloudLayer::Coverage>(type) == thesky->get_cloud_layer(index)->getCoverage() )
    return;

  thesky->get_cloud_layer(index)->setCoverage(static_cast<SGCloudLayer::Coverage>(type));
  _cloudLayersDirty = true;
}


// end of environment-mgr.cxx
