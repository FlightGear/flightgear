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
#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#include <stdio.h>
#include <stdlib.h>

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
#include <simgear/misc/fgpath.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Aircraft/aircraft.hxx>
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
#include <Joystick/joystick.hxx>
#include <Objects/matlib.hxx>
#include <Navaids/fixlist.hxx>
#include <Navaids/ilslist.hxx>
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
#include "globals.hxx"
#include "bfi.hxx"

#if defined(FX) && defined(XMESA)
#include <GL/xmesa.h>
#endif

FG_USING_STD(string);

extern const char *default_root;


// Read in configuration (file and command line) and just set fg_root
bool fgInitFGRoot ( int argc, char **argv ) {
    // Attempt to locate and parse a config file
    // First check fg_root
    FGPath config( globals->get_options()->get_fg_root() );
    config.append( "system.fgfsrc" );
    globals->get_options()->scan_config_file_for_root( config.str() );

    // Next check home directory
    char* envp = ::getenv( "HOME" );
    if ( envp != NULL ) {
	config.set( envp );
	config.append( ".fgfsrc" );
	globals->get_options()->scan_config_file_for_root( config.str() );
    }

    // Parse remaining command line options
    // These will override anything specified in a config file
    globals->get_options()->scan_command_line_for_root(argc, argv);

    return true;
}


// Read in configuration (file and command line)
bool fgInitConfig ( int argc, char **argv ) {
    // Attempt to locate and parse a config file
    // First check fg_root
    FGPath config( globals->get_options()->get_fg_root() );
    config.append( "system.fgfsrc" );
    globals->get_options()->parse_config_file( config.str() );

    // Next check home directory
    char* envp = ::getenv( "HOME" );
    if ( envp != NULL ) {
	config.set( envp );
	config.append( ".fgfsrc" );
	globals->get_options()->parse_config_file( config.str() );
    }

    // Parse remaining command line options
    // These will override anything specified in a config file
    if ( globals->get_options()->parse_command_line(argc, argv) !=
	 FGOptions::FG_OPTIONS_OK )
    {
	// Something must have gone horribly wrong with the command
	// line parsing or maybe the user just requested help ... :-)
	globals->get_options()->usage();
	FG_LOG( FG_GENERAL, FG_ALERT, "\nExiting ...");
	return false;
    }

    return true;
}


// find basic airport location info from airport database
bool fgFindAirportID( const string& id, FGAirport *a ) {
    if ( id.length() ) {
	FGPath path( globals->get_options()->get_fg_root() );
	path.append( "Airports" );
	path.append( "simple.mk4" );
	FGAirports airports( path.c_str() );

	FG_LOG( FG_GENERAL, FG_INFO, "Searching for airport code = " << id );

	if ( ! airports.search( id, a ) ) {
	    FG_LOG( FG_GENERAL, FG_ALERT,
		    "Failed to find " << id << " in " << path.str() );
	    return false;
	}
    } else {
	return false;
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Position for " << id << " is ("
	    << a->longitude << ", "
	    << a->latitude << ")" );

    return true;
}


// Set current_options lon/lat given an airport id
bool fgSetPosFromAirportID( const string& id ) {
    FGAirport a;
    // double lon, lat;

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Attempting to set starting position from airport code " << id );

    if ( fgFindAirportID( id, &a ) ) {
	globals->get_options()->set_lon( a.longitude );
	globals->get_options()->set_lat( a.latitude );
	current_properties.setDoubleValue("/position/longitude",
					  a.longitude);
	current_properties.setDoubleValue("/position/latitude",
					  a.latitude);

	FG_LOG( FG_GENERAL, FG_INFO,
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

	FGPath path( globals->get_options()->get_fg_root() );
	path.append( "Airports" );
	path.append( "runways.mk4" );
	FGRunways runways( path.c_str() );

	FG_LOG( FG_GENERAL, FG_INFO,
		"Attempting to set starting position from runway code "
		<< id << " heading " << tgt_hdg );

	// FGPath inpath( globals->get_options()->get_fg_root() );
	// inpath.append( "Airports" );
	// inpath.append( "apt_simple" );
	// airports.load( inpath.c_str() );

	// FGPath outpath( globals->get_options()->get_fg_root() );
	// outpath.append( "Airports" );
	// outpath.append( "simple.gdbm" );
	// airports.dump_gdbm( outpath.c_str() );

	if ( ! runways.search( id, &r ) ) {
	    FG_LOG( FG_GENERAL, FG_ALERT,
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
	    FG_LOG( FG_GENERAL, FG_INFO,
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
	    FG_LOG( FG_GENERAL, FG_INFO,
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

	FG_LOG( FG_GENERAL, FG_INFO, "closest runway = " << found_r.rwy_no
		<< " + " << found_dir );

    } else {
	return false;
    }

    double heading = found_r.heading + found_dir;
    while ( heading >= 360.0 ) { heading -= 360.0; }

    double lat2, lon2, az2;
    double azimuth = found_r.heading + found_dir + 180.0;
    while ( azimuth >= 360.0 ) { azimuth -= 360.0; }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "runway =  " << found_r.lon << ", " << found_r.lat
	    << " length = " << found_r.length * FEET_TO_METER * 0.5 
	    << " heading = " << azimuth );
    geo_direct_wgs_84 ( 0, found_r.lat, found_r.lon, 
			azimuth, found_r.length * FEET_TO_METER * 0.5 - 5.0,
			&lat2, &lon2, &az2 );
    globals->get_options()->set_lon( lon2 );
    globals->get_options()->set_lat( lat2 );
    globals->get_options()->set_heading( heading );
    current_properties.setDoubleValue("/position/longitude", lon2);
    current_properties.setDoubleValue("/position/latitude", lat2);
    current_properties.setDoubleValue("/orientation/heading", heading);

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Position for " << id << " is ("
	    << lon2 << ", "
	    << lat2 << ") new heading is "
	    << heading );

    return true;
}


// Set initial position and orientation
bool fgInitPosition( void ) {
    FGInterface *f = current_aircraft.fdm_state;
    string id = globals->get_options()->get_airport_id();

    // set initial position from default or command line coordinates
    f->set_Longitude( globals->get_options()->get_lon() * DEG_TO_RAD );
    f->set_Latitude( globals->get_options()->get_lat() * DEG_TO_RAD );

    if ( scenery.cur_elev > globals->get_options()->get_altitude() - 1) {
	globals->get_options()->set_altitude( scenery.cur_elev + 1 );
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "starting altitude is = " << globals->get_options()->get_altitude() );

    f->set_Altitude( globals->get_options()->get_altitude() * METER_TO_FEET );
    // fgFDMSetGroundElevation( globals->get_options()->get_flight_model(),
    //	                        f->get_Altitude() * FEET_TO_METER );

#if 0
    current_properties.setDoubleValue("/position/longitude",
				      f->get_Longitude() * RAD_TO_DEG);
    current_properties.setDoubleValue("/position/latitude",
				      f->get_Latitude() * RAD_TO_DEG);
    current_properties.setDoubleValue("/position/altitude",
				      f->get_Altitude() * RAD_TO_DEG);
#endif

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Initial position is: ("
	    << (f->get_Longitude() * RAD_TO_DEG) << ", "
	    << (f->get_Latitude() * RAD_TO_DEG) << ", "
	    << (f->get_Altitude() * FEET_TO_METER) << ")" );

    return true;
}


// General house keeping initializations
bool fgInitGeneral( void ) {
    string root;

#if defined(FX) && defined(XMESA)
    char *mesa_win_state;
#endif

    FG_LOG( FG_GENERAL, FG_INFO, "General Initialization" );
    FG_LOG( FG_GENERAL, FG_INFO, "======= ==============" );

    root = globals->get_options()->get_fg_root();
    if ( ! root.length() ) {
	// No root path set? Then bail ...
	FG_LOG( FG_GENERAL, FG_ALERT,
		"Cannot continue without environment variable FG_ROOT"
		<< "being defined." );
	exit(-1);
    }
    FG_LOG( FG_GENERAL, FG_INFO, "FG_ROOT = " << '"' << root << '"' << endl );

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


// set initial aircraft speed
bool fgVelocityInit( void ) {
  switch(globals->get_options()->get_speedset()) {
    case FGOptions::FG_VC:
      current_aircraft.fdm_state->set_V_calibrated_kts( 
                                    globals->get_options()->get_vc() );
      break;
    case FGOptions::FG_MACH:
      current_aircraft.fdm_state->set_Mach_number(
                                    globals->get_options()->get_mach() );
      break;
    case FGOptions::FG_VTUVW:
      current_aircraft.fdm_state->set_Velocities_Wind_Body(
                                    globals->get_options()->get_uBody(),
                                    globals->get_options()->get_vBody(),
                                    globals->get_options()->get_wBody() );
      break;
    case FGOptions::FG_VTNED:
      current_aircraft.fdm_state->set_Velocities_Local(
                                    globals->get_options()->get_vNorth(),
                                    globals->get_options()->get_vEast(),
                                    globals->get_options()->get_vDown() );
      break;
    default:
      current_aircraft.fdm_state->set_V_calibrated_kts( 0.0 );
    }
    return true;  
}             

        
// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.
bool fgInitSubsystems( void ) {
    fgLIGHT *l = &cur_light_params;

    FG_LOG( FG_GENERAL, FG_INFO, "Initialize Subsystems");
    FG_LOG( FG_GENERAL, FG_INFO, "========== ==========");

    // Initialize the material property lib
    FGPath mpath( globals->get_options()->get_fg_root() );
    mpath.append( "materials" );
    if ( material_lib.load( mpath.str() ) ) {
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error loading material lib!" );
	exit(-1);
    }

    // Initialize the Scenery Management subsystem
    if ( fgSceneryInit() ) {
	// Material lib initialized ok.
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Scenery initialization!" );
	exit(-1);
    }

    if ( global_tile_mgr.init() ) {
	// Load the local scenery data
	global_tile_mgr.update( globals->get_options()->get_lon(),
				globals->get_options()->get_lat() );
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Tile Manager initialization!" );
	exit(-1);
    }

    FG_LOG( FG_GENERAL, FG_DEBUG,
    	    "Current terrain elevation after tile mgr init " <<
	    scenery.cur_elev );

    if ( globals->get_options()->get_flight_model() == FGInterface::FG_LARCSIM ) {
	cur_fdm_state = new FGLaRCsim;
    } else if ( globals->get_options()->get_flight_model() == FGInterface::FG_JSBSIM ) {
	cur_fdm_state = new FGJSBsim;
    } else if ( globals->get_options()->get_flight_model() == FGInterface::FG_ADA ) {
	cur_fdm_state = new FGADA;
    } else if ( globals->get_options()->get_flight_model() == 
	 	FGInterface::FG_BALLOONSIM ) {
	cur_fdm_state = new FGBalloonSim;
    } else if ( globals->get_options()->get_flight_model() == 
		FGInterface::FG_MAGICCARPET ) {
	cur_fdm_state = new FGMagicCarpet;
    } else if ( globals->get_options()->get_flight_model() == 
		FGInterface::FG_EXTERNAL ) {
	cur_fdm_state = new FGExternal;
    } else {
	FG_LOG( FG_GENERAL, FG_ALERT,
		"No flight model, can't init aircraft" );
	exit(-1);
    }

    // allocates structures so must happen before any of the flight
    // model or control parameters are set
    fgAircraftInit();   // In the future this might not be the case.

    fgFDMSetGroundElevation( globals->get_options()->get_flight_model(),
			     scenery.cur_elev );
    
    // set the initial position
    fgInitPosition();

    // Calculate ground elevation at starting point (we didn't have
    // tmp_abs_view_pos calculated when fgTileMgrUpdate() was called above
    //
    // calculalate a cartesian point somewhere along the line between
    // the center of the earth and our view position.  Doesn't have to
    // be the exact elevation (this is good because we don't know it
    // yet :-)

    // now handled inside of the fgTileMgrUpdate()

    // Reset our altitude if we are below ground
    FG_LOG( FG_GENERAL, FG_DEBUG, "Current altitude = "
	    << cur_fdm_state->get_Altitude() );
    FG_LOG( FG_GENERAL, FG_DEBUG, "Current runway altitude = " <<
	    cur_fdm_state->get_Runway_altitude() );

    if ( cur_fdm_state->get_Altitude() < cur_fdm_state->get_Runway_altitude() +
	 3.758099) {
	cur_fdm_state->set_Altitude( cur_fdm_state->get_Runway_altitude() +
				     3.758099 );
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Updated position (after elevation adj): ("
	    << (cur_fdm_state->get_Latitude() * RAD_TO_DEG) << ", "
	    << (cur_fdm_state->get_Longitude() * RAD_TO_DEG) << ", "
	    << (cur_fdm_state->get_Altitude() * FEET_TO_METER) << ")" );

    // We need to calculate a few sea_level_radius here so we can pass
    // the correct value to the view class
    double sea_level_radius_meters;
    double lat_geoc;
    sgGeodToGeoc( cur_fdm_state->get_Latitude(),
		  cur_fdm_state->get_Altitude(),
		  &sea_level_radius_meters, &lat_geoc);
    cur_fdm_state->set_Sea_level_radius( sea_level_radius_meters *
					 METER_TO_FEET );

    // The following section sets up the flight model EOM parameters
    // and should really be read in from one or more files.

    // Initial Velocity
    fgVelocityInit();

    // Initial Orientation
    cur_fdm_state->
	set_Euler_Angles( globals->get_options()->get_roll() * DEG_TO_RAD,
			  globals->get_options()->get_pitch() * DEG_TO_RAD,
			  globals->get_options()->get_heading() * DEG_TO_RAD );

    // Initialize the event manager
    global_events.Init();

    // Output event stats every 60 seconds
    global_events.Register( "fgEVENT_MGR::PrintStats()",
			    fgMethodCallback<fgEVENT_MGR>( &global_events,
						   &fgEVENT_MGR::PrintStats),
			    fgEVENT::FG_EVENT_READY, 60000 );

    // Initialize win_ratio parameters
    for ( int i = 0; i < globals->get_viewmgr()->size(); ++i ) {
	globals->get_viewmgr()->get_view(i)->
	    set_win_ratio( globals->get_options()->get_xsize() /
			   globals->get_options()->get_ysize() );
    }

    // Initialize pilot view
    FGViewerRPH *pilot_view =
	(FGViewerRPH *)globals->get_viewmgr()->get_view( 0 );

    pilot_view->set_geod_view_pos( cur_fdm_state->get_Longitude(), 
				   cur_fdm_state->get_Lat_geocentric(), 
				   cur_fdm_state->get_Altitude() *
				   FEET_TO_METER );
    pilot_view->set_sea_level_radius( cur_fdm_state->get_Sea_level_radius() *
				      FEET_TO_METER ); 
    pilot_view->set_rph( cur_fdm_state->get_Phi(),
			 cur_fdm_state->get_Theta(),
			 cur_fdm_state->get_Psi() );

    // set current view to 0 (first) which is our main pilot view
    globals->set_current_view( pilot_view );

    FG_LOG( FG_GENERAL, FG_DEBUG, "  abs_view_pos = "
	    << globals->get_current_view()->get_abs_view_pos());

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
    // update the current timezone each 30 minutes
    global_events.Register( "fgUpdateLocalTime()", fgUpdateLocalTime,
			    fgEVENT::FG_EVENT_READY, 1800000);

    // Initialize the weather modeling subsystem
#ifndef FG_OLD_WEATHER
    // Initialize the WeatherDatabase
    FG_LOG(FG_GENERAL, FG_INFO, "Creating LocalWeatherDatabase");
    sgVec3 position;
    sgSetVec3( position, current_aircraft.fdm_state->get_Latitude(),
	       current_aircraft.fdm_state->get_Longitude(),
	       current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER );
    FGLocalWeatherDatabase::theFGLocalWeatherDatabase = 
	new FGLocalWeatherDatabase( position, globals->get_options()->get_fg_root() );
    // cout << theFGLocalWeatherDatabase << endl;
    // cout << "visibility = " 
    //      << theFGLocalWeatherDatabase->getWeatherVisibility() << endl;

    WeatherDatabase = FGLocalWeatherDatabase::theFGLocalWeatherDatabase;
     
    // register the periodic update of the weather
    global_events.Register( "weather update", fgUpdateWeatherDatabase,
                            fgEVENT::FG_EVENT_READY, 30000);
#else
    current_weather.Init();
#endif

    // Initialize vor/ndb/ils/fix list management and query systems
    FG_LOG(FG_GENERAL, FG_INFO, "Loading Navaids");

    FG_LOG(FG_GENERAL, FG_INFO, "  VOR/NDB");
    current_navlist = new FGNavList;
    FGPath p_nav( globals->get_options()->get_fg_root() );
    p_nav.append( "Navaids/default.nav" );
    current_navlist->init( p_nav );

    FG_LOG(FG_GENERAL, FG_INFO, "  ILS");
    current_ilslist = new FGILSList;
    FGPath p_ils( globals->get_options()->get_fg_root() );
    p_ils.append( "Navaids/default.ils" );
    current_ilslist->init( p_ils );

    FG_LOG(FG_GENERAL, FG_INFO, "  Fixes");
    current_fixlist = new FGFixList;
    FGPath p_fix( globals->get_options()->get_fg_root() );
    p_fix.append( "Navaids/default.fix" );
    current_fixlist->init( p_fix );

    // Initialize the underlying radio stack model
    current_radiostack = new FGRadioStack;

    current_radiostack->search( cur_fdm_state->get_Longitude(),
				cur_fdm_state->get_Latitude(),
				cur_fdm_state->get_Altitude() * FEET_TO_METER );

    current_radiostack->update( cur_fdm_state->get_Longitude(),
				cur_fdm_state->get_Latitude(),
				cur_fdm_state->get_Altitude() * FEET_TO_METER );

    // Search radio database once per second
    global_events.Register( "fgRadioSearch()", fgRadioSearch,
			    fgEVENT::FG_EVENT_READY, 1000);


    // Initialize the Cockpit subsystem
    if( fgCockpitInit( &current_aircraft )) {
	// Cockpit initialized ok.
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Cockpit initialization!" );
	exit(-1);
    }

    // Initialize the flight model subsystem data structures base on
    // above values

    if ( cur_fdm_state->init( 1.0 / globals->get_options()->get_model_hz() ) ) {
	// fdm init successful
    } else {
	FG_LOG( FG_GENERAL, FG_ALERT, "FDM init() failed!  Cannot continue." );
	exit(-1);
    }

    // *ABCD* I'm just sticking this here for now, it should probably
    // move eventually
    scenery.cur_elev = cur_fdm_state->get_Runway_altitude() * FEET_TO_METER;

    if ( cur_fdm_state->get_Altitude() <
	 cur_fdm_state->get_Runway_altitude() + 3.758099)
    {
	cur_fdm_state->set_Altitude( cur_fdm_state->get_Runway_altitude() +
				     3.758099 );
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Updated position (after elevation adj): ("
	    << (cur_fdm_state->get_Latitude() * RAD_TO_DEG) << ", "
	    << (cur_fdm_state->get_Longitude() * RAD_TO_DEG) << ", "
	    << (cur_fdm_state->get_Altitude() * FEET_TO_METER) << ")" );
    // *ABCD* end of thing that I just stuck in that I should probably
    // move

    // Joystick support
    if ( ! fgJoystickInit() ) {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Joystick initialization!" );
    }

    // Autopilot init
    current_autopilot = new FGAutopilot;
    current_autopilot->init();

    // initialize the gui parts of the autopilot
    NewTgtAirportInit();
    fgAPAdjustInit() ;
    NewHeadingInit();
    NewAltitudeInit();

    // Initialize I/O channels
#if ! defined( macintosh )
    fgIOInit();
#endif

    // Initialize the 2D panel.
    string panel_path =
	current_properties.getStringValue("/sim/panel/path",
					  "Panels/Default/default.xml");
    current_panel = fgReadPanel(panel_path);
    if (current_panel == 0) {
	FG_LOG( FG_INPUT, FG_ALERT, 
		"Error reading new panel from " << panel_path );
    } else {
	FG_LOG( FG_INPUT, FG_INFO, "Loaded new panel from " << panel_path );
    }

    // Initialize the BFI
    FGBFI::init();

    FG_LOG( FG_GENERAL, FG_INFO, endl);

    return true;
}


void fgReInitSubsystems( void )
{
    bool freeze = globals->get_freeze();
    if( !freeze )
        globals->set_freeze( true );
    
    if( global_tile_mgr.init() ) {
	// Load the local scenery data
	global_tile_mgr.update( globals->get_options()->get_lon(),
				globals->get_options()->get_lat() );
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Tile Manager initialization!" );
		exit(-1);
    }

    // cout << "current scenery elev = " << scenery.cur_elev << endl;

    fgInitPosition();
    fgFDMSetGroundElevation( globals->get_options()->get_flight_model(), 
			     scenery.cur_elev );

    // Reset our altitude if we are below ground
    FG_LOG( FG_GENERAL, FG_DEBUG, "Current altitude = "
	    << cur_fdm_state->get_Altitude() );
    FG_LOG( FG_GENERAL, FG_DEBUG, "Current runway altitude = "
	    << cur_fdm_state->get_Runway_altitude() );

    if ( cur_fdm_state->get_Altitude() <
	 cur_fdm_state->get_Runway_altitude() + 3.758099)
    {
	cur_fdm_state->set_Altitude( cur_fdm_state->get_Runway_altitude() +
				     3.758099 );
    }
    double sea_level_radius_meters;
    double lat_geoc;
    sgGeodToGeoc( cur_fdm_state->get_Latitude(), cur_fdm_state->get_Altitude(), 
		  &sea_level_radius_meters, &lat_geoc);
    cur_fdm_state->set_Sea_level_radius( sea_level_radius_meters *
					 METER_TO_FEET );
	
    // The following section sets up the flight model EOM parameters
    // and should really be read in from one or more files.

    // Initial Velocity
    fgVelocityInit();

    // Initial Orientation
    cur_fdm_state->
	set_Euler_Angles( globals->get_options()->get_roll() * DEG_TO_RAD,
			  globals->get_options()->get_pitch() * DEG_TO_RAD,
			  globals->get_options()->get_heading() * DEG_TO_RAD );

    // Initialize view parameters
    FGViewerRPH *pilot_view =
	(FGViewerRPH *)globals->get_viewmgr()->get_view( 0 );

    pilot_view->set_view_offset( 0.0 );
    pilot_view->set_goal_view_offset( 0.0 );

    pilot_view->set_geod_view_pos( cur_fdm_state->get_Longitude(), 
				   cur_fdm_state->get_Lat_geocentric(), 
				   cur_fdm_state->get_Altitude() *
				   FEET_TO_METER );
    pilot_view->set_sea_level_radius( cur_fdm_state->get_Sea_level_radius() *
				      FEET_TO_METER ); 
    pilot_view->set_rph( cur_fdm_state->get_Phi(),
			 cur_fdm_state->get_Theta(),
			 cur_fdm_state->get_Psi() );

    // set current view to 0 (first) which is our main pilot view
    globals->set_current_view( pilot_view );

    FG_LOG( FG_GENERAL, FG_DEBUG, "  abs_view_pos = "
	    << globals->get_current_view()->get_abs_view_pos());

    cur_fdm_state->init( 1.0 / globals->get_options()->get_model_hz() );

    scenery.cur_elev = cur_fdm_state->get_Runway_altitude() * FEET_TO_METER;

    if ( cur_fdm_state->get_Altitude() <
	 cur_fdm_state->get_Runway_altitude() + 3.758099)
    {
	cur_fdm_state->set_Altitude( cur_fdm_state->get_Runway_altitude() +
				     3.758099 );
    }

    controls.reset_all();
    current_autopilot->reset();

    if( !freeze )
        globals->set_freeze( false );
}
