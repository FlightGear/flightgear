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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

// work around a stdc++ lib bug in some versions of linux, but doesn't
// seem to hurt to have this here for all versions of Linux.
#ifdef linux
#  define _G_NO_EXTERN_TEMPLATES
#endif

#include <string>

#include <Include/fg_constants.h>
#include <Include/general.h>

#include <Aircraft/aircraft.hxx>
#include <Airports/simple.hxx>
#include <Astro/sky.hxx>
#include <Astro/stars.hxx>
#include <Astro/solarsystem.hxx>
#include <Autopilot/autopilot.hxx>
#include <Cockpit/cockpit.hxx>
// #include <Debug/fg_debug.h>
#include <Debug/logstream.hxx>
#include <Joystick/joystick.hxx>
#include <Math/fg_geodesy.hxx>
#include <Math/fg_random.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/event.hxx>
#include <Time/fg_time.hxx>
#include <Time/light.hxx>
#include <Time/sunpos.hxx>
#include <Weather/weather.hxx>

#include "fg_init.hxx"
#include "options.hxx"
#include "views.hxx"


extern const char *default_root;


// Set initial position and orientation
int fgInitPosition( void ) {
    string id;
    fgFLIGHT *f;

    f = current_aircraft.flight;

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
	    FG_Longitude = a.longitude * DEG_TO_RAD;
	    FG_Latitude  = a.latitude * DEG_TO_RAD;
	}
    } else {
	// set initial position from default or command line coordinates

	FG_Longitude = current_options.get_lon() * DEG_TO_RAD;
	FG_Latitude  = current_options.get_lat() * DEG_TO_RAD;
    }
    printf("starting altitude is = %.2f\n", current_options.get_altitude());

    FG_Altitude = current_options.get_altitude() * METER_TO_FEET;
    FG_Runway_altitude = FG_Altitude - 3.758099;

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Initial position is: ("
	    << (FG_Longitude * RAD_TO_DEG) << ", " 
	    << (FG_Latitude * RAD_TO_DEG) << ", " 
	    << (FG_Altitude * FEET_TO_METER) << ")" );

    return(1);
}


// General house keeping initializations
int fgInitGeneral( void ) {
    fgGENERAL *g;
    string root;
    int i;

    g = &general;

    // set default log levels
    fglog().setLogLevels( FG_ALL, FG_INFO );

    FG_LOG( FG_GENERAL, FG_INFO, "General Initialization" );
    FG_LOG( FG_GENERAL, FG_INFO, "======= ==============" );

    g->glVendor = (char *)glGetString ( GL_VENDOR );
    g->glRenderer = (char *)glGetString ( GL_RENDERER );
    g->glVersion = (char *)glGetString ( GL_VERSION );

    root = current_options.get_fg_root();
    if ( ! root.length() ) {
	// No root path set? Then bail ...
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"Cannot continue without environment variable FG_ROOT"
		<< "being defined." );
	exit(-1);
    }
    FG_LOG( FG_GENERAL, FG_INFO, "FG_ROOT = " << root << endl );

    // prime the frame rate counter pump
    for ( i = 0; i < FG_FRAME_RATE_HISTORY; i++ ) {
	g->frames[i] = 0.0;
    }

    return ( 1 ); 
}


// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.
int fgInitSubsystems( void )
{
    fgFLIGHT *f;
    fgLIGHT *l;
    fgTIME *t;
    fgVIEW *v;
    Point3D geod_pos, abs_view_pos;

    l = &cur_light_params;
    t = &cur_time_params;
    v = &current_view;

    FG_LOG( FG_GENERAL, FG_INFO, "Initialize Subsystems");
    FG_LOG( FG_GENERAL, FG_INFO, "========== ==========");

    // seed the random number generater
    fg_srandom();

    // allocates structures so must happen before any of the flight
    // model or control parameters are set
    fgAircraftInit();   // In the future this might not be the case.
    f = current_aircraft.flight;

    // set the initial position
    fgInitPosition();

    // Initialize the Scenery Management subsystem
    if ( fgSceneryInit() ) {
	// Scenery initialized ok.
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Scenery initialization!" );
	exit(-1);
    }

    if( fgTileMgrInit() ) {
	// Load the local scenery data
	fgTileMgrUpdate();
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Tile Manager initialization!" );
	exit(-1);
    }

    // calculalate a cartesian point somewhere along the line between
    // the center of the earth and our view position.  Doesn't have to
    // be the exact elevation (this is good because we don't know it
    // yet :-)
    geod_pos = Point3D( FG_Longitude, FG_Latitude, 0.0);
    abs_view_pos = fgGeodToCart(geod_pos);

    // Calculate ground elevation at starting point
    scenery.cur_elev = 
	fgTileMgrCurElev( FG_Longitude, FG_Latitude, abs_view_pos );
    FG_Runway_altitude = scenery.cur_elev * METER_TO_FEET;

    // Reset our altitude if we are below ground
    if ( FG_Altitude < FG_Runway_altitude + 3.758099) {
	FG_Altitude = FG_Runway_altitude + 3.758099;
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Updated position (after elevation adj): ("
	    << (FG_Latitude * RAD_TO_DEG) << ", " 
	    << (FG_Longitude * RAD_TO_DEG) << ", " 
	    << (FG_Altitude * FEET_TO_METER) << ")" );
    // end of thing that I just stuck in that I should probably move
		
    // The following section sets up the flight model EOM parameters
    // and should really be read in from one or more files.

    // Initial Velocity
    FG_V_north = 0.0;   //  7.287719E+00
    FG_V_east  = 0.0;   //  1.521770E+03
    FG_V_down  = 0.0;   // -1.265722E-05

    // Initial Orientation
    FG_Phi   = current_options.get_roll()    * DEG_TO_RAD;
    FG_Theta = current_options.get_pitch()   * DEG_TO_RAD;
    FG_Psi   = current_options.get_heading() * DEG_TO_RAD;

    // Initial Angular B rates
    FG_P_body = 7.206685E-05;
    FG_Q_body = 0.000000E+00;
    FG_R_body = 9.492658E-05;

    FG_Earth_position_angle = 0.000000E+00;

    // Mass properties and geometry values
    FG_Mass = 8.547270E+01;
    FG_I_xx = 1.048000E+03;
    FG_I_yy = 3.000000E+03;
    FG_I_zz = 3.530000E+03;
    FG_I_xz = 0.000000E+00;

    // CG position w.r.t. ref. point
    FG_Dx_cg = 0.000000E+00;
    FG_Dy_cg = 0.000000E+00;
    FG_Dz_cg = 0.000000E+00;

    // Initialize the event manager
    global_events.Init();

    // Output event stats every 60 seconds
    global_events.Register( "fgEVENT_MGR::PrintStats()",
			    fgMethodCallback<fgEVENT_MGR>( &global_events,
						   &fgEVENT_MGR::PrintStats),
			    fgEVENT::FG_EVENT_READY, 60000 );

    // Initialize the time dependent variables
    fgTimeInit(t);
    fgTimeUpdate(f, t);

    // Initialize view parameters
    FG_LOG( FG_GENERAL, FG_DEBUG, "Before v->init()");
    v->Init();
    FG_LOG( FG_GENERAL, FG_DEBUG, "After v->init()");
    v->UpdateViewMath(f);
    v->UpdateWorldToEye(f);

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
    global_events.Register( "fgUpdateSunPos()", fgUpdateSunPos,
			    fgEVENT::FG_EVENT_READY, 60000);

    // Initialize Lighting interpolation tables
    l->Init();

    // update the lighting parameters (based on sun angle)
    global_events.Register( "fgLight::Update()",
			    fgMethodCallback<fgLIGHT>( &cur_light_params,
						       &fgLIGHT::Update),
			    fgEVENT::FG_EVENT_READY, 30000 );

    // Initialize the weather modeling subsystem
    fgWeatherInit();

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

    fgFlightModelInit( current_options.get_flight_model(), f, 
		       1.0 / DEFAULT_MODEL_HZ );

    // I'm just sticking this here for now, it should probably move
    // eventually
    scenery.cur_elev = FG_Runway_altitude * FEET_TO_METER;

    if ( FG_Altitude < FG_Runway_altitude + 3.758099) {
	FG_Altitude = FG_Runway_altitude + 3.758099;
    }

    FG_LOG( FG_GENERAL, FG_INFO,
	    "Updated position (after elevation adj): ("
	    << (FG_Latitude * RAD_TO_DEG) << ", " 
	    << (FG_Longitude * RAD_TO_DEG) << ", "
	    << (FG_Altitude * FEET_TO_METER) << ")" );
    // end of thing that I just stuck in that I should probably move

    // Joystick support
    if ( fgJoystickInit() ) {
	// Joystick initialized ok.
    } else {
    	FG_LOG( FG_GENERAL, FG_ALERT, "Error in Joystick initialization!" );
    }

    // Autopilot init added here, by Jeff Goeke-Smith
    fgAPInit(&current_aircraft);
    
    FG_LOG( FG_GENERAL, FG_INFO, endl);

    return(1);
}


// $Log$
// Revision 1.47  1998/11/06 21:18:10  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.46  1998/10/27 02:14:38  curt
// Changes to support GLUT joystick routines as fall back.
//
// Revision 1.45  1998/10/25 10:57:21  curt
// Changes to use the new joystick library if it is available.
//
// Revision 1.44  1998/10/18 01:17:17  curt
// Point3D tweaks.
//
// Revision 1.43  1998/10/17 01:34:22  curt
// C++ ifying ...
//
// Revision 1.42  1998/10/16 23:27:54  curt
// C++-ifying.
//
// Revision 1.41  1998/10/16 00:54:01  curt
// Converted to Point3D class.
//
// Revision 1.40  1998/10/02 12:46:49  curt
// Added an "auto throttle"
//
// Revision 1.39  1998/09/29 02:03:39  curt
// Autopilot mods.
//
// Revision 1.38  1998/09/15 04:27:30  curt
// Changes for new Astro code.
//
// Revision 1.37  1998/09/15 02:09:26  curt
// Include/fg_callback.hxx
//   Moved code inline to stop g++ 2.7 from complaining.
//
// Simulator/Time/event.[ch]xx
//   Changed return type of fgEVENT::printStat().  void caused g++ 2.7 to
//   complain bitterly.
//
// Minor bugfix and changes.
//
// Simulator/Main/GLUTmain.cxx
//   Added missing type to idle_state definition - eliminates a warning.
//
// Simulator/Main/fg_init.cxx
//   Changes to airport lookup.
//
// Simulator/Main/options.cxx
//   Uses fg_gzifstream when loading config file.
//
// Revision 1.36  1998/09/08 21:40:08  curt
// Fixes by Charlie Hotchkiss.
//
// Revision 1.35  1998/08/29 13:09:26  curt
// Changes to event manager from Bernie Bright.
//
// Revision 1.34  1998/08/27 17:02:06  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.33  1998/08/25 20:53:32  curt
// Shuffled $FG_ROOT file layout.
//
// Revision 1.32  1998/08/25 16:59:09  curt
// Directory reshuffling.
//
// Revision 1.31  1998/08/22  14:49:57  curt
// Attempting to iron out seg faults and crashes.
// Did some shuffling to fix a initialization order problem between view
// position, scenery elevation.
//
// Revision 1.30  1998/08/20 20:32:33  curt
// Reshuffled some of the code in and around views.[ch]xx
//
// Revision 1.29  1998/07/30 23:48:27  curt
// Output position & orientation when pausing.
// Eliminated libtool use.
// Added options to specify initial position and orientation.
// Changed default fov to 55 degrees.
// Added command line option to start in paused or unpaused state.
//
// Revision 1.28  1998/07/27 18:41:25  curt
// Added a pause command "p"
// Fixed some initialization order problems between pui and glut.
// Added an --enable/disable-sound option.
//
// Revision 1.27  1998/07/24 21:39:10  curt
// Debugging output tweaks.
// Cast glGetString to (char *) to avoid compiler errors.
// Optimizations to fgGluLookAt() by Norman Vine.
//
// Revision 1.26  1998/07/22 21:40:44  curt
// Clear to adjusted fog color (for sunrise/sunset effects)
// Make call to fog sunrise/sunset adjustment method.
// Add a stdc++ library bug work around to fg_init.cxx
//
// Revision 1.25  1998/07/13 21:01:38  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.24  1998/07/13 15:32:39  curt
// Clear color buffer if drawing wireframe.
// When specifying and airport, start elevation at -1000 and let the system
// position you at ground level.
//
// Revision 1.23  1998/07/12 03:14:43  curt
// Added ground collision detection.
// Did some serious horsing around to be able to "hug" the ground properly
//   and still be able to take off.
// Set the near clip plane to 1.0 meters when less than 10 meters above the
//   ground.
// Did some serious horsing around getting the initial airplane position to be
//   correct based on rendered terrain elevation.
// Added a little cheat/hack that will prevent the view position from ever
//   dropping below the terrain, even when the flight model doesn't quite
//   put you as high as you'd like.
//
// Revision 1.22  1998/07/04 00:52:25  curt
// Add my own version of gluLookAt() (which is nearly identical to the
// Mesa/glu version.)  But, by calculating the Model View matrix our selves
// we can save this matrix without having to read it back in from the video
// card.  This hopefully allows us to save a few cpu cycles when rendering
// out the fragments because we can just use glLoadMatrixd() with the
// precalculated matrix for each tile rather than doing a push(), translate(),
// pop() for every fragment.
//
// Panel status defaults to off for now until it gets a bit more developed.
//
// Extract OpenGL driver info on initialization.
//
// Revision 1.21  1998/06/27 16:54:33  curt
// Replaced "extern displayInstruments" with a entry in fgOPTIONS.
// Don't change the view port when displaying the panel.
//
// Revision 1.20  1998/06/17 21:35:12  curt
// Refined conditional audio support compilation.
// Moved texture parameter setup calls to ../Scenery/materials.cxx
// #include <string.h> before various STL includes.
// Make HUD default state be enabled.
//
// Revision 1.19  1998/06/08 17:57:05  curt
// Minor sound/startup position tweaks.
//
// Revision 1.18  1998/06/03 00:47:14  curt
// Updated to compile in audio support if OSS available.
// Updated for new version of Steve's audio library.
// STL includes don't use .h
// Small view optimizations.
//
// Revision 1.17  1998/06/01 17:54:42  curt
// Added Linux audio support.
// avoid glClear( COLOR_BUFFER_BIT ) when not using it to set the sky color.
// map stl tweaks.
//
// Revision 1.16  1998/05/29 20:37:24  curt
// Tweaked material properties & lighting a bit in GLUTmain.cxx.
// Read airport list into a "map" STL for dynamic list sizing and fast tree
// based lookups.
//
// Revision 1.15  1998/05/22 21:28:53  curt
// Modifications to use the new fgEVENT_MGR class.
//
// Revision 1.14  1998/05/20 20:51:35  curt
// Tweaked smooth shaded texture lighting properties.
// Converted fgLIGHT to a C++ class.
//
// Revision 1.13  1998/05/16 13:08:35  curt
// C++ - ified views.[ch]xx
// Shuffled some additional view parameters into the fgVIEW class.
// Changed tile-radius to tile-diameter because it is a much better
//   name.
// Added a WORLD_TO_EYE transformation to views.cxx.  This allows us
//  to transform world space to eye space for view frustum culling.
//
// Revision 1.12  1998/05/13 18:29:58  curt
// Added a keyboard binding to dynamically adjust field of view.
// Added a command line option to specify fov.
// Adjusted terrain color.
// Root path info moved to fgOPTIONS.
// Added ability to parse options out of a config file.
//
// Revision 1.11  1998/05/07 23:14:15  curt
// Added "D" key binding to set autopilot heading.
// Made frame rate calculation average out over last 10 frames.
// Borland C++ floating point exception workaround.
// Added a --tile-radius=n option.
//
// Revision 1.10  1998/05/06 03:16:24  curt
// Added an averaged global frame rate counter.
// Added an option to control tile radius.
//
// Revision 1.9  1998/05/03 00:47:31  curt
// Added an option to enable/disable full-screen mode.
//
// Revision 1.8  1998/04/30 12:34:18  curt
// Added command line rendering options:
//   enable/disable fog/haze
//   specify smooth/flat shading
//   disable sky blending and just use a solid color
//   enable wireframe drawing mode
//
// Revision 1.7  1998/04/28 01:20:22  curt
// Type-ified fgTIME and fgVIEW.
// Added a command line option to disable textures.
//
// Revision 1.6  1998/04/26 05:10:03  curt
// "struct fgLIGHT" -> "fgLIGHT" because fgLIGHT is typedef'd.
//
// Revision 1.5  1998/04/25 22:06:30  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.4  1998/04/25 20:24:01  curt
// Cleaned up initialization sequence to eliminate interdependencies
// between sun position, lighting, and view position.  This creates a
// valid single pass initialization path.
//
// Revision 1.3  1998/04/25 15:11:11  curt
// Added an command line option to set starting position based on airport ID.
//
// Revision 1.2  1998/04/24 00:49:20  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Trying out some different option parsing code.
// Some code reorganization.
//
// Revision 1.1  1998/04/22 13:25:44  curt
// C++ - ifing the code.
// Starting a bit of reorganization of lighting code.
//
// Revision 1.56  1998/04/18 04:11:28  curt
// Moved fg_debug to it's own library, added zlib support.
//
// Revision 1.55  1998/04/14 02:21:03  curt
// Incorporated autopilot heading hold contributed by:  Jeff Goeke-Smith
// <jgoeke@voyager.net>
//
// Revision 1.54  1998/04/08 23:35:36  curt
// Tweaks to Gnu automake/autoconf system.
//
// Revision 1.53  1998/04/03 22:09:06  curt
// Converting to Gnu autoconf system.
//
// Revision 1.52  1998/03/23 21:24:38  curt
// Source code formating tweaks.
//
// Revision 1.51  1998/03/14 00:31:22  curt
// Beginning initial terrain texturing experiments.
//
// Revision 1.50  1998/03/09 22:46:19  curt
// Minor tweaks.
//
// Revision 1.49  1998/02/23 19:07:59  curt
// Incorporated Durk's Astro/ tweaks.  Includes unifying the sun position
// calculation code between sun display, and other FG sections that use this
// for things like lighting.
//
// Revision 1.48  1998/02/21 14:53:15  curt
// Added Charlie's HUD changes.
//
// Revision 1.47  1998/02/19 13:05:53  curt
// Incorporated some HUD tweaks from Michelle America.
// Tweaked the sky's sunset/rise colors.
// Other misc. tweaks.
//
// Revision 1.46  1998/02/18 15:07:06  curt
// Tweaks to build with SGI OpenGL (and therefor hopefully other accelerated
// drivers will work.)
//
// Revision 1.45  1998/02/16 13:39:43  curt
// Miscellaneous weekend tweaks.  Fixed? a cache problem that caused whole
// tiles to occasionally be missing.
//
// Revision 1.44  1998/02/12 21:59:50  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.43  1998/02/11 02:50:40  curt
// Minor changes.
//
// Revision 1.42  1998/02/09 22:56:58  curt
// Removed "depend" files from cvs control.  Other minor make tweaks.
//
// Revision 1.41  1998/02/09 15:07:50  curt
// Minor tweaks.
//
// Revision 1.40  1998/02/07 15:29:44  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.39  1998/02/03 23:20:25  curt
// Lots of little tweaks to fix various consistency problems discovered by
// Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
// passed arguments along to the real printf().  Also incorporated HUD changes
// by Michele America.
//
// Revision 1.38  1998/02/02 20:53:58  curt
// Incorporated Durk's changes.
//
// Revision 1.37  1998/02/01 03:39:54  curt
// Minor tweaks.
//
// Revision 1.36  1998/01/31 00:43:13  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.35  1998/01/27 00:47:57  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.34  1998/01/22 02:59:37  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.33  1998/01/21 21:11:34  curt
// Misc. tweaks.
//
// Revision 1.32  1998/01/19 19:27:08  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.31  1998/01/19 18:40:32  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.30  1998/01/13 00:23:09  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.29  1998/01/08 02:22:08  curt
// Beginning to integrate Tile management subsystem.
//
// Revision 1.28  1998/01/07 03:18:58  curt
// Moved astronomical stuff from .../Src/Scenery to .../Src/Astro/
//
// Revision 1.27  1998/01/05 18:44:35  curt
// Add an option to advance/decrease time from keyboard.
//
// Revision 1.26  1997/12/30 23:09:04  curt
// Tweaking initialization sequences.
//
// Revision 1.25  1997/12/30 22:22:33  curt
// Further integration of event manager.
//
// Revision 1.24  1997/12/30 20:47:44  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.23  1997/12/30 16:36:50  curt
// Merged in Durk's changes ...
//
// Revision 1.22  1997/12/19 23:34:05  curt
// Lot's of tweaking with sky rendering and lighting.
//
// Revision 1.21  1997/12/19 16:45:00  curt
// Working on scene rendering order and options.
//
// Revision 1.20  1997/12/18 23:32:33  curt
// First stab at sky dome actually starting to look reasonable. :-)
//
// Revision 1.19  1997/12/17 23:13:36  curt
// Began working on rendering a sky.
//
// Revision 1.18  1997/12/15 23:54:49  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.17  1997/12/15 20:59:09  curt
// Misc. tweaks.
//
// Revision 1.16  1997/12/12 19:52:48  curt
// Working on lightling and material properties.
//
// Revision 1.15  1997/12/11 04:43:55  curt
// Fixed sun vector and lighting problems.  I thing the moon is now lit
// correctly.
//
// Revision 1.14  1997/12/10 22:37:47  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.13  1997/11/25 19:25:32  curt
// Changes to integrate Durk's moon/sun code updates + clean up.
//
// Revision 1.12  1997/11/15 18:16:35  curt
// minor tweaks.
//
// Revision 1.11  1997/10/30 12:38:42  curt
// Working on new scenery subsystem.
//
// Revision 1.10  1997/10/25 03:24:23  curt
// Incorporated sun, moon, and star positioning code contributed by Durk Talsma.
//
// Revision 1.9  1997/09/23 00:29:39  curt
// Tweaks to get things to compile with gcc-win32.
//
// Revision 1.8  1997/09/22 14:44:20  curt
// Continuing to try to align stars correctly.
//
// Revision 1.7  1997/09/16 15:50:30  curt
// Working on star alignment and time issues.
//
// Revision 1.6  1997/09/05 14:17:30  curt
// More tweaking with stars.
//
// Revision 1.5  1997/09/04 02:17:36  curt
// Shufflin' stuff.
//
// Revision 1.4  1997/08/27 21:32:26  curt
// Restructured view calculation code.  Added stars.
//
// Revision 1.3  1997/08/27 03:30:19  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.2  1997/08/25 20:27:23  curt
// Merged in initial HUD and Joystick code.
//
// Revision 1.1  1997/08/23 01:46:20  curt
// Initial revision.
//

