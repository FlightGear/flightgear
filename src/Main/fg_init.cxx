// fg_init.cxx -- Flight Gear top level initialization routines
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

// For BC 5.01 this must be included before OpenGL includes.
#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>


#if defined( unix ) || defined( __CYGWIN__ )
#  include <unistd.h>		// for gethostname()
#endif

// work around a stdc++ lib bug in some versions of linux, but doesn't
// seem to hurt to have this here for all versions of Linux.
#ifdef linux
#  define _G_NO_EXTERN_TEMPLATES
#endif

#include <simgear/compiler.h>

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Aircraft/aircraft.hxx>
#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Autopilot/auto_gui.hxx>
#include <Autopilot/newauto.hxx>
#include <Cockpit/cockpit.hxx>
#include <Cockpit/radiostack.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include <FDM/ADA.hxx>
#include <FDM/Balloon.h>
#include <FDM/External.hxx>
#include <FDM/JSBSim.hxx>
#include <FDM/LaRCsim.hxx>
#include <FDM/MagicCarpet.hxx>
#include <Include/general.hxx>
#include <Input/input.hxx>
// #include <Joystick/joystick.hxx>
#include <Objects/matlib.hxx>
#include <Navaids/fixlist.hxx>
#include <Navaids/ilslist.hxx>
#include <Navaids/mkrbeacons.hxx>
#include <Navaids/navlist.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/event.hxx>
#include <Time/light.hxx>
#include <Time/sunpos.hxx>
#include <Time/moonpos.hxx>
#include <Time/tmp.hxx>

#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "options.hxx"
#include "globals.hxx"

#if defined(FX) && defined(XMESA)
#include <GL/xmesa.h>
#endif

SG_USING_STD(string);

extern const char *default_root;


// Read in configuration (file and command line) and just set fg_root
bool fgInitFGRoot ( int argc, char **argv ) {
    string root;
    char* envp;

    // First parse command line options looking for fg-root, this will
    // override anything specified in a config file
    root = fgScanForRoot(argc, argv);

#if defined( unix ) || defined( __CYGWIN__ )
    // Next check home directory for .fgfsrc.hostname file
    if ( root == "" ) {
	envp = ::getenv( "HOME" );
	if ( envp != NULL ) {
	    SGPath config( envp );
	    config.append( ".fgfsrc" );
	    char name[256];
	    gethostname( name, 256 );
	    config.concat( "." );
	    config.concat( name );
 	    root = fgScanForRoot(config.str());
	}
    }
#endif

    // Next check home directory for .fgfsrc file
    if ( root == "" ) {
	envp = ::getenv( "HOME" );
	if ( envp != NULL ) {
	    SGPath config( envp );
	    config.append( ".fgfsrc" );
	    root = fgScanForRoot(config.str());
	}
    }
    
    // Next check if fg-root is set as an env variable
    if ( root == "" ) {
	envp = ::getenv( "FG_ROOT" );
	if ( envp != NULL ) {
	    root = envp;
	}
    }

    // Otherwise, default to a random compiled-in location if we can't
    // find fg-root any other way.
    if ( root == "" ) {
#if defined( __CYGWIN__ )
        root = "/FlightGear";
#elif defined( WIN32 )
	root = "\\FlightGear";
#elif defined( macintosh )
	root = "";
#else
	root = PKGLIBDIR;
#endif
    }

    SG_LOG(SG_INPUT, SG_INFO, "fg_root = " << root );
    globals->set_fg_root(root);

    return true;
}


// Read in configuration (file and command line)
bool fgInitConfig ( int argc, char **argv ) {

				// First, set some sane default values
    fgSetDefaults();

    // Read global preferences from $FG_ROOT/preferences.xml
    SGPath props_path(globals->get_fg_root());
    props_path.append("preferences.xml");
    SG_LOG(SG_INPUT, SG_INFO, "Reading global preferences");
    if (!readProperties(props_path.str(), globals->get_props())) {
      SG_LOG(SG_INPUT, SG_ALERT, "Failed to read global preferences from "
	     << props_path.str());
    } else {
      SG_LOG(SG_INPUT, SG_INFO, "Finished Reading global preferences");
    }

    // Attempt to locate and parse the various config files in order
    // from least precidence to greatest precidence

    // Check for $fg_root/system.fgfsrc
    SGPath config( globals->get_fg_root() );
    config.append( "system.fgfsrc" );
    fgParseOptions(config.str());

    char name[256];
#if defined( unix ) || defined( __CYGWIN__ )
    // Check for $fg_root/system.fgfsrc.hostname
    gethostname( name, 256 );
    config.concat( "." );
    config.concat( name );
    fgParseOptions(config.str());
#endif

    // Check for ~/.fgfsrc
    char* envp = ::getenv( "HOME" );
    if ( envp != NULL ) {
	config.set( envp );
	config.append( ".fgfsrc" );
	fgParseOptions(config.str());
    }

#if defined( unix ) || defined( __CYGWIN__ )
    // Check for ~/.fgfsrc.hostname
    gethostname( name, 256 );
    config.concat( "." );
    config.concat( name );
    fgParseOptions(config.str());
#endif

    // Parse remaining command line options
    // These will override anything specified in a config file
    fgParseOptions(argc, argv);

    return true;
}


// find basic airport location info from airport database
bool fgFindAirportID( const string& id, FGAirport *a ) {
    if ( id.length() ) {
	SGPath path( globals->get_fg_root() );
	path.append( "Airports" );
	path.append( "simple.mk4" );
	FGAirports airports( path.c_str() );

	SG_LOG( SG_GENERAL, SG_INFO, "Searching for airport code = " << id );

	if ( ! airports.search( id, a ) ) {
	    SG_LOG( SG_GENERAL, SG_ALERT,
		    "Failed to find " << id << " in " << path.str() );
	    return false;
	}
    } else {
	return false;
    }

    SG_LOG( SG_GENERAL, SG_INFO,
	    "Position for " << id << " is ("
	    << a->longitude << ", "
	    << a->latitude << ")" );

    return true;
}


// Set current_options lon/lat given an airport id
bool fgSetPosFromAirportID( const string& id ) {
    FGAirport a;
    // double lon, lat;

    SG_LOG( SG_GENERAL, SG_INFO,
	    "Attempting to set starting position from airport code " << id );

    if ( fgFindAirportID( id, &a ) ) {
	fgSetDouble("/position/longitude",  a.longitude );
	fgSetDouble("/position/latitude",  a.latitude );
	SG_LOG( SG_GENERAL, SG_INFO,
		"Position for " << id << " is ("
		<< a.longitude << ", "
		<< a.latitude << ")" );

	return true;
    } else {
	return false;
    }

}


// Set current_options lon/lat given an airport id and heading (degrees)
bool fgSetPosFromAirportIDandHdg( const string& id, double tgt_hdg ) {
    FGRunway r;
    FGRunway found_r;
    double found_dir = 0.0;

    if ( id.length() ) {
	// set initial position from runway and heading

	SGPath path( globals->get_fg_root() );
	path.append( "Airports" );
	path.append( "runways.mk4" );
	FGRunways runways( path.c_str() );

	SG_LOG( SG_GENERAL, SG_INFO,
		"Attempting to set starting position from runway code "
		<< id << " heading " << tgt_hdg );

	// SGPath inpath( globals->get_fg_root() );
	// inpath.append( "Airports" );
	// inpath.append( "apt_simple" );
	// airports.load( inpath.c_str() );

	// SGPath outpath( globals->get_fg_root() );
	// outpath.append( "Airports" );
	// outpath.append( "simple.gdbm" );
	// airports.dump_gdbm( outpath.c_str() );

	if ( ! runways.search( id, &r ) ) {
	    SG_LOG( SG_GENERAL, SG_ALERT,
		    "Failed to find " << id << " in database." );
	    return false;
	}

	double diff;
	double min_diff = 360.0;

	while ( r.id == id ) {
	    // forward direction
	    diff = tgt_hdg - r.heading;
	    while ( diff < -180.0 ) { diff += 360.0; }
	    while ( diff >  180.0 ) { diff -= 360.0; }
	    diff = fabs(diff);
	    SG_LOG( SG_GENERAL, SG_INFO,
		    "Runway " << r.rwy_no << " heading = " << r.heading <<
		    " diff = " << diff );
	    if ( diff < min_diff ) {
		min_diff = diff;
		found_r = r;
		found_dir = 0;
	    }

	    // reverse direction
	    diff = tgt_hdg - r.heading - 180.0;
	    while ( diff < -180.0 ) { diff += 360.0; }
	    while ( diff >  180.0 ) { diff -= 360.0; }
	    diff = fabs(diff);
	    SG_LOG( SG_GENERAL, SG_INFO,
		    "Runway -" << r.rwy_no << " heading = " <<
		    r.heading + 180.0 <<
		    " diff = " << diff );
	    if ( diff < min_diff ) {
		min_diff = diff;
		found_r = r;
		found_dir = 180.0;
	    }

	    runways.next( &r );
	}

	SG_LOG( SG_GENERAL, SG_INFO, "closest runway = " << found_r.rwy_no
		<< " + " << found_dir );

    } else {
	return false;
    }

    double heading = found_r.heading + found_dir;
    while ( heading >= 360.0 ) { heading -= 360.0; }

    double lat2, lon2, az2;
    double azimuth = found_r.heading + found_dir + 180.0;
    while ( azimuth >= 360.0 ) { azimuth -= 360.0; }

    SG_LOG( SG_GENERAL, SG_INFO,
	    "runway =  " << found_r.lon << ", " << found_r.lat
	    << " length = " << found_r.length * SG_FEET_TO_METER * 0.5 
	    << " heading = " << azimuth );
    
    geo_direct_wgs_84 ( 0, found_r.lat, found_r.lon, 
			azimuth, found_r.length * SG_FEET_TO_METER * 0.5 - 5.0,
			&lat2, &lon2, &az2 );

    if ( fabs( fgGetDouble("/sim/startup/offset-distance") ) > SG_EPSILON ) {
	double olat, olon;
	double odist = fgGetDouble("/sim/startup/offset-distance");
	odist *= SG_NM_TO_METER;
	double oaz = azimuth;
	if ( fabs(fgGetDouble("/sim/startup/offset-azimuth")) > SG_EPSILON ) {
	    oaz = fgGetDouble("/sim/startup/offset-azimuth") + 180;
	}
	while ( oaz >= 360.0 ) { oaz -= 360.0; }
	geo_direct_wgs_84 ( 0, lat2, lon2, oaz, odist, &olat, &olon, &az2 );
	lat2=olat;
	lon2=olon;
    }
    fgSetDouble("/position/longitude",  lon2 );
    fgSetDouble("/position/latitude",  lat2 );
    fgSetDouble("/orientation/heading", heading );

    SG_LOG( SG_GENERAL, SG_INFO,
	    "Position for " << id << " is ("
	    << lon2 << ", "
	    << lat2 << ") new heading is "
	    << heading );

    return true;
}


// General house keeping initializations
bool fgInitGeneral( void ) {
    string root;

#if defined(FX) && defined(XMESA)
    char *mesa_win_state;
#endif

    SG_LOG( SG_GENERAL, SG_INFO, "General Initialization" );
    SG_LOG( SG_GENERAL, SG_INFO, "======= ==============" );

    root = globals->get_fg_root();
    if ( ! root.length() ) {
	// No root path set? Then bail ...
	SG_LOG( SG_GENERAL, SG_ALERT,
		"Cannot continue without environment variable FG_ROOT"
		<< "being defined." );
	exit(-1);
    }
    SG_LOG( SG_GENERAL, SG_INFO, "FG_ROOT = " << '"' << root << '"' << endl );

#if defined(FX) && defined(XMESA)
    // initialize full screen flag
    global_fullscreen = false;
    if ( strstr ( general.get_glRenderer(), "Glide" ) ) {
	// Test for the MESA_GLX_FX env variable
	if ( (mesa_win_state = getenv( "MESA_GLX_FX" )) != NULL) {
	    // test if we are fullscreen mesa/glide
	    if ( (mesa_win_state[0] == 'f') ||
		 (mesa_win_state[0] == 'F') ) {
		global_fullscreen = true;
	    }
	}
    }
#endif

    return true;
}


// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.
bool fgInitSubsystems( void ) {
    fgLIGHT *l = &cur_light_params;

    SG_LOG( SG_GENERAL, SG_INFO, "Initialize Subsystems");
    SG_LOG( SG_GENERAL, SG_INFO, "========== ==========");


    ////////////////////////////////////////////////////////////////////
    // Initialize the material property subsystem.
    ////////////////////////////////////////////////////////////////////

    SGPath mpath( globals->get_fg_root() );
    mpath.append( "materials" );
    if ( material_lib.load( mpath.str() ) ) {
    } else {
    	SG_LOG( SG_GENERAL, SG_ALERT, "Error loading material lib!" );
	exit(-1);
    }


    ////////////////////////////////////////////////////////////////////
    // Initialize the scenery management subsystem.
    ////////////////////////////////////////////////////////////////////

    if ( fgSceneryInit() ) {
	// Material lib initialized ok.
    } else {
    	SG_LOG( SG_GENERAL, SG_ALERT, "Error in Scenery initialization!" );
	exit(-1);
    }

    if ( global_tile_mgr.init() ) {
	// Load the local scenery data
	global_tile_mgr.update( fgGetDouble("/position/longitude"),
				fgGetDouble("/position/latitude") );
    } else {
    	SG_LOG( SG_GENERAL, SG_ALERT, "Error in Tile Manager initialization!" );
	exit(-1);
    }

    SG_LOG( SG_GENERAL, SG_DEBUG,
    	    "Current terrain elevation after tile mgr init " <<
	    scenery.cur_elev );


    ////////////////////////////////////////////////////////////////////
    // Initialize the flight model subsystem.
    ////////////////////////////////////////////////////////////////////

    double dt = 1.0 / fgGetInt("/sim/model-hz");
    // cout << "dt = " << dt << endl;

    aircraft_dir = fgGetString("/sim/aircraft-dir");
    const string &model = fgGetString("/sim/flight-model");
    if (model == "larcsim") {
	cur_fdm_state = new FGLaRCsim( dt );
    } else if (model == "jsb") {
	cur_fdm_state = new FGJSBsim( dt );
    } else if (model == "ada") {
	cur_fdm_state = new FGADA( dt );
    } else if (model == "balloon") {
	cur_fdm_state = new FGBalloonSim( dt );
    } else if (model == "magic") {
	cur_fdm_state = new FGMagicCarpet( dt );
    } else if (model == "external") {
	cur_fdm_state = new FGExternal( dt );
    } else {
	SG_LOG(SG_GENERAL, SG_ALERT,
	       "Unrecognized flight model '" << model
	       << ", can't init aircraft");
	exit(-1);
    }
    cur_fdm_state->init();
    cur_fdm_state->bind();
    
    // allocates structures so must happen before any of the flight
    // model or control parameters are set
    fgAircraftInit();   // In the future this might not be the case.


    ////////////////////////////////////////////////////////////////////
    // Initialize the event manager subsystem.
    ////////////////////////////////////////////////////////////////////

    global_events.Init();

    // Output event stats every 60 seconds
    global_events.Register( "fgEVENT_MGR::PrintStats()",
			    fgMethodCallback<fgEVENT_MGR>( &global_events,
						   &fgEVENT_MGR::PrintStats),
			    fgEVENT::FG_EVENT_READY, 60000 );


    ////////////////////////////////////////////////////////////////////
    // Initialize the view manager subsystem.
    ////////////////////////////////////////////////////////////////////

    // Initialize win_ratio parameters
    for ( int i = 0; i < globals->get_viewmgr()->size(); ++i ) {
	globals->get_viewmgr()->get_view(i)->
	    set_win_ratio( fgGetInt("/sim/startup/xsize") /
			   fgGetInt("/sim/startup/ysize") );
    }

    // Initialize pilot view
    FGViewerRPH *pilot_view =
	(FGViewerRPH *)globals->get_viewmgr()->get_view( 0 );

    pilot_view->set_geod_view_pos( cur_fdm_state->get_Longitude(), 
				   cur_fdm_state->get_Lat_geocentric(), 
				   cur_fdm_state->get_Altitude() *
				   SG_FEET_TO_METER );
    pilot_view->set_sea_level_radius( cur_fdm_state->get_Sea_level_radius() *
				      SG_FEET_TO_METER ); 
    pilot_view->set_rph( cur_fdm_state->get_Phi(),
			 cur_fdm_state->get_Theta(),
			 cur_fdm_state->get_Psi() );

    // set current view to 0 (first) which is our main pilot view
    globals->set_current_view( pilot_view );

    SG_LOG( SG_GENERAL, SG_DEBUG, "  abs_view_pos = "
	    << globals->get_current_view()->get_abs_view_pos());


    ////////////////////////////////////////////////////////////////////
    // Initialize the lighting subsystem.
    ////////////////////////////////////////////////////////////////////

    // fgUpdateSunPos() needs a few position and view parameters set
    // so it can calculate local relative sun angle and a few other
    // things for correctly orienting the sky.
    fgUpdateSunPos();
    fgUpdateMoonPos();
    global_events.Register( "fgUpdateSunPos()", fgUpdateSunPos,
			    fgEVENT::FG_EVENT_READY, 60000);
    global_events.Register( "fgUpdateMoonPos()", fgUpdateMoonPos,
			    fgEVENT::FG_EVENT_READY, 60000);

    // Initialize Lighting interpolation tables
    l->Init();

    // update the lighting parameters (based on sun angle)
    global_events.Register( "fgLight::Update()",
			    fgMethodCallback<fgLIGHT>( &cur_light_params,
						       &fgLIGHT::Update),
			    fgEVENT::FG_EVENT_READY, 30000 );


    ////////////////////////////////////////////////////////////////////
    // Initialize the local time subsystem.
    ////////////////////////////////////////////////////////////////////

    // update the current timezone each 30 minutes
    global_events.Register( "fgUpdateLocalTime()", fgUpdateLocalTime,
			    fgEVENT::FG_EVENT_READY, 1800000);


    ////////////////////////////////////////////////////////////////////
    // Initialize the weather subsystem.
    ////////////////////////////////////////////////////////////////////

    // Initialize the weather modeling subsystem
#ifndef FG_OLD_WEATHER
    // Initialize the WeatherDatabase
    SG_LOG(SG_GENERAL, SG_INFO, "Creating LocalWeatherDatabase");
    sgVec3 position;
    sgSetVec3( position, current_aircraft.fdm_state->get_Latitude(),
	       current_aircraft.fdm_state->get_Longitude(),
	       current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER );
    FGLocalWeatherDatabase::theFGLocalWeatherDatabase = 
	new FGLocalWeatherDatabase( position,
				    globals->get_fg_root() );
    // cout << theFGLocalWeatherDatabase << endl;
    // cout << "visibility = " 
    //      << theFGLocalWeatherDatabase->getWeatherVisibility() << endl;

    WeatherDatabase = FGLocalWeatherDatabase::theFGLocalWeatherDatabase;
    
    double init_vis = fgGetDouble("/environment/visibility");
    if ( init_vis > 0 ) {
	WeatherDatabase->setWeatherVisibility( init_vis );
    }

    // register the periodic update of the weather
    global_events.Register( "weather update", fgUpdateWeatherDatabase,
                            fgEVENT::FG_EVENT_READY, 30000);
#else
    current_weather.Init();
#endif

    ////////////////////////////////////////////////////////////////////
    // Initialize vor/ndb/ils/fix list management and query systems
    ////////////////////////////////////////////////////////////////////

    SG_LOG(SG_GENERAL, SG_INFO, "Loading Navaids");

    SG_LOG(SG_GENERAL, SG_INFO, "  VOR/NDB");
    current_navlist = new FGNavList;
    SGPath p_nav( globals->get_fg_root() );
    p_nav.append( "Navaids/default.nav" );
    current_navlist->init( p_nav );

    SG_LOG(SG_GENERAL, SG_INFO, "  ILS and Marker Beacons");
    current_beacons = new FGMarkerBeacons;
    current_beacons->init();
    current_ilslist = new FGILSList;
    SGPath p_ils( globals->get_fg_root() );
    p_ils.append( "Navaids/default.ils" );
    current_ilslist->init( p_ils );

    SG_LOG(SG_GENERAL, SG_INFO, "  Fixes");
    current_fixlist = new FGFixList;
    SGPath p_fix( globals->get_fg_root() );
    p_fix.append( "Navaids/default.fix" );
    current_fixlist->init( p_fix );


    ////////////////////////////////////////////////////////////////////
    // Initialize the built-in commands.
    ////////////////////////////////////////////////////////////////////
    fgInitCommands();


    ////////////////////////////////////////////////////////////////////
    // Initialize the radio stack subsystem.
    ////////////////////////////////////////////////////////////////////

				// A textbook example of how FGSubsystem
				// should work...
    current_radiostack = new FGRadioStack;
    current_radiostack->init();
    current_radiostack->bind();


    ////////////////////////////////////////////////////////////////////
    // Initialize the cockpit subsystem
    ////////////////////////////////////////////////////////////////////

    if( fgCockpitInit( &current_aircraft )) {
	// Cockpit initialized ok.
    } else {
    	SG_LOG( SG_GENERAL, SG_ALERT, "Error in Cockpit initialization!" );
	exit(-1);
    }


    ////////////////////////////////////////////////////////////////////
    // Initialize the joystick subsystem.
    ////////////////////////////////////////////////////////////////////

    // if ( ! fgJoystickInit() ) {
    //   SG_LOG( SG_GENERAL, SG_ALERT, "Error in Joystick initialization!" );
    // }


    ////////////////////////////////////////////////////////////////////
    // Initialize the autopilot subsystem.
    ////////////////////////////////////////////////////////////////////

    current_autopilot = new FGAutopilot;
    current_autopilot->init();

    // initialize the gui parts of the autopilot
    NewTgtAirportInit();
    fgAPAdjustInit() ;
    NewHeadingInit();
    NewAltitudeInit();

    
    ////////////////////////////////////////////////////////////////////
    // Initialize I/O subsystem.
    ////////////////////////////////////////////////////////////////////

#if ! defined( macintosh )
    fgIOInit();
#endif

    // Initialize the 2D panel.
    string panel_path = fgGetString("/sim/panel/path",
				    "Panels/Default/default.xml");
    current_panel = fgReadPanel(panel_path);
    if (current_panel == 0) {
	SG_LOG( SG_INPUT, SG_ALERT, 
		"Error reading new panel from " << panel_path );
    } else {
	SG_LOG( SG_INPUT, SG_INFO, "Loaded new panel from " << panel_path );
	current_panel->init();
	current_panel->bind();
    }

    
    ////////////////////////////////////////////////////////////////////
    // Initialize the default (kludged) properties.
    ////////////////////////////////////////////////////////////////////

    fgInitProps();


    ////////////////////////////////////////////////////////////////////
    // Initialize the controls subsystem.
    ////////////////////////////////////////////////////////////////////

    controls.init();
    controls.bind();


    ////////////////////////////////////////////////////////////////////
    // Initialize the input subsystem.
    ////////////////////////////////////////////////////////////////////

    current_input.init();
    current_input.bind();


    ////////////////////////////////////////////////////////////////////////
    // End of subsystem initialization.
    ////////////////////////////////////////////////////////////////////

    SG_LOG( SG_GENERAL, SG_INFO, endl);

				// Save the initial state for future
				// reference.
    globals->saveInitialState();

    return true;
}


void fgReInitSubsystems( void )
{
    SG_LOG( SG_GENERAL, SG_INFO,
	    "/position/altitude = " << fgGetDouble("/position/altitude") );

    bool freeze = globals->get_freeze();
    if( !freeze )
        globals->set_freeze( true );
    
    // Initialize the Scenery Management subsystem
    if ( ! fgSceneryInit() ) {
    	SG_LOG( SG_GENERAL, SG_ALERT, "Error in Scenery initialization!" );
	exit(-1);
    }

    if( global_tile_mgr.init() ) {
	// Load the local scenery data
	global_tile_mgr.update( fgGetDouble("/position/longitude"),
				fgGetDouble("/position/latitude") );
    } else {
    	SG_LOG( SG_GENERAL, SG_ALERT, "Error in Tile Manager initialization!" );
		exit(-1);
    }

    // Initialize view parameters
    FGViewerRPH *pilot_view =
	(FGViewerRPH *)globals->get_viewmgr()->get_view( 0 );

    pilot_view->set_view_offset( 0.0 );
    pilot_view->set_goal_view_offset( 0.0 );

    pilot_view->set_geod_view_pos( cur_fdm_state->get_Longitude(), 
				   cur_fdm_state->get_Lat_geocentric(), 
				   cur_fdm_state->get_Altitude() *
				   SG_FEET_TO_METER );
    pilot_view->set_sea_level_radius( cur_fdm_state->get_Sea_level_radius() *
				      SG_FEET_TO_METER ); 
    pilot_view->set_rph( cur_fdm_state->get_Phi(),
			 cur_fdm_state->get_Theta(),
			 cur_fdm_state->get_Psi() );

    // set current view to 0 (first) which is our main pilot view
    globals->set_current_view( pilot_view );

    SG_LOG( SG_GENERAL, SG_DEBUG, "  abs_view_pos = "
	    << globals->get_current_view()->get_abs_view_pos());

    cur_fdm_state->init();

    controls.reset_all();
    current_autopilot->reset();

    fgUpdateSunPos();
    fgUpdateMoonPos();
    cur_light_params.Update();
    fgUpdateLocalTime();

    if( !freeze )
        globals->set_freeze( false );
}
