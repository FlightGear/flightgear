/**************************************************************************
 * fg_time.h -- data structures and routines for managing time related stuff.
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


#ifndef FG_TIME_H
#define FG_TIME_H


#ifdef WIN32
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <time.h>

#include "../Include/types.h"
#include "../Flight/flight.h"


/* Define a structure containing global time parameters */
struct fgTIME {
    /* the date/time in various forms */
    time_t cur_time;     /* Unix "calendar" time in seconds */
    struct tm *gmt;      /* Break down of GMT time */

    double jd;           /* Julian date */
    double mjd;          /* modified Julian date */

    double gst;          /* side real time at prime meridian */
    double lst;          /* local side real time */

    double gst_diff;     /* the difference between the precise
			    sidereal time algorithm result and the
			    course result.  course + diff has good
			    accuracy for the short term */

    long int warp;       /* An offset in seconds from the true time.
			    Allows us to adjust the effective time of day. */

    long int warp_delta; /* How much to change the value of warp each
			    iteration.  Allows us to make time
			    progress faster than normal. */
};

extern struct fgTIME cur_time_params;


/* Lighting is time dependent so it shows up here */
/* Define a structure containing the global lighting parameters */
struct fgLIGHT {
    /* position of the sun in various forms */
    double sun_lon, sun_gc_lat;         /* in geocentric coordinates */
    struct fgCartesianPoint fg_sunpos;  /* in cartesian coordiantes */
    GLfloat sun_vec[4];                 /* (in view coordinates) */
    GLfloat sun_vec_inv[4];             /* inverse (in view coordinates) */
    double sun_angle;                   /* the angle between the sun and the 
					   local horizontal */

    /* Derived lighting values */
    GLfloat scene_ambient[3];           /* ambient component */
    GLfloat scene_diffuse[3];           /* diffuse component */
    GLfloat fog_color[4];               /* fog color */
    GLfloat sky_color[4];               /* clear screen color */
};

extern struct fgLIGHT cur_light_params;


/* Initialize the time dependent variables */
void fgTimeInit(struct fgTIME *t);

/* Update the time dependent variables */
void fgTimeUpdate(struct fgFLIGHT *f, struct fgTIME *t);


#endif /* FG_TIME_H */


/* $Log$
/* Revision 1.12  1998/01/05 18:44:37  curt
/* Add an option to advance/decrease time from keyboard.
/*
 * Revision 1.11  1997/12/19 23:35:07  curt
 * Lot's of tweaking with sky rendering and lighting.
 *
 * Revision 1.10  1997/12/15 23:55:07  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.9  1997/12/10 22:37:55  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.8  1997/12/09 04:25:38  curt
 * Working on adding a global lighting params structure.
 *
 * Revision 1.7  1997/11/25 19:25:41  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.6  1997/09/20 03:34:35  curt
 * Still trying to get those durned stars aligned properly.
 *
 * Revision 1.5  1997/09/16 15:50:31  curt
 * Working on star alignment and time issues.
 *
 * Revision 1.4  1997/09/13 02:00:08  curt
 * Mostly working on stars and generating sidereal time for accurate star
 * placement.
 *
 * Revision 1.3  1997/09/04 02:17:39  curt
 * Shufflin' stuff.
 *
 * Revision 1.2  1997/08/27 03:30:36  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.1  1997/08/13 21:56:00  curt
 * Initial revision.
 *
 */
