/* -*- Mode: C++ -*-
 *
 * fg_init.c -- Flight Gear top level initialization routines
 *
 * Written by Curtis Olson, started August 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include <Main/fg_init.h>
#include <Main/views.h>

#include <Include/cmdargs.h>
#include <Include/fg_constants.h>
#include <Include/general.h>

#include <Aircraft/aircraft.h>
#include <Astro/moon.h>
#include <Astro/planets.h>
#include <Astro/sky.h>
#include <Astro/stars.h>
#include <Astro/sun.h>
#include <Cockpit/cockpit.h>
#include <Joystick/joystick.h>
#include <Math/fg_random.h>
#include <Scenery/scenery.h>
#include <Scenery/tilemgr.h>
#include <Time/event.h>
#include <Time/fg_time.h>
#include <Time/sunpos.h>
#include <Weather/weather.h>
#include <Main/fg_debug.h>

extern int show_hud;             /* HUD state */
extern int displayInstruments;
extern const char *default_root;

/* General house keeping initializations */

int fgInitGeneral( void ) {
    struct fgGENERAL *g;

    g = &general;

    fgInitDebug();

    fgPrintf( FG_GENERAL, FG_INFO, "General Initialization\n" );
    fgPrintf( FG_GENERAL, FG_INFO, "======= ==============\n" );

    /* seed the random number generater */
    fg_srandom();

    // determine the fg root path. The command line parser getargs() will
    // fill in a root directory if the option was used.

    if( !(g->root_dir) ) { 
	// If not set by command line test for environmental var..
	g->root_dir = getenv("FG_ROOT");
	if ( !g->root_dir ) { 
	    // No root path set? Then assume, we will exit if this is
	    // wrong when looking for support files.
	    g->root_dir = (char *)DefaultRootDir;
	}
    }
    fgPrintf( FG_GENERAL, FG_INFO, "FG_ROOT = %s\n\n", g->root_dir);

    // Dummy value can be changed if future initializations
    // fail a critical task.
    return ( 0 /* FALSE */ ); 
}


// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.

int fgInitSubsystems( void ) {
    double cur_elev;

    // Ok will be flagged only if we get EVERYTHING done.
    int ret_val = 1 /* TRUE */;

    fgFLIGHT *f;
    struct fgLIGHT *l;
    struct fgTIME *t;
    struct fgVIEW *v;

    l = &cur_light_params;
    t = &cur_time_params;
    v = &current_view;

    fgPrintf( FG_GENERAL, FG_INFO, "Initialize Subsystems\n");
    fgPrintf( FG_GENERAL, FG_INFO, "========== ==========\n");

    /****************************************************************
     * The following section sets up the flight model EOM parameters and
     * should really be read in from one or more files.
     ****************************************************************/

    /* Must happen before any of the flight model or control
     * parameters are set */

    fgAircraftInit();   // In the future this might not be the case.
    f = current_aircraft.flight;

    /* Globe Aiport, AZ */
    FG_Runway_longitude = -398391.28;
    FG_Runway_latitude = 120070.41;
    FG_Runway_altitude = 3234.5;
    FG_Runway_heading = 102.0 * DEG_TO_RAD;

    /* Initial Position at (P13) Globe, AZ */
    FG_Longitude = ( -398391.28 / 3600.0 ) * DEG_TO_RAD;
    FG_Latitude  = (  120070.41 / 3600.0 ) * DEG_TO_RAD;
    FG_Runway_altitude = (3234.5 + 300);
    FG_Altitude = FG_Runway_altitude + 3.758099;

    // Initial Position at (E81) Superior, AZ
    // FG_Longitude = ( -111.1270650 ) * DEG_TO_RAD;
    // FG_Latitude  = (  33.2778339 ) * DEG_TO_RAD;
    // FG_Runway_altitude = (2646 + 1000);
    // FG_Altitude = FG_Runway_altitude + 3.758099;

    // Initial Position at (TUS) Tucson, AZ
    // FG_Longitude = ( -110.9412597 ) * DEG_TO_RAD;
    // FG_Latitude  = (  32.1162439 ) * DEG_TO_RAD;
    // FG_Runway_altitude = (2641 + 0);
    // FG_Altitude = FG_Runway_altitude + 3.758099;

    /* Initial Position at near Anchoraze, AK */
    /* FG_Longitude = ( -150.00 ) * DEG_TO_RAD; */
    /* FG_Latitude  = (  61.17 ) * DEG_TO_RAD; */
    /* FG_Runway_altitude = (2641 + 07777); */
    /* FG_Altitude = FG_Runway_altitude + 3.758099; */

    /* Initial Position at (SEZ) SEDONA airport */
    /* FG_Longitude = (-111.7884614 + 0.02) * DEG_TO_RAD; */
    /* FG_Latitude  = (  34.8486289 - 0.04) * DEG_TO_RAD; */
    /* FG_Runway_altitude = (4827 + 800); */
    /* FG_Altitude = FG_Runway_altitude + 3.758099; */
        
    /* Initial Position at (HSP) Hot Springs, VA */
    /* FG_Longitude = (-79.8338964 + 0.02) * DEG_TO_RAD; */
    /* FG_Latitude  = ( 37.9514564 + 0.05) * DEG_TO_RAD; */
    /* FG_Runway_altitude = (792 + 1500); */
    /* FG_Altitude = FG_Runway_altitude + 3.758099; */
    
    /* Initial Position at (ANE) Anoka County airport */
    /* FG_Longitude = -93.2113889 * DEG_TO_RAD; */
    /* FG_Latitude  = 45.145 * DEG_TO_RAD; */
    /* FG_Runway_altitude = 912; */
    /* FG_Altitude = FG_Runway_altitude + 3.758099; */

    /* Initial Position north of the city of Globe */
    /* FG_Longitude = ( -398673.28 / 3600.0 ) * DEG_TO_RAD; */
    /* FG_Latitude  = (  120625.64 / 3600.0 ) * DEG_TO_RAD; */
    /* FG_Longitude = ( -397867.44 / 3600.0 ) * DEG_TO_RAD; */
    /* FG_Latitude  = (  119548.21 / 3600.0 ) * DEG_TO_RAD; */
    /* FG_Altitude = 0.0 + 3.758099; */

    /* Initial Posisition near where I used to live in Globe, AZ */
    /* FG_Longitude = ( -398757.6 / 3600.0 ) * DEG_TO_RAD; */
    /* FG_Latitude  = (  120160.0 / 3600.0 ) * DEG_TO_RAD;  */
    /* FG_Runway_altitude = 5000.0; */
    /* FG_Altitude = FG_Runway_altitude + 3.758099; */

    /* Initial Position: 10125 Jewell St. NE */
    /* FG_Longitude = ( -93.15 ) * DEG_TO_RAD; */
    /* FG_Latitude  = (  45.15 ) * DEG_TO_RAD; */
    /* FG_Altitude = FG_Runway_altitude + 3.758099; */

    // Initial Position: Somewhere near the Grand Canyon
    FG_Longitude = ( -112.5 ) * DEG_TO_RAD;
    FG_Latitude  = (  36.5 ) * DEG_TO_RAD;
    FG_Runway_altitude = 5000.0;
    FG_Altitude = FG_Runway_altitude + 3.758099;

    // Initial Position: (GCN) Grand Canyon Airport, AZ
    // FG_Longitude = ( -112.1469647 ) * DEG_TO_RAD;
    // FG_Latitude  = (   35.9523539 ) * DEG_TO_RAD;
    // FG_Runway_altitude = 6606.0;
    // FG_Altitude = FG_Runway_altitude + 3.758099;

    // Initial Position: Jim Brennon's Kingmont Observatory
    // FG_Longitude = ( -121.1131666 ) * DEG_TO_RAD;
    // FG_Latitude  = (   38.8293916 ) * DEG_TO_RAD;
    // FG_Runway_altitude = 920.0 + 100;
    // FG_Altitude = FG_Runway_altitude + 3.758099;

    // Test Position
    // FG_Longitude = ( -111.18 ) * DEG_TO_RAD;
    // FG_Latitude  = (   33.70 ) * DEG_TO_RAD;
    // FG_Runway_altitude = 5000.0;
    // FG_Altitude = FG_Runway_altitude + 3.758099;

    // A random test position
    // FG_Longitude = ( 88128.00 / 3600.0 ) * DEG_TO_RAD;
    // FG_Latitude  = ( 93312.00 / 3600.0 ) * DEG_TO_RAD;

    fgPrintf( FG_GENERAL, FG_INFO, 
	      "Initial position is: (%.4f, %.4f, %.2f)\n", 
	      FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG, 
	      FG_Altitude * FEET_TO_METER);

    /* Initial Velocity */
    FG_V_north = 0.0 /*  7.287719E+00 */;
    FG_V_east  = 0.0 /*  1.521770E+03 */;
    FG_V_down  = 0.0 /* -1.265722E-05 */;

    /* Initial Orientation */
    FG_Phi   = -2.658474E-06;
    FG_Theta =  7.401790E-03;
    FG_Psi   =  270.0 * DEG_TO_RAD;

    /* Initial Angular B rates */
    FG_P_body = 7.206685E-05;
    FG_Q_body = 0.000000E+00;
    FG_R_body = 9.492658E-05;

    FG_Earth_position_angle = 0.000000E+00;

    /* Mass properties and geometry values */
    FG_Mass = 8.547270E+01;
    FG_I_xx = 1.048000E+03;
    FG_I_yy = 3.000000E+03;
    FG_I_zz = 3.530000E+03;
    FG_I_xz = 0.000000E+00;

    /* CG position w.r.t. ref. point */
    FG_Dx_cg = 0.000000E+00;
    FG_Dy_cg = 0.000000E+00;
    FG_Dz_cg = 0.000000E+00;

    /* Set initial position and slew parameters */
    /* fgSlewInit(-398391.3, 120070.41, 244, 3.1415); */ /* GLOBE Airport */
    /* fgSlewInit(-335340,162540, 15, 4.38); */
    /* fgSlewInit(-398673.28,120625.64, 53, 4.38); */

    /* Initialize the event manager */
    fgEventInit();

    /* Dump event stats every 60 seconds */
    fgEventRegister( "fgEventPrintStats()", fgEventPrintStats,
		     FG_EVENT_READY, 60000 );

    /* Initialize "time" */
    fgTimeInit(t);
    fgTimeUpdate(f, t);

    /* fgViewUpdate() needs the sun in the right place, while
     * fgUpdateSunPos() needs to know the view position.  I'll get
     * around this interdependency for now by calling fgUpdateSunPos()
     * once, then moving on with normal initialization. */
    fgUpdateSunPos();

    /* Initialize view parameters */
    fgViewInit(v);
    fgViewUpdate(f, v, l);

    /* Initialize shared sun position and sun_vec */
    fgEventRegister( "fgUpdateSunPos()", fgUpdateSunPos, FG_EVENT_READY, 1000 );

    /* Initialize the weather modeling subsystem */
    fgWeatherInit();

    /* update the weather for our current position */
    fgEventRegister( "fgWeatherUpdate()", fgWeatherUpdate,
		     FG_EVENT_READY, 120000 );

    /* Initialize the Cockpit subsystem */
    if( fgCockpitInit( &current_aircraft )) {
	// Cockpit initialized ok.
    } else {
    	fgPrintf( FG_GENERAL, FG_EXIT, "Error in Cockpit initialization!\n" );
    }

    // Initialize the orbital elements of sun, moon and mayor planets
    fgSolarSystemInit(*t);

    // Initialize the Stars subsystem
    if( fgStarsInit() ) {
	// Stars initialized ok.
    } else {
    	fgPrintf( FG_GENERAL, FG_EXIT, "Error in Stars initialization!\n" );
    }

    // Initialize the planetary subsystem
    fgEventRegister("fgPlanetsInit()", fgPlanetsInit, FG_EVENT_READY, 600000);

    // Initialize the sun's position 
    fgEventRegister("fgSunInit()", fgSunInit, FG_EVENT_READY, 60000 );

    // Intialize the moon's position
    fgEventRegister( "fgMoonInit()", fgMoonInit, FG_EVENT_READY, 600000 );

    // Initialize the "sky"
    fgSkyInit();

    // Initialize the Scenery Management subsystem
    if( fgTileMgrInit() ) {
	// Load the local scenery data
	fgTileMgrUpdate();
    } else {
    	fgPrintf( FG_GENERAL, FG_EXIT, 
		  "Error in Tile Manager initialization!\n" );
    }

    // I'm just sticking this here for now, it should probably move
    // eventually
    // cur_elev = mesh_altitude(FG_Longitude * RAD_TO_DEG * 3600.0,
    //           FG_Latitude  * RAD_TO_DEG * 3600.0); */
    // fgPrintf( FG_GENERAL, FG_INFO,
    //   "True ground elevation is %.2f meters here.\n",
    //   cur_elev); */

    cur_elev = FG_Runway_altitude * FEET_TO_METER;
    if ( cur_elev > -9990.0 ) {
	FG_Runway_altitude = cur_elev * METER_TO_FEET;
    }

    if ( FG_Altitude < FG_Runway_altitude ) {
	FG_Altitude = FG_Runway_altitude + 3.758099;
    }

    fgPrintf(FG_GENERAL, FG_INFO,
	     "Updated position (after elevation adj): (%.4f, %.4f, %.2f)\n",
	     FG_Latitude * RAD_TO_DEG, FG_Longitude * RAD_TO_DEG,
	     FG_Altitude * FEET_TO_METER);

    /* end of thing that I just stuck in that I should probably move */
		
    /* Initialize the flight model subsystem data structures base on
     * above values */

    fgFlightModelInit( FG_LARCSIM, f, 1.0 / DEFAULT_MODEL_HZ );

    // To HUD or not to HUD  - Now a command line issue
    //              show_hud = 0;

    // Let's not show the instrument panel
    displayInstruments = 0;

    // Joystick support
    if (fgJoystickInit(0) ) {
	// Joystick initialized ok.
    } else {
    	fgPrintf( FG_GENERAL, FG_EXIT, "Error in Joystick initialization!\n" );
    }

    // One more try here to get the sky synced up
    fgSkyColorsInit();
    ret_val = 0;

    fgPrintf(FG_GENERAL, FG_INFO,"\n");
    return ret_val;
}


/* $Log$
/* Revision 1.47  1998/02/19 13:05:53  curt
/* Incorporated some HUD tweaks from Michelle America.
/* Tweaked the sky's sunset/rise colors.
/* Other misc. tweaks.
/*
 * Revision 1.46  1998/02/18 15:07:06  curt
 * Tweaks to build with SGI OpenGL (and therefor hopefully other accelerated
 * drivers will work.)
 *
 * Revision 1.45  1998/02/16 13:39:43  curt
 * Miscellaneous weekend tweaks.  Fixed? a cache problem that caused whole
 * tiles to occasionally be missing.
 *
 * Revision 1.44  1998/02/12 21:59:50  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.43  1998/02/11 02:50:40  curt
 * Minor changes.
 *
 * Revision 1.42  1998/02/09 22:56:58  curt
 * Removed "depend" files from cvs control.  Other minor make tweaks.
 *
 * Revision 1.41  1998/02/09 15:07:50  curt
 * Minor tweaks.
 *
 * Revision 1.40  1998/02/07 15:29:44  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.39  1998/02/03 23:20:25  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.38  1998/02/02 20:53:58  curt
 * Incorporated Durk's changes.
 *
 * Revision 1.37  1998/02/01 03:39:54  curt
 * Minor tweaks.
 *
 * Revision 1.36  1998/01/31 00:43:13  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.35  1998/01/27 00:47:57  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.34  1998/01/22 02:59:37  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.33  1998/01/21 21:11:34  curt
 * Misc. tweaks.
 *
 * Revision 1.32  1998/01/19 19:27:08  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.31  1998/01/19 18:40:32  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.30  1998/01/13 00:23:09  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.29  1998/01/08 02:22:08  curt
 * Beginning to integrate Tile management subsystem.
 *
 * Revision 1.28  1998/01/07 03:18:58  curt
 * Moved astronomical stuff from .../Src/Scenery to .../Src/Astro/
 *
 * Revision 1.27  1998/01/05 18:44:35  curt
 * Add an option to advance/decrease time from keyboard.
 *
 * Revision 1.26  1997/12/30 23:09:04  curt
 * Tweaking initialization sequences.
 *
 * Revision 1.25  1997/12/30 22:22:33  curt
 * Further integration of event manager.
 *
 * Revision 1.24  1997/12/30 20:47:44  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.23  1997/12/30 16:36:50  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.22  1997/12/19 23:34:05  curt
 * Lot's of tweaking with sky rendering and lighting.
 *
 * Revision 1.21  1997/12/19 16:45:00  curt
 * Working on scene rendering order and options.
 *
 * Revision 1.20  1997/12/18 23:32:33  curt
 * First stab at sky dome actually starting to look reasonable. :-)
 *
 * Revision 1.19  1997/12/17 23:13:36  curt
 * Began working on rendering a sky.
 *
 * Revision 1.18  1997/12/15 23:54:49  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.17  1997/12/15 20:59:09  curt
 * Misc. tweaks.
 *
 * Revision 1.16  1997/12/12 19:52:48  curt
 * Working on lightling and material properties.
 *
 * Revision 1.15  1997/12/11 04:43:55  curt
 * Fixed sun vector and lighting problems.  I thing the moon is now lit
 * correctly.
 *
 * Revision 1.14  1997/12/10 22:37:47  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.13  1997/11/25 19:25:32  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.12  1997/11/15 18:16:35  curt
 * minor tweaks.
 *
 * Revision 1.11  1997/10/30 12:38:42  curt
 * Working on new scenery subsystem.
 *
 * Revision 1.10  1997/10/25 03:24:23  curt
 * Incorporated sun, moon, and star positioning code contributed by Durk Talsma.
 *
 * Revision 1.9  1997/09/23 00:29:39  curt
 * Tweaks to get things to compile with gcc-win32.
 *
 * Revision 1.8  1997/09/22 14:44:20  curt
 * Continuing to try to align stars correctly.
 *
 * Revision 1.7  1997/09/16 15:50:30  curt
 * Working on star alignment and time issues.
 *
 * Revision 1.6  1997/09/05 14:17:30  curt
 * More tweaking with stars.
 *
 * Revision 1.5  1997/09/04 02:17:36  curt
 * Shufflin' stuff.
 *
 * Revision 1.4  1997/08/27 21:32:26  curt
 * Restructured view calculation code.  Added stars.
 *
 * Revision 1.3  1997/08/27 03:30:19  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.2  1997/08/25 20:27:23  curt
 * Merged in initial HUD and Joystick code.
 *
 * Revision 1.1  1997/08/23 01:46:20  curt
 * Initial revision.
 *
 */
