/**************************************************************************
 * stars.c -- data structures and routines for managing and rendering stars.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Aircraft/aircraft.h>
#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Include/general.h>
#include <Main/views.hxx>
#include <Time/fg_time.hxx>

#include "orbits.hxx"
#include "planets.hxx"
#include "stars.hxx"


#define EpochStart           (631065600)
#define DaysSinceEpoch(secs) (((secs)-EpochStart)*(1.0/(24*3600)))


/* Define four structures, each with varying amounts of stars */
/* static */  GLint stars[FG_STAR_LEVELS];


/* Initialize the Star Management Subsystem */
int fgStarsInit( void ) {
    FILE *fd;
    fgGENERAL *g;
    /* struct CelestialCoord pltPos; */
    char path[1024];
    char line[256], name[256];
    char *front, *end;
    double right_ascension, declination, magnitude;
    /* double ra_save, decl_save; */
    /* double ra_save1, decl_save1; */
    int count, i, max_stars;

    fgPrintf( FG_ASTRO, FG_INFO, "Initializing stars\n");

    g = &general;

    /* build the full path name to the stars data base file */
    path[0] = '\0';
    strcat(path, g->root_dir);
    strcat(path, "/Scenery/");
    strcat(path, "Stars.dat");

    max_stars = FG_MAX_STARS;

    for ( i = 0; i < FG_STAR_LEVELS; i++ ) {
	fgPrintf( FG_ASTRO, FG_INFO,
		  "  Loading %d Stars: %s\n", max_stars, path);

	if ( (fd = fopen(path, "r")) == NULL ) {
	    fgPrintf( FG_ASTRO, FG_ALERT,
		      "Cannot open star file: '%s'\n", path);
	    return 0; // Oops, lets not even try to continue. This is critical.
	}

	stars[i] = xglGenLists(1);
	xglNewList( stars[i], GL_COMPILE );
	xglBegin( GL_POINTS );

	/* read in each line of the file */
	count = 0;
	while ( (fgets(line, 256, fd) != NULL) && (count < max_stars) ) {
	    front = line;

	    /* printf("  Read line = %s", front); */

	    /* advance to first non-whitespace character */
	    while ( (front[0] == ' ') || (front[0] == '\t') ) {
		front++;
	    }

	    /* printf("  Line length (after trimming) = %d\n", strlen(front));*/

	    if ( front[0] == '#' ) {
		/* comment */
	    } else if ( strlen(front) <= 1 ) {
		/* blank line */
	    } else {
		/* star data line */

		/* get name */
		end = front;
		while ( end[0] != ',' ) {
		    end++;
		}
		end[0] = '\0';
		strcpy(name, front);
		front = end;
		front++;

		sscanf(front, "%lf,%lf,%lf\n",
		       &right_ascension, &declination, &magnitude);

		/*
		  if ( strcmp(name, "Betelgeuse") == 0 ) {
		  printf("  *** Marking %s\n", name);
		  ra_save = right_ascension;
		  decl_save = declination;
		  }
		  */

		/*
		  if ( strcmp(name, "Alnilam") == 0 ) {
		  printf("  *** Marking %s\n", name);
		  ra_save1 = right_ascension;
		  decl_save1 = declination;
		  }
		  */

		/* scale magnitudes to (0.0 - 1.0) */
		magnitude = (0.0 - magnitude) / 5.0 + 1.0;

		/* scale magnitudes again so they look ok */
		if ( magnitude > 1.0 ) { magnitude = 1.0; }
		if ( magnitude < 0.0 ) { magnitude = 0.0; }
		magnitude =
		    magnitude * 0.7 + (((FG_STAR_LEVELS - 1) - i) * 0.1);
		/* printf("  Found star: %d %s, %.3f %.3f %.3f\n", count,
		   name, right_ascension, declination, magnitude); */

		xglColor3f( magnitude, magnitude, magnitude );
		/*xglColor3f(0,0,0);*/
		xglVertex3f( 50000.0 * cos(right_ascension) * cos(declination),
			     50000.0 * sin(right_ascension) * cos(declination),
			     50000.0 * sin(declination) );

		count++;
	    } //  valid line

	} /* while */

	fclose(fd);

	xglEnd();

	/*
	xglBegin(GL_LINE_LOOP);
        xglColor3f(1.0, 0.0, 0.0);
	xglVertex3f( 50000.0 * cos(ra_save-0.2) * cos(decl_save-0.2),
		    50000.0 * sin(ra_save-0.2) * cos(decl_save-0.2),
		    50000.0 * sin(decl_save-0.2) );
	xglVertex3f( 50000.0 * cos(ra_save+0.2) * cos(decl_save-0.2),
		    50000.0 * sin(ra_save+0.2) * cos(decl_save-0.2),
		    50000.0 * sin(decl_save-0.2) );
 	xglVertex3f( 50000.0 * cos(ra_save+0.2) * cos(decl_save+0.2),
		    50000.0 * sin(ra_save+0.2) * cos(decl_save+0.2),
		    50000.0 * sin(decl_save+0.2) );
 	xglVertex3f( 50000.0 * cos(ra_save-0.2) * cos(decl_save+0.2),
		    50000.0 * sin(ra_save-0.2) * cos(decl_save+0.2),
		    50000.0 * sin(decl_save+0.2) );
	xglEnd();
	*/

	/*
	xglBegin(GL_LINE_LOOP);
        xglColor3f(0.0, 1.0, 0.0);
	xglVertex3f( 50000.0 * cos(ra_save1-0.2) * cos(decl_save1-0.2),
		    50000.0 * sin(ra_save1-0.2) * cos(decl_save1-0.2),
		    50000.0 * sin(decl_save1-0.2) );
	xglVertex3f( 50000.0 * cos(ra_save1+0.2) * cos(decl_save1-0.2),
		    50000.0 * sin(ra_save1+0.2) * cos(decl_save1-0.2),
		    50000.0 * sin(decl_save1-0.2) );
 	xglVertex3f( 50000.0 * cos(ra_save1+0.2) * cos(decl_save1+0.2),
		    50000.0 * sin(ra_save1+0.2) * cos(decl_save1+0.2),
		    50000.0 * sin(decl_save1+0.2) );
 	xglVertex3f( 50000.0 * cos(ra_save1-0.2) * cos(decl_save1+0.2),
		    50000.0 * sin(ra_save1-0.2) * cos(decl_save1+0.2),
		    50000.0 * sin(decl_save1+0.2) );
	xglEnd();
	*/

	xglEndList();

	max_stars /= 2;
    }

    return 1;  // OK, we got here because initialization worked.
}


/* Draw the Stars */
void fgStarsRender( void ) {
    fgFLIGHT *f;
    struct fgVIEW *v;
    struct fgLIGHT *l;
    struct fgTIME *t;
    int i;

    f = current_aircraft.flight;
    l = &cur_light_params;
    t = &cur_time_params;
    v = &current_view;

    /* FG_PI_2 + 0.1 is about 6 degrees after sundown and before sunrise */

    /* t->sun_angle = 3.0; */ /* to force stars to be drawn (for testing) */

    /* render the stars */
    if ( l->sun_angle > (FG_PI_2 + 5 * DEG_TO_RAD ) ) {
	/* determine which star structure to draw */
	if ( l->sun_angle > (FG_PI_2 + 7.25 * DEG_TO_RAD ) ) {
	    i = 0;
	} else if ( l->sun_angle > (FG_PI_2 + 6.50 * DEG_TO_RAD ) ) {
	    i = 1;
	} else if ( l->sun_angle > (FG_PI_2 + 5.75 * DEG_TO_RAD ) ) {
	    i = 2;
	} else {
	    i = 3;
	}

	/* printf("RENDERING STARS = %d (night)\n", i); */

	xglCallList(stars[i]);
    } else {
	/* printf("not RENDERING STARS (day)\n"); */
    }
}


/* $Log$
/* Revision 1.2  1998/04/24 00:45:03  curt
/* Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
/* Fixed a bug when generating sky colors.
/*
 * Revision 1.1  1998/04/22 13:21:34  curt
 * C++ - ifing the code a bit.
 *
 * Revision 1.11  1998/04/18 04:13:58  curt
 * Moved fg_debug.c to it's own library.
 *
 * Revision 1.10  1998/04/03 21:52:51  curt
 * Converting to Gnu autoconf system.
 *
 * Revision 1.9  1998/03/14 00:27:12  curt
 * Updated fgGENERAL to a "type" of struct.
 *
 * Revision 1.8  1998/02/12 21:59:38  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.7  1998/02/09 15:07:48  curt
 * Minor tweaks.
 *
 * Revision 1.6  1998/02/02 20:53:23  curt
 * To version 0.29
 *
 * Revision 1.5  1998/01/27 18:35:53  curt
 * Minor tweaks.
 *
 * Revision 1.4  1998/01/27 00:47:49  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.3  1998/01/19 19:26:59  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.2  1998/01/19 18:40:18  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.1  1998/01/07 03:16:20  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.24  1997/12/30 22:22:39  curt
 * Further integration of event manager.
 *
 * Revision 1.23  1997/12/30 20:47:53  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.22  1997/12/30 16:36:53  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.21  1997/12/19 23:35:00  curt
 * Lot's of tweaking with sky rendering and lighting.
 *
 * Revision 1.20  1997/12/15 23:55:03  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.19  1997/12/12  19:53:00  curt
 * Working on lightling and material properties.
 *
 * Revision 1.18  1997/12/10 22:37:52  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.17  1997/12/09 04:25:33  curt
 * Working on adding a global lighting params structure.
 *
 * Revision 1.16  1997/11/25 19:25:38  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.15  1997/10/30 12:38:45  curt
 * Working on new scenery subsystem.
 *
 * Revision 1.14  1997/10/28 21:00:22  curt
 * Changing to new terrain format.
 *
 * Revision 1.13  1997/10/25 03:18:28  curt
 * Incorporated sun, moon, and planet position and rendering code contributed
 * by Durk Talsma.
 *
 * Revision 1.12  1997/09/23 00:29:43  curt
 * Tweaks to get things to compile with gcc-win32.
 *
 * Revision 1.11  1997/09/22 14:44:21  curt
 * Continuing to try to align stars correctly.
 *
 * Revision 1.10  1997/09/20 03:34:32  curt
 * Still trying to get those durned stars aligned properly.
 *
 * Revision 1.9  1997/09/18 16:20:09  curt
 * At dusk/dawn add/remove stars in stages.
 *
 * Revision 1.8  1997/09/16 22:14:52  curt
 * Tweaked time of day lighting equations.  Don't draw stars during the day.
 *
 * Revision 1.7  1997/09/16 15:50:31  curt
 * Working on star alignment and time issues.
 *
 * Revision 1.6  1997/09/05 14:17:31  curt
 * More tweaking with stars.
 *
 * Revision 1.5  1997/09/05 01:35:59  curt
 * Working on getting stars right.
 *
 * Revision 1.4  1997/09/04 02:17:38  curt
 * Shufflin' stuff.
 *
 * Revision 1.3  1997/08/29 17:55:28  curt
 * Worked on properly aligning the stars.
 *
 * Revision 1.2  1997/08/27 21:32:30  curt
 * Restructured view calculation code.  Added stars.
 *
 * Revision 1.1  1997/08/27 03:34:48  curt
 * Initial revision.
 *
 */
