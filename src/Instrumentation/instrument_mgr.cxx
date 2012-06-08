// instrument_mgr.cxx - manage aircraft instruments.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <string>
#include <sstream>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Instrumentation/HUD/HUD.hxx>

#include "instrument_mgr.hxx"
#include "adf.hxx"
#include "airspeed_indicator.hxx"
#include "altimeter.hxx"
#include "attitude_indicator.hxx"
#include "clock.hxx"
#include "dme.hxx"
#include "gps.hxx"
#include "gsdi.hxx"
#include "heading_indicator.hxx"
#include "heading_indicator_fg.hxx"
#include "heading_indicator_dg.hxx"
#include "kr_87.hxx"
#include "kt_70.hxx"
#include "mag_compass.hxx"
#include "marker_beacon.hxx"
#include "newnavradio.hxx"
#include "slip_skid_ball.hxx"
#include "transponder.hxx"
#include "turn_indicator.hxx"
#include "vertical_speed_indicator.hxx"
#include "inst_vertical_speed_indicator.hxx"
#include "wxradar.hxx"
#include "tacan.hxx"
#include "mk_viii.hxx"
#include "mrg.hxx"
#include "groundradar.hxx"
#include "agradar.hxx"
#include "rad_alt.hxx"
#include "tcas.hxx"
#include "NavDisplay.hxx"

FGInstrumentMgr::FGInstrumentMgr () :
  _explicitGps(false)
{    
    globals->add_subsystem("hud", new HUD, SGSubsystemMgr::DISPLAY);
}

FGInstrumentMgr::~FGInstrumentMgr ()
{
}

void FGInstrumentMgr::init()
{
  SGPropertyNode_ptr config_props = new SGPropertyNode;
  SGPropertyNode* path_n = fgGetNode("/sim/instrumentation/path");
  if (!path_n) {
    SG_LOG(SG_COCKPIT, SG_WARN, "No instrumentation model specified for this model!");
    return;
  }

  SGPath config = globals->resolve_aircraft_path(path_n->getStringValue());
  SG_LOG( SG_COCKPIT, SG_INFO, "Reading instruments from " << config.str() );

  try {
    readProperties( config.str(), config_props );
    if (!build(config_props)) {
      throw sg_exception(
                    "Detected an internal inconsistency in the instrumentation\n"
                    "system specification file.  See earlier errors for details.");
    }
  } catch (const sg_exception& e) {
    SG_LOG(SG_COCKPIT, SG_ALERT, "Failed to load instrumentation system model: "
                    << config.str() << ":" << e.getFormattedMessage() );
  }


  if (!_explicitGps) {
    SG_LOG(SG_INSTR, SG_INFO, "creating default GPS instrument");
    SGPropertyNode_ptr nd(new SGPropertyNode);
    nd->setStringValue("name", "gps");
    nd->setIntValue("number", 0);
    _instruments.push_back("gps[0]");
    set_subsystem("gps[0]", new GPS(nd));
  }

  // bind() created instruments before init.
  for (unsigned int i=0; i<_instruments.size(); ++i) {
    const std::string& nm(_instruments[i]);
    SGSubsystem* instr = get_subsystem(nm);
    instr->bind();
  }

  SGSubsystemGroup::init();
}

void FGInstrumentMgr::reinit()
{  
// delete all our instrument
  for (unsigned int i=0; i<_instruments.size(); ++i) {
    const std::string& nm(_instruments[i]);
    SGSubsystem* instr = get_subsystem(nm);
    instr->unbind();
    remove_subsystem(nm);
    delete instr;
  }
  
  init();
}

bool FGInstrumentMgr::build (SGPropertyNode* config_props)
{
    for ( int i = 0; i < config_props->nChildren(); ++i ) {
        SGPropertyNode *node = config_props->getChild(i);
        string name = node->getName();

        std::ostringstream subsystemname;
        subsystemname << "instrument-" << i << '-'
                << node->getStringValue("name", name.c_str());
        int index = node->getIntValue("number", 0);
        if (index > 0)
            subsystemname << '['<< index << ']';
        string id = subsystemname.str();
        _instruments.push_back(id);
      
        if ( name == "adf" ) {
            set_subsystem( id, new ADF( node ), 0.15 );

        } else if ( name == "airspeed-indicator" ) {
            set_subsystem( id, new AirspeedIndicator( node ) );

        } else if ( name == "altimeter" ) {
            set_subsystem( id, new Altimeter( node ) );

        } else if ( name == "attitude-indicator" ) {
            set_subsystem( id, new AttitudeIndicator( node ) );

        } else if ( name == "clock" ) {
            set_subsystem( id, new Clock( node ), 0.25 );

        } else if ( name == "dme" ) {
            set_subsystem( id, new DME( node ), 1.0 );

        } else if ( name == "encoder" ) {
            set_subsystem( id, new Altimeter( node ), 0.15 );

        } else if ( name == "gps" ) {
            set_subsystem( id, new GPS( node ) );
            _explicitGps = true;
        } else if ( name == "gsdi" ) {
            set_subsystem( id, new GSDI( node ) );

        } else if ( name == "heading-indicator" ) {
            set_subsystem( id, new HeadingIndicator( node ) );

        } else if ( name == "heading-indicator-fg" ) {
            set_subsystem( id, new HeadingIndicatorFG( node ) );

        } else if ( name == "heading-indicator-dg" ) {
            set_subsystem( id, new HeadingIndicatorDG( node ) );

        } else if ( name == "KR-87" ) {
            set_subsystem( id, new FGKR_87( node ) );

        } else if ( name == "KT-70" ) {
            set_subsystem( id, new FGKT_70( node ) );

        } else if ( name == "magnetic-compass" ) {
            set_subsystem( id, new MagCompass( node ) );

        } else if ( name == "marker-beacon" ) {
            set_subsystem( id, new FGMarkerBeacon( node ), 0.2 );

        } else if ( name == "nav-radio" ) {
            set_subsystem( id, Instrumentation::NavRadio::createInstance( node ) );

        } else if ( name == "slip-skid-ball" ) {
            set_subsystem( id, new SlipSkidBall( node ), 0.03 );

        } else if ( name == "transponder" ) {
            set_subsystem( id, new Transponder( node ), 0.2 );

        } else if ( name == "turn-indicator" ) {
            set_subsystem( id, new TurnIndicator( node ) );

        } else if ( name == "vertical-speed-indicator" ) {
            set_subsystem( id, new VerticalSpeedIndicator( node ) );

        } else if ( name == "radar" ) {
            set_subsystem( id, new wxRadarBg ( node ), 1);

        } else if ( name == "inst-vertical-speed-indicator" ) {
            set_subsystem( id, new InstVerticalSpeedIndicator( node ) );

        } else if ( name == "tacan" ) {
            set_subsystem( id, new TACAN( node ), 0.2 );

        } else if ( name == "mk-viii" ) {
            set_subsystem( id, new MK_VIII( node ), 0.2);

        } else if ( name == "master-reference-gyro" ) {
            set_subsystem( id, new MasterReferenceGyro( node ) );

        } else if ( name == "groundradar" ) {
            set_subsystem( id, new GroundRadar( node ), 1 );

        } else if ( name == "air-ground-radar" ) {
            set_subsystem( id, new agRadar( node ),1);

        } else if ( name == "radar-altimeter" ) {
            set_subsystem( id, new radAlt( node ),1);

        } else if ( name == "tcas" ) {
            set_subsystem( id, new TCAS( node ), 0.2);
        
        } else if ( name == "navigation-display" ) {
            set_subsystem( id, new NavDisplay( node ) );
            
        } else {
            SG_LOG( SG_ALL, SG_ALERT, "Unknown top level section: "
                    << name );
            return false;
        }
    }
    return true;
}

// end of instrument_manager.cxx
