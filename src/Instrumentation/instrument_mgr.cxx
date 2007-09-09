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
#include "navradio.hxx"
#include "slip_skid_ball.hxx"
#include "transponder.hxx"
#include "turn_indicator.hxx"
#include "vertical_speed_indicator.hxx"
#include "inst_vertical_speed_indicator.hxx" // (TJ)
#include "od_gauge.hxx"
#include "wxradar.hxx"
#include "tacan.hxx"
#include "mk_viii.hxx"
#include "mrg.hxx"
#include "groundradar.hxx"

FGInstrumentMgr::FGInstrumentMgr ()
{
    set_subsystem("od_gauge", new FGODGauge);
    set_subsystem("hud", new HUD);

    config_props = new SGPropertyNode;

    SGPropertyNode *path_n = fgGetNode("/sim/instrumentation/path");

    if (path_n) {
        SGPath config( globals->get_fg_root() );
        config.append( path_n->getStringValue() );

        SG_LOG( SG_ALL, SG_INFO, "Reading instruments from "
                << config.str() );
        try {
            readProperties( config.str(), config_props );

            if ( !build() ) {
                throw sg_throwable(string(
                        "Detected an internal inconsistency in the instrumentation\n"
                        "system specification file.  See earlier errors for details."));
            }
        } catch (const sg_exception& exc) {
            SG_LOG( SG_ALL, SG_ALERT, "Failed to load instrumentation system model: "
                    << config.str() );
        }

    } else {
        SG_LOG( SG_ALL, SG_WARN,
                "No instrumentation model specified for this model!");
    }

    delete config_props;
}

FGInstrumentMgr::~FGInstrumentMgr ()
{
}

bool FGInstrumentMgr::build ()
{
    SGPropertyNode *node;
    int i;

    int count = config_props->nChildren();
    for ( i = 0; i < count; ++i ) {
        node = config_props->getChild(i);
        string name = node->getName();
        std::ostringstream temp;
        temp << i;
        if ( name == "adf" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new ADF( node ), 0.15 );
        } else if ( name == "airspeed-indicator" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new AirspeedIndicator( node ) );
        } else if ( name == "altimeter" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new Altimeter( node ) );
        } else if ( name == "attitude-indicator" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new AttitudeIndicator( node ) );
        } else if ( name == "clock" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new Clock( node ), 0.25 );
        } else if ( name == "dme" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new DME( node ), 1.0 );
        } else if ( name == "encoder" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new Altimeter( node ) );
        } else if ( name == "gps" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new GPS( node ), 0.45 );
        } else if ( name == "gsdi" ) {
            set_subsystem( "instrument" + temp.str(),
                           new GSDI( node ) );
        } else if ( name == "heading-indicator" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new HeadingIndicator( node ) );
        } else if ( name == "heading-indicator-fg" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new HeadingIndicatorFG( node ) );
        } else if ( name == "heading-indicator-dg" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new HeadingIndicatorDG( node ) );
        } else if ( name == "KR-87" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new FGKR_87( node ) );
        } else if ( name == "KT-70" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new FGKT_70( node ) );
        } else if ( name == "magnetic-compass" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new MagCompass( node ) );
        } else if ( name == "marker-beacon" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new FGMarkerBeacon( node ) );
        } else if ( name == "nav-radio" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new FGNavRadio( node ) );
        } else if ( name == "slip-skid-ball" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new SlipSkidBall( node ) );
        } else if ( name == "transponder" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new Transponder( node ) );
        } else if ( name == "turn-indicator" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new TurnIndicator( node ) );
        } else if ( name == "vertical-speed-indicator" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new VerticalSpeedIndicator( node ) );
        } else if ( name == "radar" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new wxRadarBg ( node ), 0.5 );
        } else if ( name == "inst-vertical-speed-indicator" ) { // (TJ)
            set_subsystem( "instrument" + temp.str(), 
                           new InstVerticalSpeedIndicator( node ) );
        } else if ( name == "tacan" ) { 
            set_subsystem( "instrument" + temp.str(), 
                           new TACAN( node ) );
        } else if ( name == "mk-viii" ) { 
            set_subsystem( "instrument" + temp.str(), 
                           new MK_VIII( node ) );
        } else if ( name == "master-reference-gyro" ) { 
            set_subsystem( "instrument" + temp.str(), 
                           new MasterReferenceGyro( node ) );
        } else if ( name == "groundradar" ) { 
            set_subsystem( "instrument" + temp.str(), 
                           new GroundRadar( node ), 1 );

        } else {
            SG_LOG( SG_ALL, SG_ALERT, "Unknown top level section: " 
                    << name );
            return false;
        }
    }
    return true;
}

// end of instrument_manager.cxx
