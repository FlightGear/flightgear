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


#ifdef WIN32
#  include <windows.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <GL/glut.h>

#include "stars.h"

#include "../constants.h"
#include "../general.h"

#include "../GLUT/views.h"
#include "../Aircraft/aircraft.h"
#include "../Time/fg_time.h"


#define EpochStart           (631065600)
#define DaysSinceEpoch(secs) (((secs)-EpochStart)*(1.0/(24*3600)))


/* Define four structures, each with varying amounts of stars */
static GLint stars[FG_STAR_LEVELS];


/* Initialize the Star Management Subsystem */
void fgStarsInit() {
    FILE *fd;
    struct GENERAL *g;
    char path[1024];
    char line[256], name[256];
    char *front, *end;
    double right_ascension, declination, magnitude;
    double ra_save, decl_save;
    double ra_save1, decl_save1;
    int count, i, max_stars;

    g = &general;

    /* build the full path name to the stars data base file */
    path[0] = '\0';
    strcat(path, g->root_dir);
    strcat(path, "/Scenery/");
    strcat(path, "Stars.dat");

    max_stars = FG_MAX_STARS;

    for ( i = 0; i < FG_STAR_LEVELS; i++ ) {
	printf("Loading %d Stars: %s\n", max_stars, path);

	if ( (fd = fopen(path, "r")) == NULL ) {
	    printf("Cannot open star file: '%s'\n", path);
	    return;
	}
	
	stars[i] = glGenLists(1);
	glNewList( stars[i], GL_COMPILE );
	glBegin( GL_POINTS );

	/* read in each line of the file */
	count = 0;
	while ( (fgets(line, 256, fd) != NULL) && (count < max_stars) ) {
	    front = line;

	    /* printf("Read line = %s", front); */

	    /* advance to first non-whitespace character */
	    while ( (front[0] == ' ') || (front[0] == '\t') ) {
		front++;
	    }

	    /* printf("Line length (after trimming) = %d\n", strlen(front)); */

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

		if ( strcmp(name, "Betelgeuse") == 0 ) {
		    printf("  *** Marking %s\n", name);
		    ra_save = right_ascension;
		    decl_save = declination;
		}

		if ( strcmp(name, "Alnilam") == 0 ) {
		    printf("  *** Marking %s\n", name);
		    ra_save1 = right_ascension;
		    decl_save1 = declination;
		}

		/* scale magnitudes to (0.0 - 1.0) */
		magnitude = (0.0 - magnitude) / 5.0 + 1.0;

		/* scale magnitudes again so they look ok */
		if ( magnitude > 1.0 ) { magnitude = 1.0; }
		if ( magnitude < 0.0 ) { magnitude = 0.0; }
		magnitude = 
		    magnitude * 0.7 + (((FG_STAR_LEVELS - 1) - i) * 0.1);
		/* printf("Found star: %d %s, %.3f %.3f %.3f\n", count,
		       name, right_ascension, declination, magnitude); */

		glColor3f( magnitude, magnitude, magnitude );
		glVertex3f( 190000.0 * cos(right_ascension) * cos(declination),
			    190000.0 * sin(right_ascension) * cos(declination),
			    190000.0 * sin(declination) );

		count++;
	    } /* if valid line */

	} /* while */

	fclose(fd);

	glEnd();


	glBegin(GL_LINE_LOOP);
        glColor3f(1.0, 0.0, 0.0);
	glVertex3f( 190000.0 * cos(ra_save-0.2) * cos(decl_save-0.2),
		    190000.0 * sin(ra_save-0.2) * cos(decl_save-0.2),
		    190000.0 * sin(decl_save-0.2) );
	glVertex3f( 190000.0 * cos(ra_save+0.2) * cos(decl_save-0.2),
		    190000.0 * sin(ra_save+0.2) * cos(decl_save-0.2),
		    190000.0 * sin(decl_save-0.2) );
 	glVertex3f( 190000.0 * cos(ra_save+0.2) * cos(decl_save+0.2),
		    190000.0 * sin(ra_save+0.2) * cos(decl_save+0.2),
		    190000.0 * sin(decl_save+0.2) );
 	glVertex3f( 190000.0 * cos(ra_save-0.2) * cos(decl_save+0.2),
		    190000.0 * sin(ra_save-0.2) * cos(decl_save+0.2),
		    190000.0 * sin(decl_save+0.2) );
	glEnd();

	glBegin(GL_LINE_LOOP);
        glColor3f(0.0, 1.0, 0.0);
	glVertex3f( 190000.0 * cos(ra_save1-0.2) * cos(decl_save1-0.2),
		    190000.0 * sin(ra_save1-0.2) * cos(decl_save1-0.2),
		    190000.0 * sin(decl_save1-0.2) );
	glVertex3f( 190000.0 * cos(ra_save1+0.2) * cos(decl_save1-0.2),
		    190000.0 * sin(ra_save1+0.2) * cos(decl_save1-0.2),
		    190000.0 * sin(decl_save1-0.2) );
 	glVertex3f( 190000.0 * cos(ra_save1+0.2) * cos(decl_save1+0.2),
		    190000.0 * sin(ra_save1+0.2) * cos(decl_save1+0.2),
		    190000.0 * sin(decl_save1+0.2) );
 	glVertex3f( 190000.0 * cos(ra_save1-0.2) * cos(decl_save1+0.2),
		    190000.0 * sin(ra_save1-0.2) * cos(decl_save1+0.2),
		    190000.0 * sin(decl_save1+0.2) );
	glEnd();
       
	glEndList();

	max_stars /= 2;
    }
}


/* Draw the Stars */
void fgStarsRender() {
    struct FLIGHT *f;
    struct VIEW *v;
    struct fgTIME *t;
    double angle;
    static double warp = 0;
    int i;

    f = &current_aircraft.flight;
    t = &cur_time_params;
    v = &current_view;

    /* FG_PI_2 + 0.1 is about 6 degrees after sundown and before sunrise */

    t->sun_angle = 3.0; /* force stars (for testing) */

    if ( t->sun_angle > (FG_PI_2 + 5 * DEG_TO_RAD ) ) {
	/* determine which star structure to draw */
	if ( t->sun_angle > (FG_PI_2 + 7.25 * DEG_TO_RAD ) ) {
	    i = 0;
	} else if ( t->sun_angle > (FG_PI_2 + 6.50 * DEG_TO_RAD ) ) {
	    i = 1;
	} else if ( t->sun_angle > (FG_PI_2 + 5.75 * DEG_TO_RAD ) ) {
	    i = 2;
	} else {
	    i = 3;
	}

	printf("RENDERING STARS = %d (night)\n", i);

	glDisable( GL_FOG );
	glDisable( GL_LIGHTING );
	glPushMatrix();

	glTranslatef( v->view_pos.x, v->view_pos.y, v->view_pos.z );

	angle = t->gst * 15.0;  /* 15 degrees per hour rotation */
	/* warp += 1.0; */
	/* warp = 15.0; */
	warp = 0.0;
	glRotatef( (angle+warp), 0.0, 0.0, -1.0 );
	printf("Rotating stars by %.2f degrees + %.2f degrees\n",angle,warp);

	glCallList(stars[i]);

	glPopMatrix();
	glEnable( GL_LIGHTING );
	glEnable( GL_FOG );
    } else {
	printf("not RENDERING STARS (day)\n");
    }
}


/* $Log$
/* Revision 1.11  1997/09/22 14:44:21  curt
/* Continuing to try to align stars correctly.
/*
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
