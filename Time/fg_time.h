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


#ifndef _FG_TIME_H
#define _FG_TIME_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


#include <config.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <time.h>

#include <Include/fg_types.h>
#include <Flight/flight.h>


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

typedef struct fg_timestamp_t {
    long seconds;
    long millis;
} fg_timestamp;


/* Portability wrap to get current time. */
void timestamp(fg_timestamp *timestamp);

/* Return duration in millis from first to last */
long timediff(fg_timestamp *first, fg_timestamp *last);

/* Return new timestamp given a time stamp and an interval to add in */
void timesum(fg_timestamp *res, fg_timestamp *start, long millis);

/* Initialize the time dependent variables */
void fgTimeInit(struct fgTIME *t);

/* Update the time dependent variables */
void fgTimeUpdate(fgFLIGHT *f, struct fgTIME *t);


#ifdef __cplusplus
}
#endif


#endif /* _FG_TIME_H */


/* $Log$
/* Revision 1.19  1998/04/21 17:01:44  curt
/* Fixed a problems where a pointer to a function was being passed around.  In
/* one place this functions arguments were defined as ( void ) while in another
/* place they were defined as ( int ).  The correct answer was ( int ).
/*
/* Prepairing for C++ integration.
/*
 * Revision 1.18  1998/04/08 23:35:40  curt
 * Tweaks to Gnu automake/autoconf system.
 *
 * Revision 1.17  1998/04/03 22:12:56  curt
 * Converting to Gnu autoconf system.
 * Centralized time handling differences.
 *
 * Revision 1.16  1998/02/07 15:29:47  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.15  1998/01/27 00:48:06  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.14  1998/01/22 02:59:43  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.13  1998/01/19 19:27:20  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.12  1998/01/05 18:44:37  curt
 * Add an option to advance/decrease time from keyboard.
 *
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
