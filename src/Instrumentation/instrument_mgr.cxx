// instrument_mgr.cxx - manage aircraft instruments.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <iostream>
#include <string>
#include <sstream>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sg_inlines.h>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>

#include "instrument_mgr.hxx"
#include "adf.hxx"
#include "airspeed_indicator.hxx"
#include "altimeter.hxx"
#include "annunciator.hxx"
#include "attitude_indicator.hxx"
#include "clock.hxx"
#include "dme.hxx"
#include "gps.hxx"
#include "heading_indicator.hxx"
#include "kr_87.cxx"
#include "mag_compass.hxx"
#include "slip_skid_ball.hxx"
#include "turn_indicator.hxx"
#include "vertical_speed_indicator.hxx"


FGInstrumentMgr::FGInstrumentMgr ()
{
    //set_subsystem("asi", new AirspeedIndicator(0));
    //set_subsystem("asi-backup", new AirspeedIndicator(1, 1, 1));
    set_subsystem("annunciator", new Annunciator);
    //set_subsystem("ai", new AttitudeIndicator);
    //set_subsystem("alt", new Altimeter);
    //set_subsystem("ti", new TurnIndicator);
    //set_subsystem("ball", new SlipSkidBall);
    //set_subsystem("hi", new HeadingIndicator);
    //set_subsystem("vsi", new VerticalSpeedIndicator);
    //set_subsystem("compass", new MagCompass);
    //set_subsystem("dme", new DME, 1.0);
    //set_subsystem("adf", new ADF, 0.15);
    //set_subsystem("gps", new GPS, 0.45);
    //set_subsystem("clock", new Clock, 0.25);
    //set_subsystem("nhi", new NewHeadingIndicator);

    config_props = new SGPropertyNode;

    SGPropertyNode *path_n = fgGetNode("/sim/instrumentation/path");

    if (path_n) {
        SGPath config( globals->get_fg_root() );
        config.append( path_n->getStringValue() );

        SG_LOG( SG_ALL, SG_INFO, "Reading instruments from "
                << config.str() );
        try {
            readProperties( config.str(), config_props );

            if ( build() ) {
                enabled = true;
            } else {
                SG_LOG( SG_ALL, SG_ALERT,
                        "Detected an internal inconsistancy in the instrumentation");
                SG_LOG( SG_ALL, SG_ALERT,
                        " system specification file.  See earlier errors for" );
                SG_LOG( SG_ALL, SG_ALERT,
                        " details.");
                exit(-1);
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
        } else if ( name == "heading-indicator" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new HeadingIndicator( node ) );
        } else if ( name == "KR-87" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new FGKR_87( node ) );
        } else if ( name == "magnetic-compass" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new MagCompass( node ) );
        } else if ( name == "slip-skid-ball" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new SlipSkidBall( node ) );
        } else if ( name == "turn-indicator" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new TurnIndicator( node ) );
        } else if ( name == "vertical-speed-indicator" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new VerticalSpeedIndicator( node ) );
        } else if ( name == "gps" ) {
            set_subsystem( "instrument" + temp.str(), 
                           new GPS( node ), 0.45 );
        } else {
            SG_LOG( SG_ALL, SG_ALERT, "Unknown top level section: " 
                    << name );
            return false;
        }
    }
    return true;
}

// end of instrument_manager.cxx
