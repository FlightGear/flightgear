//
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
#include <XGL/xgl.h>

#include <stdio.h>
#include <stdlib.h>

// work around a stdc++ lib bug in some versions of linux, but doesn't
// seem to hurt to have this here for all versions of Linux.
#ifdef linux
#  define _G_NO_EXTERN_TEMPLATES
#endif

#include <Include/compiler.h>

#include STL_STRING

#include <Debug/logstream.hxx>
#include <Aircraft/aircraft.hxx>
#include <Airports/simple.hxx>
#include <Astro/sky.hxx>
#include <Astro/stars.hxx>
#include <Astro/solarsystem.hxx>
#include <Autopilot/autopilot.hxx>
#include <Cockpit/cockpit.hxx>
// #include <FDM/Balloon.h>
#include <FDM/JSBsim.hxx>
#include <FDM/LaRCsim.hxx>
#include <FDM/MagicCarpet.hxx>
#include <Include/fg_constants.h>
#include <Include/general.hxx>
#include <Joystick/joystick.hxx>
#include <Math/fg_geodesy.hxx>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Misc/fgpath.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/event.hxx>
#include <Time/fg_time.hxx>
#include <Time/light.hxx>
#include <Time/sunpos.hxx>
#include <Time/moonpos.hxx>

#ifdef FG_NEW_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "fg_init.hxx"
#include "options.hxx"
#include "views.hxx"
#include "fg_serial.hxx"

#if defined(FX) && defined(XMESA)
#include <GL/xmesa.h>
#endif

FG_USING_STD(string);

extern const char *default_root;


// Read in configuration (file and command line)
bool fgInitConfig ( int argc, char **argv ) {
    // Attempt to locate and parse a config file
    // First check fg_root
    FGPath config( current_options.get_fg_root() );
    config.append( "system.fgfsrc" );
    current_options.parse_config_file( config.str() );

    // Next check home directory
    char* envp = ::getenv( "HOME" );
    if ( envp != NULL ) {
	config.set( envp );
	config.append( ".fgfsrc" );
	current_options.parse_config_file( config.str() );
    }

    // Parse remaining command line options
    // These will override anything specified in a config file
    if ( current_options.parse_command_line(argc, argv) !=
	 fgOPTIONS::FG_OPTIONS_OK )
    {
	// Something must have gone horribly wrong with the command
	// line parsing or maybe the user just requested help ... :-)
	current_options.usage();
	FG_LOG( FG_GENERAL, FG_ALERT, "\nExiting ...");
	return false;
    }

    return true;
}


// Set initial position and orientation
bool fgInitPosition( void ) {
    string id;
    FGInterface *f;

    f = current_aircraft.fdm_state;

    id = current_options.get_airport_id();
    if ( id.length() ) {
	// set initial position from airport id

	fgAIRPORTS airports;
	fgAIRPORT a;

	FG_LOG( FG_GENERAL, FG_INFO,
		"Attempting to set starting position from airport code "
		<< id );

	airports.load("apt_simple");
	if ( ! airports.search( id, &a ) ) {
	    FG_LOG( FG_GENERAL, FG_ALERT,
		    "Failed to find " << id << " in database." );
	    exit(-1);
	} else {
	    f->set_Longitude( a.longitude * DEG_TO_RAD );
	    f->set_Latitude( a.latitude * DEG_TO_RAD );
	}
    } else {
	// set initial position from default or command line coordinates

	f->set_Longitude( current_options.get_lon() * DEG_TO_RAD );
	f->set_Latitude( current_options.get_lat() * DEG_TO_RAD );
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "starting altitude is = " << current_options.get_altitude() );

    f->set_Altitude( current_options.get_altitude() * METER_TO_FEET );
    fgFDMSetGroundElevation( current_options.get_flight_model(),
			     (f->get_Altitude() - 3.758099) * FEET_TO_METER );

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
    char *mesa_win_state;

    FG_LOG( FG_GENERAL, FG_INFO, "General Initialization" );
    FG_LOG( FG_GENERAL, FG_INFO, "======= ==============" );

    root = current_options.get_fg_root();
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


// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.
bool fgInitSubsystems( void ) {
    FGTime::cur_time_params = new FGTime();

    fgLIGHT *l = &cur_light_params;
    FGTime *t = FGTime::cur_time_params;

    FG_LOG( FG_GENERAL, FG_INFO, "Initialize Subsystems");
    FG_LOG( FG_GENERAL, FG_INFO, "========== ==========");

    if ( current_options.get_flight_model() == FGInterface::FG_LARCSIM ) {
	cur_fdm_state = new FGLaRCsim;
    } else if ( current_options.get_flight_model() == FGInterface::FG_JSBSIM ) {
	cur_fdm_state = new FGJSBsim;
    // } else if ( current_options.get_flight_model() == 
    //             FGInterface::FG_BALLOONSIM ) {
    //     cur_fdm_state = new FGBalloonSim;
    } else if ( current_options.get_flight_model() == 
		FGInterface::FG_MAGICCARPET ) {
	cur_fdm_state = new FGMagicCarpet;
    } else {
	FG_LOG( FG_GENERAL, FG_ALERT,
		"No flight model, can't init aircraft" );
	exit(-1);
    }

    // allocates structures so must happen before any of the flight
    // model or control parameters are set
    fgAircraftInit();   // In the future this might not be the case.

    // set the initial position
    fgInitPosition();

    // Initialize the Scenery Management subsystem
    if ( fgSceneryInit() ) {
	// Scenery initialized ok.
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Scenery initialization!" );
	exit(-1);
    }

    if( global_tile_mgr.init() ) {
	// Load the local scenery data
	global_tile_mgr.update();
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Tile Manager initialization!" );
	exit(-1);
    }

    FG_LOG( FG_GENERAL, FG_DEBUG,
    	    "Current terrain elevation after tile mgr init " <<
	    scenery.cur_elev );

    // Calculate ground elevation at starting point (we didn't have
    // tmp_abs_view_pos calculated when fgTileMgrUpdate() was called above
    //
    // calculalate a cartesian point somewhere along the line between
    // the center of the earth and our view position.  Doesn't have to
    // be the exact elevation (this is good because we don't know it
    // yet :-)

    // now handled inside of the fgTileMgrUpdate()

    /*
    geod_pos = Point3D( cur_fdm_state->get_Longitude(), cur_fdm_state->get_Latitude(), 0.0);
    tmp_abs_view_pos = fgGeodToCart(geod_pos);

    FG_LOG( FG_GENERAL, FG_DEBUG,
    	    "Initial abs_view_pos = " << tmp_abs_view_pos );
    scenery.cur_elev =
	fgTileMgrCurElev( cur_fdm_state->get_Longitude(), cur_fdm_state->get_Latitude(),
			  tmp_abs_view_pos );
    FG_LOG( FG_GENERAL, FG_DEBUG,
	    "Altitude after update " << scenery.cur_elev );
    */

    fgFDMSetGroundElevation( current_options.get_flight_model(),
			     scenery.cur_elev );

    // Reset our altitude if we are below ground
    FG_LOG( FG_GENERAL, FG_DEBUG, "Current altitude = " << cur_fdm_state->get_Altitude() );
    FG_LOG( FG_GENERAL, FG_DEBUG, "Current runway altitude = " <<
	    cur_fdm_state->get_Runway_altitude() );

    if ( cur_fdm_state->get_Altitude() < cur_fdm_state->get_Runway_altitude() + 3.758099) {
	cur_fdm_state->set_Altitude( cur_fdm_state->get_Runway_altitude() + 3.758099 );
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Updated position (after elevation adj): ("
	    << (cur_fdm_state->get_Latitude() * RAD_TO_DEG) << ", "
	    << (cur_fdm_state->get_Longitude() * RAD_TO_DEG) << ", "
	    << (cur_fdm_state->get_Altitude() * FEET_TO_METER) << ")" );

    // We need to calculate a few more values here that would normally
    // be calculated by the FDM so that the current_view.UpdateViewMath()
    // routine doesn't get hosed.

    double sea_level_radius_meters;
    double lat_geoc;
    // Set the FG variables first
    fgGeodToGeoc( cur_fdm_state->get_Latitude(), cur_fdm_state->get_Altitude(),
		  &sea_level_radius_meters, &lat_geoc);
    cur_fdm_state->set_Geocentric_Position( lat_geoc, cur_fdm_state->get_Longitude(),
				cur_fdm_state->get_Altitude() +
				(sea_level_radius_meters * METER_TO_FEET) );
    cur_fdm_state->set_Sea_level_radius( sea_level_radius_meters * METER_TO_FEET );

    cur_fdm_state->set_sin_cos_longitude(cur_fdm_state->get_Longitude());
    cur_fdm_state->set_sin_cos_latitude(cur_fdm_state->get_Latitude());
	
    cur_fdm_state->set_sin_lat_geocentric(sin(lat_geoc));
    cur_fdm_state->set_cos_lat_geocentric(cos(lat_geoc));

    // The following section sets up the flight model EOM parameters
    // and should really be read in from one or more files.

    // Initial Velocity
    cur_fdm_state->set_Velocities_Local( current_options.get_uBody(),
                             current_options.get_vBody(),
                             current_options.get_wBody());

    // Initial Orientation
    cur_fdm_state->set_Euler_Angles( current_options.get_roll() * DEG_TO_RAD,
			 current_options.get_pitch() * DEG_TO_RAD,
			 current_options.get_heading() * DEG_TO_RAD );

    // Initial Angular Body rates
    cur_fdm_state->set_Omega_Body( 7.206685E-05, 0.0, 9.492658E-05 );

    cur_fdm_state->set_Earth_position_angle( 0.0 );

    // Mass properties and geometry values
    cur_fdm_state->set_Inertias( 8.547270E+01,
		     1.048000E+03, 3.000000E+03, 3.530000E+03, 0.000000E+00 );

    // CG position w.r.t. ref. point
    cur_fdm_state->set_CG_Position( 0.0, 0.0, 0.0 );

    // Initialize the event manager
    global_events.Init();

    // Output event stats every 60 seconds
    global_events.Register( "fgEVENT_MGR::PrintStats()",
			    fgMethodCallback<fgEVENT_MGR>( &global_events,
						   &fgEVENT_MGR::PrintStats),
			    fgEVENT::FG_EVENT_READY, 60000 );

    // Initialize the time dependent variables
    t->init(*cur_fdm_state);
    t->update(*cur_fdm_state);

    // Initialize view parameters
    FG_LOG( FG_GENERAL, FG_DEBUG, "Before current_view.init()");
    current_view.Init();
    pilot_view.Init();
    FG_LOG( FG_GENERAL, FG_DEBUG, "After current_view.init()");
    current_view.UpdateViewMath(*cur_fdm_state);
    pilot_view.UpdateViewMath(*cur_fdm_state);
    FG_LOG( FG_GENERAL, FG_DEBUG, "  abs_view_pos = " << current_view.get_abs_view_pos());
    // current_view.UpdateWorldToEye(f);

    // Build the solar system
    //fgSolarSystemInit(*t);
    FG_LOG(FG_GENERAL, FG_INFO, "Building SolarSystem");
    SolarSystem::theSolarSystem = new SolarSystem(t);

    // Initialize the Stars subsystem
    if( fgStarsInit() ) {
	// Stars initialized ok.
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Stars initialization!" );
	exit(-1);
    }

    // Initialize the planetary subsystem
    // global_events.Register( "fgPlanetsInit()", fgPlanetsInit,
    //			    fgEVENT::FG_EVENT_READY, 600000);

    // Initialize the sun's position
    // global_events.Register( "fgSunInit()", fgSunInit,
    //			    fgEVENT::FG_EVENT_READY, 30000 );

    // Intialize the moon's position
    // global_events.Register( "fgMoonInit()", fgMoonInit,
    //			    fgEVENT::FG_EVENT_READY, 600000 );

    // register the periodic update of Sun, moon, and planets
    global_events.Register( "ssolsysUpdate", solarSystemRebuild,
			    fgEVENT::FG_EVENT_READY, 600000);

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

    // Initialize the weather modeling subsystem
#ifdef FG_NEW_WEATHER
    // Initialize the WeatherDatabase
    FG_LOG(FG_GENERAL, FG_INFO, "Creating LocalWeatherDatabase");
    FGLocalWeatherDatabase::theFGLocalWeatherDatabase = 
	new FGLocalWeatherDatabase(
	      Point3D( current_aircraft.fdm_state->get_Latitude(),
		       current_aircraft.fdm_state->get_Longitude(),
		       current_aircraft.fdm_state->get_Altitude() 
		          * FEET_TO_METER) );

    WeatherDatabase = FGLocalWeatherDatabase::theFGLocalWeatherDatabase;

    // register the periodic update of the weather
    global_events.Register( "weather update", fgUpdateWeatherDatabase,
			    fgEVENT::FG_EVENT_READY, 30000);
#else
    current_weather.Init();
#endif

    // Initialize the Cockpit subsystem
    if( fgCockpitInit( &current_aircraft )) {
	// Cockpit initialized ok.
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Cockpit initialization!" );
	exit(-1);
    }

    // Initialize the "sky"
    fgSkyInit();

    // Initialize the flight model subsystem data structures base on
    // above values

    // fgFDMInit( current_options.get_flight_model(), cur_fdm_state,
    //            1.0 / current_options.get_model_hz() );
    cur_fdm_state->init( 1.0 / current_options.get_model_hz() );

    // I'm just sticking this here for now, it should probably move
    // eventually
    scenery.cur_elev = cur_fdm_state->get_Runway_altitude() * FEET_TO_METER;

    if ( cur_fdm_state->get_Altitude() < cur_fdm_state->get_Runway_altitude() + 3.758099) {
	cur_fdm_state->set_Altitude( cur_fdm_state->get_Runway_altitude() + 3.758099 );
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Updated position (after elevation adj): ("
	    << (cur_fdm_state->get_Latitude() * RAD_TO_DEG) << ", "
	    << (cur_fdm_state->get_Longitude() * RAD_TO_DEG) << ", "
	    << (cur_fdm_state->get_Altitude() * FEET_TO_METER) << ")" );
    // end of thing that I just stuck in that I should probably move

    // Joystick support
    if ( fgJoystickInit() ) {
	// Joystick initialized ok.
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Joystick initialization!" );
    }

    // Autopilot init added here, by Jeff Goeke-Smith
    fgAPInit(&current_aircraft);

    // Initialize serial ports
#if ! defined( MACOS )
    fgSerialInit();
#endif

    FG_LOG( FG_GENERAL, FG_INFO, endl);

    return true;
}


void fgReInitSubsystems( void )
{
    FGTime *t = FGTime::cur_time_params;
    
    int toggle_pause = t->getPause();
    
    if( !toggle_pause )
        t->togglePauseMode();
    
    fgInitPosition();
    if( global_tile_mgr.init() ) {
	// Load the local scenery data
	global_tile_mgr.update();
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Tile Manager initialization!" );
		exit(-1);
    }
    fgFDMSetGroundElevation( current_options.get_flight_model(), 
			     scenery.cur_elev );

    // Reset our altitude if we are below ground
    FG_LOG( FG_GENERAL, FG_DEBUG, "Current altitude = " << cur_fdm_state->get_Altitude() );
    FG_LOG( FG_GENERAL, FG_DEBUG, "Current runway altitude = " << 
	    cur_fdm_state->get_Runway_altitude() );

    if ( cur_fdm_state->get_Altitude() < cur_fdm_state->get_Runway_altitude() + 3.758099) {
	cur_fdm_state->set_Altitude( cur_fdm_state->get_Runway_altitude() + 3.758099 );
    }
    double sea_level_radius_meters;
    double lat_geoc;
    // Set the FG variables first
    fgGeodToGeoc( cur_fdm_state->get_Latitude(), cur_fdm_state->get_Altitude(), 
		  &sea_level_radius_meters, &lat_geoc);
    cur_fdm_state->set_Geocentric_Position( lat_geoc, cur_fdm_state->get_Longitude(), 
				cur_fdm_state->get_Altitude() + 
				(sea_level_radius_meters * METER_TO_FEET) );
    cur_fdm_state->set_Sea_level_radius( sea_level_radius_meters * METER_TO_FEET );
	
    cur_fdm_state->set_sin_cos_longitude(cur_fdm_state->get_Longitude());
    cur_fdm_state->set_sin_cos_latitude(cur_fdm_state->get_Latitude());
	
    cur_fdm_state->set_sin_lat_geocentric(sin(lat_geoc));
    cur_fdm_state->set_cos_lat_geocentric(cos(lat_geoc));

    // The following section sets up the flight model EOM parameters
    // and should really be read in from one or more files.

    // Initial Velocity
    cur_fdm_state->set_Velocities_Local( current_options.get_uBody(),
                             current_options.get_vBody(),
                             current_options.get_wBody());

    // Initial Orientation
    cur_fdm_state->set_Euler_Angles( current_options.get_roll() * DEG_TO_RAD,
			 current_options.get_pitch() * DEG_TO_RAD,
			 current_options.get_heading() * DEG_TO_RAD );

    // Initial Angular Body rates
    cur_fdm_state->set_Omega_Body( 7.206685E-05, 0.0, 9.492658E-05 );

    cur_fdm_state->set_Earth_position_angle( 0.0 );

    // Mass properties and geometry values
    cur_fdm_state->set_Inertias( 8.547270E+01, 
		     1.048000E+03, 3.000000E+03, 3.530000E+03, 0.000000E+00 );

    // CG position w.r.t. ref. point
    cur_fdm_state->set_CG_Position( 0.0, 0.0, 0.0 );

    // Initialize view parameters
    current_view.set_view_offset( 0.0 );
    current_view.set_goal_view_offset( 0.0 );
    pilot_view.set_view_offset( 0.0 );
    pilot_view.set_goal_view_offset( 0.0 );

    FG_LOG( FG_GENERAL, FG_DEBUG, "After current_view.init()");
    current_view.UpdateViewMath(*cur_fdm_state);
    pilot_view.UpdateViewMath(*cur_fdm_state);
    FG_LOG( FG_GENERAL, FG_DEBUG, "  abs_view_pos = " << current_view.get_abs_view_pos());

    // fgFDMInit( current_options.get_flight_model(), cur_fdm_state, 
    //            1.0 / current_options.get_model_hz() );
    cur_fdm_state->init( 1.0 / current_options.get_model_hz() );

    scenery.cur_elev = cur_fdm_state->get_Runway_altitude() * FEET_TO_METER;

    if ( cur_fdm_state->get_Altitude() < cur_fdm_state->get_Runway_altitude() + 3.758099) {
	cur_fdm_state->set_Altitude( cur_fdm_state->get_Runway_altitude() + 3.758099 );
    }

    controls.reset_all();
    fgAPReset();

    if( !toggle_pause )
        t->togglePauseMode();
}
