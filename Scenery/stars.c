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

#include <GL/glut.h>

#include "stars.h"

#include "../general.h"


/* static struct STAR stars[FG_MAX_STARS]; */
static GLint stars;


/* Initialize the Star Management Subsystem */
void fgStarsInit() {
    FILE *fd;
    struct general_params *g;
    char path[1024];
    char line[256], name[256];
    char *tmp_ptr;
    double right_ascension, declination, magnitude;
    int count;

    g = &general;

    /* build the full path name to the stars data base file */
    path[0] = '\0';
    strcat(path, g->root_dir);
    strcat(path, "/Scenery/");
    strcat(path, "Stars.dat");

    printf("Loading Stars: %s\n", path);

    if ( (fd = fopen(path, "r")) == NULL ) {
	printf("Cannot open star file: '%s'\n", path);
	return;
    }

    stars = glGenLists(1);
    glNewList( stars, GL_COMPILE );
    glBegin( GL_POINTS );

    /* read in each line of the file */
    count = 0;
    while ( (fgets(line, 256, fd) != NULL) && (count < FG_MAX_STARS) ) {
	tmp_ptr = line;
	
	/* advance to first non-whitespace character */
	while ( (tmp_ptr[0] == ' ') || (tmp_ptr[0] == '\t') ) {
	    tmp_ptr++;
	}

	if ( tmp_ptr[0] == '#' ) {
	    /* comment */
	} else if ( strlen(tmp_ptr) == 0 ) {
	    /* blank line */
	} else {
	    /* star data line */
	    fscanf(fd, "%s %lf %lf %lf\n", 
		   name, &right_ascension, &declination, &magnitude);
	    /* printf("Found star: %d %s, %.3f %.3f %.3f\n", count,
	       name, right_ascension, declination, magnitude); */
	    count++;
	    glColor3f( magnitude, magnitude, magnitude );
	    glVertex3f( 100.0 * sin(right_ascension) * cos(declination),
			100.0 * cos(right_ascension) * cos(declination),
			100.0 * sin(declination) );

	} /* if valid line */

    } /* while */

    glEnd();
    glEndList();
}


/* Draw the Stars */
void fgStarsRender() {
    glCallList(stars);
}


/* $Log$
/* Revision 1.1  1997/08/27 03:34:48  curt
/* Initial revisio.
/*
 */
