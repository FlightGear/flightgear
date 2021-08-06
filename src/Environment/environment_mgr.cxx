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
#include <simgear/scene/model/particles.hxx>
#include <simgear/structure/event_mgr.hxx>

#include <Main/main.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/ViewPropertyEvaluator.hxx>

#include <FDM/flight.hxx>

#include "environment.hxx"
#include "environment_mgr.hxx"
#include "environment_ctrl.hxx"
#include "realwx_ctrl.hxx"
#include "fgclouds.hxx"
#include "precipitation_mgr.hxx"
#include "ridge_lift.hxx"
#include "terrainsampler.hxx"
#include "Airports/airport.hxx"
#include "gravity.hxx"
#include "climate.hxx"
#include "magvarmanager.hxx"

#include "AIModel/AINotifications.hxx"

class FG3DCloudsListener : public SGPropertyChangeListener {
public:
  FG3DCloudsListener( FGClouds * fgClouds );
  virtual ~FG3DCloudsListener();

  virtual void valueChanged (SGPropertyNode * node);

private:
  FGClouds * _fgClouds;
  SGPropertyNode_ptr _enableNode;
};

FG3DCloudsListener::FG3DCloudsListener( FGClouds * fgClouds ) :
    _fgClouds( fgClouds )
{
  _enableNode = fgGetNode( "/sim/rendering/clouds3d-enable", true );
  _enableNode->addChangeListener( this );

  valueChanged( _enableNode );
}

FG3DCloudsListener::~FG3DCloudsListener()
{
  _enableNode->removeChangeListener( this );
}

void FG3DCloudsListener::valueChanged( SGPropertyNode * node )
{
  _fgClouds->set_3dClouds( _enableNode->getBoolValue() );
}

FGEnvironmentMgr::FGEnvironmentMgr () :
  _environment(new FGEnvironment()),
  _multiplayerListener(nullptr),
  _sky(globals->get_renderer()->getSky()),
  nearestCarrier(nullptr),
  nearestAirport(nullptr)
{
  fgClouds = new FGClouds;
  _3dCloudsEnableListener = new FG3DCloudsListener(fgClouds);
  set_subsystem("controller", Environment::LayerInterpolateController::createInstance( fgGetNode("/environment/config", true ) ));

  set_subsystem("climate", new FGClimate);
  set_subsystem("precipitation", new FGPrecipitationMgr);
  set_subsystem("realwx", Environment::RealWxController::createInstance( fgGetNode("/environment/realwx", true ) ), 1.0 );
  set_subsystem("terrainsampler", Environment::TerrainSampler::createInstance( fgGetNode("/environment/terrain", true ) ));
  set_subsystem("ridgelift", new FGRidgeLift);

  set_subsystem("magvar", new FGMagVarManager);
  max_tower_height_feet = fgGetDouble("/sim/airport/max-tower-height-ft", 70);
  min_tower_height_feet = fgGetDouble("/sim/airport/min-tower-height-ft", 6);
  default_tower_height_feet = fgGetDouble("default-tower-height-ft", 30);
}

FGEnvironmentMgr::~FGEnvironmentMgr ()
{
  remove_subsystem( "ridgelift" );
  remove_subsystem( "terrainsampler" );
  remove_subsystem("precipitation");
  remove_subsystem("realwx");
  remove_subsystem("controller");
  remove_subsystem("magvar");

  delete fgClouds;
  delete _3dCloudsEnableListener;
  delete _environment;
}

struct FGEnvironmentMgrMultiplayerListener : SGPropertyChangeListener {
    FGEnvironmentMgrMultiplayerListener(FGEnvironmentMgr* environmentmgr)
    :
    _environmentmgr(environmentmgr)
    {
        _node = fgGetNode("/sim/current-view/model-view", true /*create*/);
        _node->addChangeListener(this);
    }
    virtual void valueChanged(SGPropertyNode* node)
    {
        _environmentmgr->updateClosestAirport();
    }
    virtual ~FGEnvironmentMgrMultiplayerListener()
    {
        _node->removeChangeListener(this);
    }
    private:
        FGEnvironmentMgr*   _environmentmgr;
        SGPropertyNode_ptr  _node;
};

SGSubsystem::InitStatus FGEnvironmentMgr::incrementalInit()
{

  InitStatus r = SGSubsystemGroup::incrementalInit();
  if (r == INIT_DONE) {
    fgClouds->Init();
    _multiplayerListener = new FGEnvironmentMgrMultiplayerListener(this);
    globals->get_event_mgr()->addTask("updateClosestAirport", this,
                                      &FGEnvironmentMgr::updateClosestAirport, 10 );
  }

  return r;
}

void
FGEnvironmentMgr::shutdown()
{
  globals->get_event_mgr()->removeTask("updateClosestAirport");
  delete _multiplayerListener;
  _multiplayerListener = nullptr;
  SGSubsystemGroup::shutdown();
}

void
FGEnvironmentMgr::reinit ()
{
  SG_LOG( SG_ENVIRONMENT, SG_INFO, "Reinitializing environment subsystem");
  SGSubsystemGroup::reinit();
}

void
FGEnvironmentMgr::bind ()
{
  SGSubsystemGroup::bind();
  _environment->Tie( fgGetNode("/environment", true ) );

  _tiedProperties.setRoot( fgGetNode( "/environment", true ) );

  _tiedProperties.Tie( "effective-visibility-m", _sky,
          &SGSky::get_visibility );

  _tiedProperties.Tie("rebuild-layers", fgClouds,
          &FGClouds::get_update_event,
          &FGClouds::set_update_event);
//  _tiedProperties.Tie("turbulence/use-cloud-turbulence", &sgEnviro,
//          &SGEnviro::get_turbulence_enable_state,
//          &SGEnviro::set_turbulence_enable_state);

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

  _tiedProperties.Tie( "clouds3d-density", _sky,
          &SGSky::get_3dCloudDensity,
          &SGSky::set_3dCloudDensity);

  _tiedProperties.Tie("clouds3d-vis-range", _sky,
          &SGSky::get_3dCloudVisRange,
          &SGSky::set_3dCloudVisRange);

  _tiedProperties.Tie("clouds3d-impostor-range", _sky,
          &SGSky::get_3dCloudImpostorDistance,
          &SGSky::set_3dCloudImpostorDistance);

  _tiedProperties.Tie("clouds3d-lod1-range", _sky,
          &SGSky::get_3dCloudLoD1Range,
          &SGSky::set_3dCloudLoD1Range);

  _tiedProperties.Tie("clouds3d-lod2-range", _sky,
          &SGSky::get_3dCloudLoD2Range,
          &SGSky::set_3dCloudLoD2Range);

  _tiedProperties.Tie("clouds3d-wrap", _sky,
          &SGSky::get_3dCloudWrap,
          &SGSky::set_3dCloudWrap);

  _tiedProperties.Tie("clouds3d-use-impostors", _sky,
          &SGSky::get_3dCloudUseImpostors,
          &SGSky::set_3dCloudUseImpostors);
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
  SGGeod aircraftPos(globals->get_aircraft_position());

  SGSubsystemGroup::update(dt);

  _environment->set_elevation_ft( aircraftPos.getElevationFt() );

  auto particlesManager = simgear::ParticlesGlobalManager::instance();
  particlesManager->setWindFrom(_environment->get_wind_from_heading_deg(),
                                _environment->get_wind_speed_kt());
  particlesManager->update(dt, globals->get_aircraft_position());

  if( _cloudLayersDirty ) {
    _cloudLayersDirty = false;
    fgClouds->set_update_event( fgClouds->get_update_event()+1 );
  }
  updateTowerPosition();
  
  fgSetDouble( "/environment/gravitational-acceleration-mps2",
    Environment::Gravity::instance()->getGravity(aircraftPos));
}

void FGEnvironmentMgr::updateTowerPosition()
{
    if (towerViewPositionLatDegNode != nullptr && towerViewPositionLonDegNode != nullptr && towerViewPositionAltFtNode != nullptr) {
        auto automaticTowerActive = fgGetBool("/sim/tower/auto-position", true);

        fgSetDouble("/sim/airport/nearest-tower-latitude-deg",  towerViewPositionLatDegNode->getDoubleValue());
        fgSetDouble("/sim/airport/nearest-tower-longitude-deg", towerViewPositionLonDegNode->getDoubleValue());
        fgSetDouble("/sim/airport/nearest-tower-altitude-ft",   towerViewPositionAltFtNode->getDoubleValue());

        if (automaticTowerActive) {
            fgSetDouble("/sim/tower/latitude-deg", towerViewPositionLatDegNode->getDoubleValue());
            fgSetDouble("/sim/tower/longitude-deg", towerViewPositionLonDegNode->getDoubleValue());
            fgSetDouble("/sim/tower/altitude-ft", towerViewPositionAltFtNode->getDoubleValue());
        }
    }
}

void FGEnvironmentMgr::updateClosestAirport()
{
    SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "FGEnvironmentMgr::update: updating closest airport");

    SGGeod pos = globals->get_aircraft_position();

    //
    // If we are viewing a multiplayer aircraft, find nearest airport so that
    // Tower View etc works.
    std::string view_config_root = ViewPropertyEvaluator::getStringValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/root)");

    if (view_config_root != "/" && view_config_root != "") {
        /* We are currently viewing a multiplayer aircraft. */
        pos = SGGeod::fromDegFt(
            ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/position/longitude-deg)"),
            ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/position/latitude-deg)"),
            ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/position/altitude-ft)"));
    }

    // nearest tower logic;
    // 1. find nearest airport
    // 2. find nearest carrier
    // - select the nearest one as the tower.

    nearestAirport = FGAirport::findClosest(pos, 100.0);
    auto automaticTowerActive = fgGetBool("/sim/tower/auto-position", true);

    SGGeod nearestTowerPosition;
    bool nearestTowerPositionValid = false; /* <nearestTowerPositionValid> is set but unused. */
    std::string nearestIdent;
    const SGGeod airportGeod;
    double towerDistance = numeric_limits<double>::max();
    if (nearestAirport) {
        const string currentId = fgGetString("/sim/airport/closest-airport-id", "");
        if (currentId != nearestAirport->ident()) {
            SG_LOG(SG_ENVIRONMENT, SG_INFO, "FGEnvironmentMgr::updateClosestAirport: selected:" << nearestAirport->ident());
            fgSetString("/sim/airport/closest-airport-id", nearestAirport->ident().c_str());
        }

        if (nearestAirport->hasTower()) {
            nearestTowerPosition = nearestAirport->getTowerLocation();
            SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "airport-id=" << nearestAirport->getId() << " tower_pos=" << nearestTowerPosition);
        }
        else {
            nearestTowerPosition = nearestAirport->geod();
            SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "no tower for airport-id=" << nearestAirport->getId());
        }
        //Ensure that the tower isn't at ground level by adding a nominal amount
        //TODO: (fix the data so that too short or too tall towers aren't present in the data)
        auto towerAirpotDistance = abs(nearestTowerPosition.getElevationFt() - nearestAirport->geod().getElevationFt());
        if (towerAirpotDistance < min_tower_height_feet) {
            nearestTowerPosition.setElevationFt(nearestTowerPosition.getElevationFt() + default_tower_height_feet);
            SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Tower altitude adjusted because it was at below minimum height above ground (" << min_tower_height_feet << "feet) for airport " << nearestAirport->getId());
        }
        else if (towerAirpotDistance > max_tower_height_feet) {
            nearestTowerPosition.setElevationFt(nearestTowerPosition.getElevationFt() + default_tower_height_feet);
            SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Tower altitude adjusted because it was taller than the permitted maximum of (" << max_tower_height_feet << "feet) for airport " << nearestAirport->getId());
        }
        //
        nearestIdent = nearestAirport->ident();
        nearestTowerPositionValid = true;

        towerDistance = SGGeodesy::distanceM(nearestTowerPosition, pos);
        
        // when the tower doesn't move we can clear these. 
        // if the carrier is nearer these variables will be set in that logic.
        towerViewPositionLatDegNode = towerViewPositionLonDegNode = towerViewPositionAltFtNode = nullptr;
    }
    else {
        SG_LOG(SG_ENVIRONMENT, SG_INFO, "FGEnvironmentMgr::update: No airport within 100NM range");
    }
    auto nctn = SGSharedPtr< NearestCarrierToNotification> (new NearestCarrierToNotification(pos));
    if (simgear::Emesary::ReceiptStatus::OK == simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(nctn)) {
        if (nearestCarrier != nctn->GetCarrier()) {
            SG_LOG(SG_ENVIRONMENT, SG_INFO, "Nearest carrier " << nctn->GetCarrierIdent() << " at a distance of " << nctn->GetDistanceMeters());
            nearestCarrier = nctn->GetCarrier();
            fgSetString("/sim/airport/nearest-carrier", nctn->GetCarrierIdent());
        }
    } else {
        SG_LOG(SG_ENVIRONMENT, SG_INFO, "No carriers found");
        fgSetString("/sim/airport/nearest-carrier", "");
        fgSetDouble("/sim/airport/nearest-carrier-latitude-deg", 0);
        fgSetDouble("/sim/airport/nearest-carrier-longitude-deg", 0);
        fgSetDouble("/sim/airport/nearest-carrier-altitude-ft", 0);
        fgSetDouble("/sim/airport/nearest-carrier-deck-height", 0);
        nearestCarrier = nullptr;
    }

    // figure out if the carrier's tower is closer
    if (nearestCarrier && (nctn->GetDistanceMeters() < towerDistance)) {
        nearestIdent = nctn->GetCarrierIdent();
        nearestTowerPositionValid = true;

        //
        // these will be used to determine and update the tower position
        towerViewPositionLatDegNode = nctn->GetViewPositionLatNode();
        towerViewPositionLonDegNode = nctn->GetViewPositionLonNode();
        towerViewPositionAltFtNode = nctn->GetViewPositionAltNode();

        // although the carrier is moving - these values can afford to be 10 seconds old so we don't need to
        // update them.
        fgSetDouble("/sim/airport/nearest-carrier-latitude-deg", nctn->GetPosition()->getLatitudeDeg());
        fgSetDouble("/sim/airport/nearest-carrier-longitude-deg", nctn->GetPosition()->getLongitudeDeg());
        fgSetDouble("/sim/airport/nearest-carrier-altitude-ft", nctn->GetPosition()->getElevationFt());
        fgSetDouble("/sim/airport/nearest-carrier-deck-height", nctn->GetDeckheight());
    } else {
        if (nearestAirport != nullptr) {
            std::string   path = ViewPropertyEvaluator::getStringValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/root)/sim/tower/");
            fgSetString(path + "airport-id", nearestAirport->getId());

            fgSetDouble(path + "latitude-deg", nearestTowerPosition.getLatitudeDeg());
            fgSetDouble(path + "longitude-deg", nearestTowerPosition.getLongitudeDeg());
            fgSetDouble(path + "altitude-ft", nearestTowerPosition.getElevationFt());
        }
        else {
            SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "FGEnvironmentMgr::update: No airport or carrier within 100NM range of current multiplayer aircraft");
        }
    }


    if (fgGetString("/sim/airport/nearest-tower-ident") != nearestIdent) {
        SG_LOG(SG_ENVIRONMENT, SG_INFO, "Nearest airport tower now " << nearestIdent);
        fgSetString("/sim/airport/nearest-tower-ident", nearestIdent);
    }
    if (automaticTowerActive) {
        if (fgGetString("/sim/tower/airport-id") != nearestIdent) {
            fgSetString("/sim/tower/airport-id", nearestIdent);
            SG_LOG(SG_ENVIRONMENT, SG_INFO, "Auto Tower: now " << nearestIdent);
        }
    }
    updateTowerPosition();
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
  return _sky->get_cloud_layer(index)->getSpan_m();
}

void
FGEnvironmentMgr::set_cloud_layer_span_m (int index, double span_m)
{
  _sky->get_cloud_layer(index)->setSpan_m(span_m);
}

double
FGEnvironmentMgr::get_cloud_layer_elevation_ft (int index) const
{
  return _sky->get_cloud_layer(index)->getElevation_m() * SG_METER_TO_FEET;
}

void
FGEnvironmentMgr::set_cloud_layer_elevation_ft (int index, double elevation_ft)
{
  FGEnvironment env = *_environment;
  env.set_elevation_ft(elevation_ft);

  _sky->get_cloud_layer(index)
    ->setElevation_m(elevation_ft * SG_FEET_TO_METER);

  _sky->get_cloud_layer(index)
    ->setSpeed(env.get_wind_speed_kt() * 0.5151);	// 1 kt = 0.5151 m/s

  _sky->get_cloud_layer(index)
    ->setDirection(env.get_wind_from_heading_deg());
}

double
FGEnvironmentMgr::get_cloud_layer_thickness_ft (int index) const
{
  return _sky->get_cloud_layer(index)->getThickness_m() * SG_METER_TO_FEET;
}

void
FGEnvironmentMgr::set_cloud_layer_thickness_ft (int index, double thickness_ft)
{
  _sky->get_cloud_layer(index)
    ->setThickness_m(thickness_ft * SG_FEET_TO_METER);
}

double
FGEnvironmentMgr::get_cloud_layer_transition_ft (int index) const
{
  return _sky->get_cloud_layer(index)->getTransition_m() * SG_METER_TO_FEET;
}

void
FGEnvironmentMgr::set_cloud_layer_transition_ft (int index,
						 double transition_ft)
{
  _sky->get_cloud_layer(index)
    ->setTransition_m(transition_ft * SG_FEET_TO_METER);
}

const char *
FGEnvironmentMgr::get_cloud_layer_coverage (int index) const
{
  return _sky->get_cloud_layer(index)->getCoverageString().c_str();
}

void
FGEnvironmentMgr::set_cloud_layer_coverage (int index,
                                            const char * coverage_name)
{
  if( _sky->get_cloud_layer(index)->getCoverageString() == coverage_name )
    return;

  _sky->get_cloud_layer(index)->setCoverageString(coverage_name);
  _cloudLayersDirty = true;
}

int
FGEnvironmentMgr::get_cloud_layer_coverage_type (int index) const
{
  return _sky->get_cloud_layer(index)->getCoverage();
}

double
FGEnvironmentMgr::get_cloud_layer_visibility_m (int index) const
{
    return _sky->get_cloud_layer(index)->getVisibility_m();
}

void
FGEnvironmentMgr::set_cloud_layer_visibility_m (int index, double visibility_m)
{
    _sky->get_cloud_layer(index)->setVisibility_m(visibility_m);
}

double
FGEnvironmentMgr::get_cloud_layer_maxalpha (int index ) const
{
    return _sky->get_cloud_layer(index)->getMaxAlpha();
}

void
FGEnvironmentMgr::set_cloud_layer_maxalpha (int index, double maxalpha)
{
    _sky->get_cloud_layer(index)->setMaxAlpha(maxalpha);
}

void
FGEnvironmentMgr::set_cloud_layer_coverage_type (int index, int type )
{
  if( type < 0 || type >= SGCloudLayer::SG_MAX_CLOUD_COVERAGES ) {
    SG_LOG(SG_ENVIRONMENT,SG_WARN,"Unknown cloud layer type " << type << " ignored" );
    return;
  }

  if( static_cast<SGCloudLayer::Coverage>(type) == _sky->get_cloud_layer(index)->getCoverage() )
    return;

  _sky->get_cloud_layer(index)->setCoverage(static_cast<SGCloudLayer::Coverage>(type));
  _cloudLayersDirty = true;
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGEnvironmentMgr> registrantFGEnvironmentMgr;

// end of environment-mgr.cxx
