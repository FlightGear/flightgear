/**************************************************************************
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
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include "fg_init.h"
#include "views.h"

#include "../constants.h"
#include "../general.h"

#include "../Aircraft/aircraft.h"
#include "../Cockpit/cockpit.h"
#include "../Joystick/joystick.h"
#include "../Math/fg_random.h"
#include "../Scenery/mesh.h"
#include "../Scenery/astro.h"
#include "../Scenery/scenery.h"
#include "../Time/fg_time.h"
#include "../Time/sunpos.h"
#include "../Weather/weather.h"


extern int show_hud;             /* HUD state */


/* General house keeping initializations */

void fgInitGeneral( void ) {
    struct GENERAL *g;

    g = &general;

    /* seed the random number generater */
    fg_srandom();

    /* determine the fg root path */
    if ( (g->root_dir = getenv("FG_ROOT")) == NULL ) {
	/* environment variable not defined */
	printf("FG_ROOT needs to be defined!  See the documentation.\n");
	exit(0);
    } 
    printf("FG_ROOT = %s\n", g->root_dir);
}


/* This is the top level init routine which calls all the other
 * initialization routines.  If you are adding a subsystem to flight
 * gear, its initialization call should located in this routine.*/

void fgInitSubsystems( void ) {
    double cur_elev;

    struct FLIGHT *f;
    struct fgTIME *t;
    struct VIEW *v;

    f = &current_aircraft.flight;
    t = &cur_time_params;
    v = &current_view;


    /****************************************************************
     * The following section sets up the flight model EOM parameters and 
     * should really be read in from one or more files.
     ****************************************************************/

    /* Globe Aiport, AZ */
    FG_Runway_longitude = -398391.28;
    FG_Runway_latitude = 120070.41;
    FG_Runway_altitude = 3234.5;
    FG_Runway_heading = 102.0 * DEG_TO_RAD;

    /* Initial Position at GLOBE airport */
    FG_Longitude = ( -398391.28 / 3600.0 ) * DEG_TO_RAD;
    FG_Latitude  = (  120070.41 / 3600.0 ) * DEG_TO_RAD;
    FG_Altitude = FG_Runway_altitude + 3.758099;
    FG_Altitude = 10000;
    
    /* Initial Position north of the city of Globe */
    /* FG_Longitude = ( -398673.28 / 3600.0 ) * DEG_TO_RAD; */
    /* FG_Latitude  = (  120625.64 / 3600.0 ) * DEG_TO_RAD; */
    /* FG_Altitude = 0.0 + 3.758099; */

    /* Initial Position: 10125 Jewell St. NE */
    /* FG_Longitude = ( -93.15 ) * DEG_TO_RAD; */
    /* FG_Latitude  = (  45.15 ) * DEG_TO_RAD; */
    /* FG_Altitude = FG_Runway_altitude + 3.758099; */

    /* A random test position */
    /* FG_Longitude = ( 88128.00 / 3600.0 ) * DEG_TO_RAD; */
    /* FG_Latitude  = ( 93312.00 / 3600.0 ) * DEG_TO_RAD; */

    printf("Initial position is: (%.4f, %.4f, %.2f)\n", 
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
    /* FG_Psi   =  0.0 * DEG_TO_RAD; */

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

    /* Initialize "time" */
    fgTimeInit(t);
    fgTimeUpdate(f, t);

    /* Initialize shared sun position and sun_vec */
    fgUpdateSunPos(scenery.center);

    /* Initialize view parameters */
    fgViewInit(v);

    /* Initialize the weather modeling subsystem */
    fgWeatherInit();

    /* Initialize the Cockpit subsystem */
    if( fgCockpitInit( current_aircraft ) == NULL )
    {
    	printf( "Error in Cockpit initialization!\n" );
    	exit( 1 );
    }

    /* Initialize Astronomical Objects */
    fgAstroInit();

    /* Initialize the Scenery Management subsystem */
    fgSceneryInit();

    /* Tell the Scenery Management system where we are so it can load
     * the correct scenery data */
    fgSceneryUpdate(FG_Latitude, FG_Longitude, FG_Altitude);


    /* I'm just sticking this here for now, it should probably move 
     * eventually */
    cur_elev = mesh_altitude(FG_Longitude * RAD_TO_DEG * 3600.0, 
			     FG_Latitude  * RAD_TO_DEG * 3600.0);
    printf("Ground elevation is %.2f meters here.\n", cur_elev);
    if ( cur_elev > -9990.0 ) {
	FG_Runway_altitude = cur_elev * METER_TO_FEET;
    }

    if ( FG_Altitude < FG_Runway_altitude ) {
	FG_Altitude = FG_Runway_altitude + 3.758099;
    }
    printf("Updated position is: (%.4f, %.4f, %.2f)\n", 
	   FG_Latitude * RAD_TO_DEG, FG_Longitude * RAD_TO_DEG, 
	   FG_Altitude * FEET_TO_METER);
    /* end of thing that I just stuck in that I should probably move */


    /* Initialize the flight model subsystem data structures base on
     * above values */
    fgFlightModelInit( FG_LARCSIM, f, 1.0 / DEFAULT_MODEL_HZ );

    /* To HUD or not to HUD */
    show_hud = 1;

    /* Joystick support */
    fgJoystickInit( 0 );
}


/* $Log$
/* Revision 1.13  1997/11/25 19:25:32  curt
/* Changes to integrate Durk's moon/sun code updates + clean up.
/*
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
