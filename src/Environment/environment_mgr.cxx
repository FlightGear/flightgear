/*
 * SPDX-FileName: environment_mgr.cxx
 * SPDX-FileComment: manager for natural environment information
 * SPDX-FileCopyrightText: Copyright (C) 2002  David Megginson - david@megginson.com
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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

FGEnvironmentMgr::FGEnvironmentMgr() : _environment(new FGEnvironment()),
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


struct FGEnvironmentMgrPropertyListener : SGPropertyChangeListener {
    FGEnvironmentMgrPropertyListener(FGEnvironmentMgr* environmentmgr) : _environmentmgr(environmentmgr)
    {
        _modelViewNode = fgGetNode("/sim/current-view/model-view", true /*create*/);
        _modelViewNode->addChangeListener(this);
        _viewNumberNode = fgGetNode("/sim/current-view/view-number-raw", true /*create*/);
        _viewNumberNode->addChangeListener(this);
        _airportIdNode = fgGetNode("/sim/tower/airport-id", true /*create*/);
        _airportIdNode->addChangeListener(this);
        _autoTowerNode = fgGetNode("/sim/tower/auto-position", true /*create*/);
        _autoTowerNode->addChangeListener(this);
    }

    void valueChanged(SGPropertyNode* node) override
    {
        if ((node == _modelViewNode) || (_autoTowerNode == node)) {
            _environmentmgr->updateClosestAirport();
        }

        if ((node == _viewNumberNode) || (node == _airportIdNode)) {
            _environmentmgr->onTowerAirportIDChanged();
        }
    }
    virtual ~FGEnvironmentMgrPropertyListener()
    {
        _modelViewNode->removeChangeListener(this);
        _viewNumberNode->removeChangeListener(this);
        _airportIdNode->removeChangeListener(this);
        _airportIdNode->removeChangeListener(this);
    }
    private:
        FGEnvironmentMgr*   _environmentmgr;
        SGPropertyNode_ptr _modelViewNode;
        SGPropertyNode_ptr _viewNumberNode;
        SGPropertyNode_ptr _airportIdNode;
        SGPropertyNode_ptr _autoTowerNode;
};


SGSubsystem::InitStatus FGEnvironmentMgr::incrementalInit()
{

  InitStatus r = SGSubsystemGroup::incrementalInit();
  if (r == INIT_DONE) {
    fgClouds->Init();
    _listener.reset(new FGEnvironmentMgrPropertyListener(this));
    globals->get_event_mgr()->addTask("updateClosestAirport",
        [this](){ this->updateClosestAirport(); }, 10 );
  }

  return r;
}

void
FGEnvironmentMgr::shutdown()
{
  globals->get_event_mgr()->removeTask("updateClosestAirport");
  _listener.reset();
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

    _automaticTowerEnableNode = fgGetNode("/sim/tower/auto-position", true);
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
  updateDynamicTowerPosition();

  fgSetDouble( "/environment/gravitational-acceleration-mps2",
    Environment::Gravity::instance()->getGravity(aircraftPos));
}

void FGEnvironmentMgr::onTowerAirportIDChanged()
{
    FGAirportRef apt;
    const auto automaticTowerActive = _automaticTowerEnableNode->getBoolValue();

    if (automaticTowerActive) {
        apt = FGAirport::findByIdent(fgGetString("/sim/airport/closest-airport-id"));
    } else {
        apt = FGAirport::findByIdent(fgGetString("/sim/tower/airport-id"));
    }

    if (!apt) {
        return;
    }

    SGGeod towerPos;
    if (apt->hasTower()) {
        towerPos = apt->getTowerLocation();
        SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "airport-id=" << apt->getId() << " tower_pos=" << towerPos);
    } else {
        towerPos = apt->geod();
        SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "no tower for airport-id=" << apt->getId());
    }
    //Ensure that the tower isn't at ground level by adding a nominal amount
    // TODO: (fix the data so that too short or too tall towers aren't present in the data)
    auto towerAirpotDistance = abs(towerPos.getElevationFt() - apt->geod().getElevationFt());
    if (towerAirpotDistance < min_tower_height_feet) {
        towerPos.setElevationFt(towerPos.getElevationFt() + default_tower_height_feet);
        SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Tower altitude adjusted because it was at below minimum height above ground (" << min_tower_height_feet << "feet) for airport " << apt->getId());
    } else if (towerAirpotDistance > max_tower_height_feet) {
        towerPos.setElevationFt(towerPos.getElevationFt() + default_tower_height_feet);
        SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Tower altitude adjusted because it was taller than the permitted maximum of (" << max_tower_height_feet << "feet) for airport " << apt->getId());
    }

    std::string path = ViewPropertyEvaluator::getStringValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/root)/sim/tower/");

    fgSetDouble(path + "latitude-deg", towerPos.getLatitudeDeg());
    fgSetDouble(path + "longitude-deg", towerPos.getLongitudeDeg());
    fgSetDouble(path + "altitude-ft", towerPos.getElevationFt());
}

void FGEnvironmentMgr::updateDynamicTowerPosition()
{
    if (towerViewPositionLatDegNode != nullptr && towerViewPositionLonDegNode != nullptr && towerViewPositionAltFtNode != nullptr) {
        const auto automaticTowerActive = _automaticTowerEnableNode->getBoolValue();

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

    auto nearestAirport = FGAirport::findClosest(pos, 100.0);
    const auto automaticTowerActive = _automaticTowerEnableNode->getBoolValue();

    SGGeod nearestTowerPosition;
    std::string nearestIdent;
    double towerDistance = std::numeric_limits<double>::max();
    if (nearestAirport) {
        const std::string currentId = fgGetString("/sim/airport/closest-airport-id", "");
        if (currentId != nearestAirport->ident()) {
            SG_LOG(SG_ENVIRONMENT, SG_INFO, "FGEnvironmentMgr::updateClosestAirport: selected:" << nearestAirport->ident());
            fgSetString("/sim/airport/closest-airport-id", nearestAirport->ident().c_str());
        }

        nearestTowerPosition = nearestAirport->geod();
        nearestIdent = nearestAirport->ident();
        towerDistance = SGGeodesy::distanceM(nearestTowerPosition, pos);

        // clear these so we don't do dynamic updates unless a carrier is active
        towerViewPositionLatDegNode = towerViewPositionLonDegNode = towerViewPositionAltFtNode = nullptr;
    }
    else {
        SG_LOG(SG_ENVIRONMENT, SG_INFO, "FGEnvironmentMgr::update: No airport within 100NM range");
    }

    // check for closer carrier
    auto nctn = SGSharedPtr< NearestCarrierToNotification> (new NearestCarrierToNotification(pos));
    if (simgear::Emesary::ReceiptStatus::OK == simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(nctn)) {
        if (nearestCarrier != nctn->GetCarrier()) {
            nearestCarrier = nctn->GetCarrier();
            fgSetString("/sim/airport/nearest-carrier", nctn->GetCarrierIdent());
        }
    } else {
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

        std::string path = ViewPropertyEvaluator::getStringValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/root)/sim/tower/");
        auto currentViewAirportIdNode = fgGetNode(path + "airport-id", true);
        const auto curId = currentViewAirportIdNode->getStringValue();
        if (curId != nearestIdent) {
          currentViewAirportIdNode->setStringValue(nearestIdent);
        }
    }

    updateDynamicTowerPosition();
}


FGEnvironment
FGEnvironmentMgr::getEnvironment () const
{
  return *_environment;
}

const FGEnvironment* FGEnvironmentMgr::getAircraftEnvironment() const
{
    return _environment;
}

FGEnvironment
FGEnvironmentMgr::getEnvironmentAtPosition(const SGGeod& aPos) const
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
