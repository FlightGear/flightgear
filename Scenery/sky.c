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


#ifdef WIN32
#  include <windows.h>
#endif

#include <math.h>
/*
#include <stdio.h>
#include <string.h>
#include <time.h>
*/

#include <GL/glut.h>
#include "../XGL/xgl.h"

#include "sky.h"

#include "../Time/fg_time.h"

#include "../Aircraft/aircraft.h"
#include "../Flight/flight.h"
#include "../Include/constants.h"
#include "../Main/views.h"
#include "../Math/fg_random.h"
/*
#include "../Include/general.h"
*/

/* in meters of course */
#define CENTER_ELEV   25000.0
#define INNER_RADIUS  50000.0
#define INNER_ELEV    20000.0
#define MIDDLE_RADIUS 70000.0
#define MIDDLE_ELEV    8000.0
#define OUTER_RADIUS  80000.0
#define OUTER_ELEV        0.0


static float sky_inner[12][3];
static float sky_middle[12][3];
static float sky_outer[12][3];

/* Calculate the sky structure verticies */
void fgSkyInit() {
    float theta;
    int i;

    printf("Generating the sky dome vertices.\n");

    for ( i = 0; i < 12; i++ ) {
	theta = (i * 30.0) * DEG_TO_RAD;
	
	sky_inner[i][0] = cos(theta) * INNER_RADIUS;
	sky_inner[i][1] = sin(theta) * INNER_RADIUS;
	sky_inner[i][2] = INNER_ELEV;
	
	printf(" %.2f %.2f\n", cos(theta) * INNER_RADIUS, 
	       sin(theta) * INNER_RADIUS);

	sky_middle[i][0] = cos((double)theta) * MIDDLE_RADIUS;
	sky_middle[i][1] = sin((double)theta) * MIDDLE_RADIUS;
	sky_middle[i][2] = MIDDLE_ELEV;
	    
	sky_outer[i][0] = cos((double)theta) * OUTER_RADIUS;
	sky_outer[i][1] = sin((double)theta) * OUTER_RADIUS;
	sky_outer[i][2] = OUTER_ELEV;
	    
    }
}


/* Draw the Sky */
void fgSkyRender() {
    struct fgFLIGHT *f;
    struct fgLIGHT *l;
    struct fgVIEW *v;
    float inner_color[4], middle_color[4], diff, east_dot, dot, angle;
    int i;

    f = &current_aircraft.flight;
    l = &cur_light_params;
    v = &current_view;

    printf("Rendering the sky.\n");

    /*
    l->fog_color[0] = 1.0;
    l->fog_color[1] = 0.0;
    l->fog_color[2] = 0.0;
    l->fog_color[3] = 1.0;
    */

    /* calculate transition colors between sky and fog */
    for ( i = 0; i < 3; i++ ) {
	diff = l->sky_color[i] - l->fog_color[i];
	inner_color[i] = l->sky_color[i] - diff * 0.3;
	middle_color[i] = l->sky_color[i] - diff * 1.0;
    }
    inner_color[3] = middle_color[3] = l->sky_color[3];

    xglPushMatrix();

    /* calculate the angle between v->surface_to_sun and
     * v->surface_east.  We do this so we can sort out the acos()
     * ambiguity.  I wish I could think of a more efficient way ... :-( */
    east_dot = MAT3_DOT_PRODUCT(v->surface_to_sun, v->surface_east);
    printf("  East dot product = %.2f\n", east_dot);

    /* calculate the angle between v->surface_to_sun and
     * v->surface_south.  this is how much we have to rotate the sky
     * for it to align with the sun */
    dot = MAT3_DOT_PRODUCT(v->surface_to_sun, v->surface_south);
    printf("  Dot product = %.2f\n", dot);
    if ( east_dot >= 0 ) {
	angle = acos(dot);
    } else {
	angle = -acos(dot);
    }
    printf("  Sky needs to rotate = %.3f rads = %.1f degrees.\n", 
	   angle, angle * RAD_TO_DEG);

    /* Translate to view position */
    xglTranslatef( v->cur_zero_elev.x, v->cur_zero_elev.y, v->cur_zero_elev.z );
    /* printf("  Translated to %.2f %.2f %.2f\n", 
	   v->cur_zero_elev.x, v->cur_zero_elev.y, v->cur_zero_elev.z ); */

    /* Rotate to proper orientation */
    printf("  lon = %.2f  lat = %.2f\n", FG_Longitude * RAD_TO_DEG,
	   FG_Latitude * RAD_TO_DEG);
    xglRotatef( FG_Longitude * RAD_TO_DEG, 0.0, 0.0, 1.0 );
    xglRotatef( 90.0 - FG_Latitude * RAD_TO_DEG, 0.0, 1.0, 0.0 );
    xglRotatef( angle * RAD_TO_DEG, 0.0, 0.0, 1.0 );

    /* Draw inner/center section of sky*/
    xglBegin( GL_TRIANGLE_FAN );
    xglColor4fv(l->sky_color);
    xglVertex3f(0.0, 0.0, CENTER_ELEV);
    xglColor4fv( inner_color );
    for ( i = 0; i < 12; i++ ) {
	xglVertex3fv( sky_inner[i] );
    }
    xglVertex3fv( sky_inner[0] );
    xglEnd();

    /* Draw the middle ring */
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4fv( middle_color );
	xglVertex3fv( sky_middle[i] );
	xglColor4fv( inner_color );
	xglVertex3fv( sky_inner[i] );
    }
    xglColor4fv( middle_color );
      xglColor4f(1.0, 0.0, 0.0, 1.0);
    xglVertex3fv( sky_middle[0] );
    xglColor4fv( inner_color );
      xglColor4f(1.0, 0.0, 0.0, 1.0);
    xglVertex3fv( sky_inner[0] );
    xglEnd();

    /* Draw the outer ring */
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4fv( l->fog_color );
	xglVertex3fv( sky_outer[i] );
	xglColor4fv( middle_color );
	xglVertex3fv( sky_middle[i] );
    }
    xglColor4fv( l->fog_color );
    xglVertex3fv( sky_outer[0] );
    xglColor4fv( middle_color );
    xglVertex3fv( sky_middle[0] );
    xglEnd();

    xglPopMatrix();
}


/* $Log$
/* Revision 1.6  1997/12/22 04:14:34  curt
/* Aligned sky with sun so dusk/dawn effects can be correct relative to the sun.
/*
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
