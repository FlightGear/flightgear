/**************************************************************************
 * astro.c
 *
 * Written by Durk Talsma. Started November 1997, for use with the flight
 * gear project.
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


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <GL/glut.h>

#include "astro.h"
#include "moon.h"
#include "orbits.h"
#include "planets.h"
#include "stars.h"
#include "sun.h"

#include "../constants.h"
#include "../general.h"

#include "../Main/views.h"
#include "../Aircraft/aircraft.h"
#include "../Time/fg_time.h"

static double prevUpdate = 0;


/* Initialize Astronomical Objects */
void fgAstroInit() {
    struct fgTIME *t;
    t = &cur_time_params;

    /* Initialize the orbital elements of sun, moon and mayor planets */
    fgSolarSystemInit(*t);

    /* Intialize the moon's position */
    fgMoonInit(); 

    /* Initialize the sun's position */
    fgSunInit();       

    /* Initialize the Stars subsystem  */
    fgStarsInit();             
}


/* Render Astronomical Objects */
void fgAstroRender() {
    struct fgFLIGHT *f;
    struct fgLIGHT *l;
    struct fgVIEW *v;
    struct fgTIME *t;
    double angle;

    f = &current_aircraft.flight;
    l = &cur_light_params;
    t = &cur_time_params;
    v = &current_view;

    /* a hack: Force sun and moon position to be updated on an hourly basis */
    if (((t->gst - prevUpdate) > 1) || (t->gst < prevUpdate)) {
	prevUpdate = t->gst;
	fgSunInit();
	fgMoonInit();
    }

    /* Disable fog effects */
    glDisable( GL_FOG );

    glPushMatrix();

    /* Translate to view position */
    glTranslatef( v->view_pos.x, v->view_pos.y, v->view_pos.z );

    /* Rotate based on gst (side real time) */
    angle = t->gst * 15.041085; /* should be 15.041085, Curt thought it was 15*/
#ifdef DEBUG
    printf("Rotating astro objects by %.2f degrees\n",angle);
#endif
    glRotatef( angle, 0.0, 0.0, -1.0 );

    /* render the stars */
    fgStarsRender();

    /* render the moon */
    fgMoonRender();

    /* render the sun */
    fgSunRender();

    glPopMatrix();

    /* reenable fog effects */
    glEnable( GL_FOG );
}


/* $Log$
/* Revision 1.4  1997/12/11 04:43:56  curt
/* Fixed sun vector and lighting problems.  I thing the moon is now lit
/* correctly.
/*
 * Revision 1.3  1997/12/10 22:37:49  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.2  1997/12/09 04:25:33  curt
 * Working on adding a global lighting params structure.
 *
 * Revision 1.1  1997/11/25 23:20:22  curt
 * Initial revision.
 *
 */
