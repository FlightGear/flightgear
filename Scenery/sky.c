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
/*
#include "../Include/general.h"
*/

/* in meters of course */
#define INNER_RADIUS  50000.0
#define INNER_ELEV    8000.0 /* was 20000 */
#define MIDDLE_RADIUS 70000.0
#define MIDDLE_ELEV   10000.0
#define OUTER_RADIUS  80000.0
#define OUTER_ELEV        0.0


static GLint sky;  /* sky display list */
static float sky_center[12][3];
static float sky_middle[12][3];
static float sky_outer[12][3];

/* (Re)generate the display list */
void fgSkyInit() {
    struct fgLIGHT *l;
    static int dl_exists = 0;
    float theta;
    int i;

    l = &cur_light_params;

    if ( dl_exists ) {
	xglDeleteLists(sky, 1);
    } else {
	dl_exists = 1;

	/* first time through, calculate sky vertices */
	for ( i = 0; i < 12; i++ ) {
	    theta = (i * 30.0) * DEG_TO_RAD;

	    sky_center[i][0] = cos(theta) * INNER_RADIUS;
	    sky_center[i][1] = sin(theta) * INNER_RADIUS;
	    sky_center[i][2] = INNER_ELEV;
	    
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

    printf("Creating the sky\n");

    sky = xglGenLists(1);
    xglNewList(sky, GL_COMPILE );

    /* xglMaterialfv(GL_FRONT, GL_AMBIENT, l->scene_clear);
       xglMaterialfv(GL_FRONT, GL_DIFFUSE, moon_color); */

    /* Draw inner section */
    xglBegin( GL_TRIANGLE_FAN );

    xglColor3f(0.0, 0.0, 1.0);
    xglVertex3f(0.0, 0.0, INNER_ELEV);

    xglColor3f(0.2, 0.2, 1.0);
    for ( i = 0; i < 12; i++ ) {
	xglVertex3fv( sky_center[i] );
    }
    xglVertex3fv( sky_center[0] );

    xglEnd();

    xglEndList();
}


/* Draw the Sky */
void fgSkyRender() {
    struct fgFLIGHT *f;
    struct fgVIEW *v;
    /*
    struct fgLIGHT *l;
    struct fgTIME *t;
    int i;
    */

    f = &current_aircraft.flight;
    v = &current_view;
    /*
    l = &cur_light_params;
    t = &cur_time_params;
    */

    /* FG_PI_2 + 0.1 is about 6 degrees after sundown and before sunrise */

    /* t->sun_angle = 3.0; */ /* to force stars to be drawn (for testing) */

    printf("Rendering the sky.\n");

    xglDisable( GL_LIGHTING );

    xglPushMatrix();

    /* Translate to view position */
    xglTranslatef( v->cur_zero_elev.x, v->cur_zero_elev.y, v->cur_zero_elev.z );
    /* printf("  Translated to %.2f %.2f %.2f\n", 
	   v->cur_zero_elev.x, v->cur_zero_elev.y, v->cur_zero_elev.z ); */

    printf("  lon = %.2f  lat = %.2f\n", FG_Longitude * RAD_TO_DEG,
	   FG_Latitude * RAD_TO_DEG);
    xglRotatef( FG_Longitude * RAD_TO_DEG, 0.0, 0.0, 1.0 );
    xglRotatef( 90.0 - FG_Latitude * RAD_TO_DEG, 0.0, 1.0, 0.0 );

    xglCallList( sky );

    xglPopMatrix();

    xglEnable( GL_LIGHTING );
}


/* $Log$
/* Revision 1.2  1997/12/18 04:07:03  curt
/* Worked on properly translating and positioning the sky dome.
/*
 * Revision 1.1  1997/12/17 23:14:30  curt
 * Initial revision.
 * Begin work on rendering the sky. (Rather than just using a clear screen.)
 *
 */
