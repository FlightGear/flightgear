/**************************************************************************
 * sky.c -- model sky with an upside down "bowl"
 *
 * Written by Curtis Olson, started December 1997.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <math.h>

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Aircraft/aircraft.h>
#include <Debug/fg_debug.h>
#include <Flight/flight.h>
#include <Include/fg_constants.h>
#include <Main/views.hxx>
#include <Math/fg_random.h>
#include <Time/event.hxx>
#include <Time/fg_time.hxx>

#include "sky.hxx"


/*
#include <Include/general.h>
*/

/* in meters of course */
#define CENTER_ELEV   25000.0

#define INNER_RADIUS  50000.0
#define INNER_ELEV    20000.0

#define MIDDLE_RADIUS 70000.0
#define MIDDLE_ELEV    8000.0

#define OUTER_RADIUS  80000.0
#define OUTER_ELEV        0.0

#define BOTTOM_RADIUS 50000.0
#define BOTTOM_ELEV   -2000.0


static float inner_vertex[12][3];
static float middle_vertex[12][3];
static float outer_vertex[12][3];
static float bottom_vertex[12][3];

static float inner_color[12][4];
static float middle_color[12][4];
static float outer_color[12][4];


/* Calculate the sky structure vertices */
void fgSkyVerticesInit( void ) {
    float theta;
    int i;

    fgPrintf(FG_ASTRO, FG_INFO, "  Generating the sky dome vertices.\n");

    for ( i = 0; i < 12; i++ ) {
	theta = (i * 30.0) * DEG_TO_RAD;
	
	inner_vertex[i][0] = cos(theta) * INNER_RADIUS;
	inner_vertex[i][1] = sin(theta) * INNER_RADIUS;
	inner_vertex[i][2] = INNER_ELEV;
	
	/* printf("    %.2f %.2f\n", cos(theta) * INNER_RADIUS, 
	       sin(theta) * INNER_RADIUS); */

	middle_vertex[i][0] = cos((double)theta) * MIDDLE_RADIUS;
	middle_vertex[i][1] = sin((double)theta) * MIDDLE_RADIUS;
	middle_vertex[i][2] = MIDDLE_ELEV;
	    
	outer_vertex[i][0] = cos((double)theta) * OUTER_RADIUS;
	outer_vertex[i][1] = sin((double)theta) * OUTER_RADIUS;
	outer_vertex[i][2] = OUTER_ELEV;
	    
	bottom_vertex[i][0] = cos((double)theta) * BOTTOM_RADIUS;
	bottom_vertex[i][1] = sin((double)theta) * BOTTOM_RADIUS;
	bottom_vertex[i][2] = BOTTOM_ELEV;
    }
}


/* (Re)calculate the sky colors at each vertex */
void fgSkyColorsInit( void ) {
    fgLIGHT *l;
    double sun_angle, diff;
    double outer_param[3], outer_amt[3], outer_diff[3];
    double middle_param[3], middle_amt[3], middle_diff[3];
    int i, j;

    l = &cur_light_params;

    fgPrintf( FG_ASTRO, FG_INFO, 
	      "  Generating the sky colors for each vertex.\n" );

    /* setup for the possibility of sunset effects */
    sun_angle = l->sun_angle * RAD_TO_DEG;
    fgPrintf( FG_ASTRO, FG_INFO, "  Sun angle in degrees = %.2f\n", sun_angle);

    if ( (sun_angle > 80.0) && (sun_angle < 100.0) ) {
	/* 0.0 - 0.4 */
	outer_param[0] = (10.0 - fabs(90.0 - sun_angle)) / 25.0;
	outer_param[1] = (10.0 - fabs(90.0 - sun_angle)) / 35.0;
	outer_param[2] = 0.0;

	middle_param[0] = (10.0 - fabs(90.0 - sun_angle)) / 20.0;
	middle_param[1] = (10.0 - fabs(90.0 - sun_angle)) / 40.0;
	middle_param[2] = 0.0;

	outer_diff[0] = outer_param[0] / 6.0;
	outer_diff[1] = outer_param[1] / 6.0;
	outer_diff[2] = outer_param[2] / 6.0;

	middle_diff[0] = middle_param[0] / 6.0;
	middle_diff[1] = middle_param[1] / 6.0;
	middle_diff[2] = middle_param[2] / 6.0;
    } else {
	outer_param[0] = outer_param[1] = outer_param[2] = 0.0;
	middle_param[0] = middle_param[1] = middle_param[2] = 0.0;

	outer_diff[0] = outer_diff[1] = outer_diff[2] = 0.0;
	middle_diff[0] = middle_diff[1] = middle_diff[2] = 0.0;
    }
    /* printf("  outer_red_param = %.2f  outer_red_diff = %.2f\n", 
	   outer_red_param, outer_red_diff); */

    /* calculate transition colors between sky and fog */
    for ( j = 0; j < 3; j++ ) {
	outer_amt[j] = outer_param[j];
	middle_amt[j] = middle_param[j];
    }

    for ( i = 0; i < 6; i++ ) {
	for ( j = 0; j < 3; j++ ) {
	    diff = l->sky_color[j] - l->fog_color[j];

	    /* printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
		   l->sky_color[j], l->fog_color[j], diff); */

	    inner_color[i][j] = l->sky_color[j] - diff * 0.3;
	    middle_color[i][j] = l->sky_color[j] - diff * 0.9 + middle_amt[j];
	    outer_color[i][j] = l->fog_color[j] + outer_amt[j];

	    if ( inner_color[i][j] > 1.00 ) { inner_color[i][j] = 1.00; }
	    if ( inner_color[i][j] < 0.10 ) { inner_color[i][j] = 0.10; }
	    if ( middle_color[i][j] > 1.00 ) { middle_color[i][j] = 1.00; }
	    if ( middle_color[i][j] < 0.10 ) { middle_color[i][j] = 0.10; }
	    if ( outer_color[i][j] > 1.00 ) { outer_color[i][j] = 1.00; }
	    if ( outer_color[i][j] < 0.10 ) { outer_color[i][j] = 0.10; }
	}
	inner_color[i][3] = middle_color[i][3] = outer_color[i][3] = 
	    l->sky_color[3];

	for ( j = 0; j < 3; j++ ) {
	    outer_amt[j] -= outer_diff[j];
	    middle_amt[j] -= middle_diff[j];
	}

	/*
	printf("inner_color[%d] = %.2f %.2f %.2f %.2f\n", i, inner_color[i][0],
	       inner_color[i][1], inner_color[i][2], inner_color[i][3]);
	printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]);
	printf("outer_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       outer_color[i][0], outer_color[i][1], outer_color[i][2], 
	       outer_color[i][3]);
	*/
    }

    for ( j = 0; j < 3; j++ ) {
	outer_amt[j] = 0.0;
	middle_amt[j] = 0.0;
    }

    for ( i = 6; i < 12; i++ ) {

	for ( j = 0; j < 3; j++ ) {
	    diff = l->sky_color[j] - l->fog_color[j];

	    /* printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
		   l->sky_color[j], l->fog_color[j], diff); */

	    inner_color[i][j] = l->sky_color[j] - diff * 0.3;
	    middle_color[i][j] = l->sky_color[j] - diff * 0.9 + middle_amt[j];
	    outer_color[i][j] = l->fog_color[j] + outer_amt[j];

	    if ( inner_color[i][j] > 1.00 ) { inner_color[i][j] = 1.00; }
	    if ( inner_color[i][j] < 0.10 ) { inner_color[i][j] = 0.10; }
	    if ( middle_color[i][j] > 1.00 ) { middle_color[i][j] = 1.00; }
	    if ( middle_color[i][j] < 0.10 ) { middle_color[i][j] = 0.10; }
	    if ( outer_color[i][j] > 1.00 ) { outer_color[i][j] = 1.00; }
	    if ( outer_color[i][j] < 0.15 ) { outer_color[i][j] = 0.15; }
	}
	inner_color[i][3] = middle_color[i][3] = outer_color[i][3] = 
	    l->sky_color[3];

	for ( j = 0; j < 3; j++ ) {
	    outer_amt[j] += outer_diff[j];
	    middle_amt[j] += middle_diff[j];
	}

	/*
	printf("inner_color[%d] = %.2f %.2f %.2f %.2f\n", i, inner_color[i][0],
	       inner_color[i][1], inner_color[i][2], inner_color[i][3]);
	printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]);
	printf("outer_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       outer_color[i][0], outer_color[i][1], outer_color[i][2], 
	       outer_color[i][3]);
	*/
    }
}


/* Initialize the sky structure and colors */
void fgSkyInit( void ) {
    fgPrintf(FG_ASTRO, FG_INFO, "Initializing the sky\n");

    fgSkyVerticesInit();

    /* regester fgSkyColorsInit() as an event to be run periodically */
    global_events.Register( "fgSkyColorsInit()", fgSkyColorsInit, 
		            FG_EVENT_READY, 30000);
}


/* Draw the Sky */
void fgSkyRender( void ) {
    fgFLIGHT *f;
    fgLIGHT *l;
    fgVIEW *v;
    // float east_dot, dot, angle;
    int i;

    f = current_aircraft.flight;
    l = &cur_light_params;
    v = &current_view;

    /* printf("Rendering the sky.\n"); */

    xglPushMatrix();

#ifdef INCLUDE_THIS_CODE
    /* calculate the angle between v->surface_to_sun and
     * v->surface_east.  We do this so we can sort out the acos()
     * ambiguity.  I wish I could think of a more efficient way ... :-( */
    east_dot = MAT3_DOT_PRODUCT(v->surface_to_sun, v->surface_east);
    /* printf("  East dot product = %.2f\n", east_dot); */

    /* calculate the angle between v->surface_to_sun and
     * v->surface_south.  this is how much we have to rotate the sky
     * for it to align with the sun */
    dot = MAT3_DOT_PRODUCT(v->surface_to_sun, v->surface_south);
    /* printf("  Dot product = %.2f\n", dot); */
    if ( east_dot >= 0 ) {
	angle = acos(dot);
    } else {
	angle = -acos(dot);
    }
    /* printf("  Sky needs to rotate = %.3f rads = %.1f degrees.\n", 
	   angle, angle * RAD_TO_DEG); */
#endif

    /* Translate to view position */
    xglTranslatef( v->cur_zero_elev.x, v->cur_zero_elev.y, v->cur_zero_elev.z );
    /* printf("  Translated to %.2f %.2f %.2f\n", 
	   v->cur_zero_elev.x, v->cur_zero_elev.y, v->cur_zero_elev.z ); */

    /* Rotate to proper orientation */
    /* printf("  lon = %.2f  lat = %.2f\n", FG_Longitude * RAD_TO_DEG,
	   FG_Latitude * RAD_TO_DEG); */
    xglRotatef( FG_Longitude * RAD_TO_DEG, 0.0, 0.0, 1.0 );
    xglRotatef( 90.0 - FG_Latitude * RAD_TO_DEG, 0.0, 1.0, 0.0 );
    xglRotatef( l->sun_rotation * RAD_TO_DEG, 0.0, 0.0, 1.0 );

    /* Draw inner/center section of sky*/
    xglBegin( GL_TRIANGLE_FAN );
    xglColor4fv(l->sky_color);
    xglVertex3f(0.0, 0.0, CENTER_ELEV);
    for ( i = 0; i < 12; i++ ) {
	xglColor4fv( inner_color[i] );
	xglVertex3fv( inner_vertex[i] );
    }
    xglColor4fv( inner_color[0] );
    xglVertex3fv( inner_vertex[0] );
    xglEnd();

    // Draw the middle ring
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4fv( middle_color[i] );
	/* printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]); */
	// xglColor4f(1.0, 0.0, 0.0, 1.0);
	xglVertex3fv( middle_vertex[i] );
	xglColor4fv( inner_color[i] );
	/* printf("inner_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       inner_color[i][0], inner_color[i][1], inner_color[i][2], 
	       inner_color[i][3]); */
	// xglColor4f(0.0, 0.0, 1.0, 1.0);
	xglVertex3fv( inner_vertex[i] );
    }
    xglColor4fv( middle_color[0] );
    // xglColor4f(1.0, 0.0, 0.0, 1.0);
    xglVertex3fv( middle_vertex[0] );
    xglColor4fv( inner_color[0] );
    // xglColor4f(0.0, 0.0, 1.0, 1.0);
    xglVertex3fv( inner_vertex[0] );
    xglEnd();

    /* Draw the outer ring */
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4fv( outer_color[i] );
	xglVertex3fv( outer_vertex[i] );
	xglColor4fv( middle_color[i] );
	xglVertex3fv( middle_vertex[i] );
    }
    xglColor4fv( outer_color[0] );
    xglVertex3fv( outer_vertex[0] );
    xglColor4fv( middle_color[0] );
    xglVertex3fv( middle_vertex[0] );
    xglEnd();

    /* Draw the bottom skirt */
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4fv( l->adj_fog_color );
	xglVertex3fv( bottom_vertex[i] );
	xglColor4fv( outer_color[i] );
	xglVertex3fv( outer_vertex[i] );
    }
    xglColor4fv( l->adj_fog_color );
    xglVertex3fv( bottom_vertex[0] );
    xglColor4fv( outer_color[0] );
    xglVertex3fv( outer_vertex[0] );
    xglEnd();

    xglPopMatrix();
}


/* $Log$
/* Revision 1.7  1998/07/22 21:39:21  curt
/* Lower skirt tracks adjusted fog color, not fog color.
/*
 * Revision 1.6  1998/05/23 14:07:14  curt
 * Use new C++ events class.
 *
 * Revision 1.5  1998/04/28 01:19:02  curt
 * Type-ified fgTIME and fgVIEW
 *
 * Revision 1.4  1998/04/26 05:10:01  curt
 * "struct fgLIGHT" -> "fgLIGHT" because fgLIGHT is typedef'd.
 *
 * Revision 1.3  1998/04/25 22:06:25  curt
 * Edited cvs log messages in source files ... bad bad bad!
 *
 * Revision 1.2  1998/04/24 00:45:03  curt
 * Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
 * Fixed a bug when generating sky colors.
 *
 * Revision 1.1  1998/04/22 13:21:32  curt
 * C++ - ifing the code a bit.
 *
 * Revision 1.9  1998/04/03 21:52:50  curt
 * Converting to Gnu autoconf system.
 *
 * Revision 1.8  1998/03/09 22:47:25  curt
 * Incorporated Durk's updates.
 *
 * Revision 1.7  1998/02/19 13:05:49  curt
 * Incorporated some HUD tweaks from Michelle America.
 * Tweaked the sky's sunset/rise colors.
 * Other misc. tweaks.
 *
 * Revision 1.6  1998/02/07 15:29:32  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.5  1998/01/27 00:47:48  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.4  1998/01/26 15:54:28  curt
 * Added a "skirt" to try to help hide gaps between scenery and sky.  This will
 * have to be revisited in the future.
 *
 * Revision 1.3  1998/01/19 19:26:59  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.2  1998/01/19 18:40:17  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.1  1998/01/07 03:16:19  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.11  1997/12/30 22:22:38  curt
 * Further integration of event manager.
 *
 * Revision 1.10  1997/12/30 20:47:53  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.9  1997/12/30 13:06:57  curt
 * A couple lighting tweaks ...
 *
 * Revision 1.8  1997/12/23 04:58:38  curt
 * Tweaked the sky coloring a bit to build in structures to allow finer rgb
 * control.
 *
 * Revision 1.7  1997/12/22 23:45:48  curt
 * First stab at sunset/sunrise sky glow effects.
 *
 * Revision 1.6  1997/12/22 04:14:34  curt
 * Aligned sky with sun so dusk/dawn effects can be correct relative to the sun.
 *
 * Revision 1.5  1997/12/19 23:34:59  curt
 * Lot's of tweaking with sky rendering and lighting.
 *
 * Revision 1.4  1997/12/19 16:45:02  curt
 * Working on scene rendering order and options.
 *
 * Revision 1.3  1997/12/18 23:32:36  curt
 * First stab at sky dome actually starting to look reasonable. :-)
 *
 * Revision 1.2  1997/12/18 04:07:03  curt
 * Worked on properly translating and positioning the sky dome.
 *
 * Revision 1.1  1997/12/17 23:14:30  curt
 * Initial revision.
 * Begin work on rendering the sky. (Rather than just using a clear screen.)
 *
 */
