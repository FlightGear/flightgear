/**************************************************************************
 * obj.c -- routines to handle WaveFront .obj format files.
 *
 * Written by Curtis Olson, started October 1997.
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

#include <stdio.h>
#include <GL/glut.h>

#include "obj.h"
#include "scenery.h"

#include "../constants.h"
#include "../types.h"
#include "../Math/fg_geodesy.h"
#include "../Math/polar.h"


#define MAXNODES 100000

float nodes[MAXNODES][3];


/* convert a geodetic point lon(arcsec), lat(arcsec), elev(meter) to
 * a cartesian point */
struct fgCartesianPoint geod_to_cart(float geod[3]) {
    struct fgCartesianPoint p;
    double gc_lon, gc_lat, sl_radius;

    /* printf("A geodetic point is (%.2f, %.2f, %.2f)\n", 
	   geod[0], geod[1], geod[2]); */

    gc_lon = geod[0]*ARCSEC_TO_RAD;
    fgGeodToGeoc(geod[1]*ARCSEC_TO_RAD, geod[2], &sl_radius, &gc_lat);

    /* printf("A geocentric point is (%.2f, %.2f, %.2f)\n", gc_lon, 
	   gc_lat, sl_radius+geod[2]); */

    p = fgPolarToCart(gc_lon, gc_lat, sl_radius+geod[2]);
    
    /* printf("A cart point is (%.8f, %.8f, %.8f)\n", p.x, p.y, p.z); */

    return(p);
}


/* Load a .obj file and generate the GL call list */
GLint fgObjLoad(char *path) {
    char line[256];
    static GLfloat color[4] = { 0.5, 0.5, 0.25, 1.0 };
    struct fgCartesianPoint p1, p2, p3, p4, ref;
    GLint area;
    FILE *f;
    int first, ncount, n1, n2, n3, n4;

    if ( (f = fopen(path, "r")) == NULL ) {
	printf("Cannot open file: %s\n", path);
	exit(-1);
    }

    area = glGenLists(1);
    glNewList(area, GL_COMPILE);

    /* glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color ); */
    glColor3fv(color);

    glBegin(GL_TRIANGLE_STRIP);

    first = 1;
    ncount = 1;

    while ( fgets(line, 250, f) != NULL ) {
	if ( line[0] == '#' ) {
	    /* comment -- ignore */
	} else if ( line[0] == 'v' ) {
	    /* node (vertex) */
	    if ( ncount < MAXNODES ) {
		/* printf("vertex = %s", line); */
		sscanf(line, "v %f %f %f\n", 
		       &nodes[ncount][0], &nodes[ncount][1], &nodes[ncount][2]);
		if ( ncount == 1 ) {
		    /* first node becomes the reference point */
		    ref = geod_to_cart(nodes[ncount]);
		    scenery.center = ref;
		}
		ncount++;
	    } else {
		printf("Read too many nodes ... dying :-(\n");
		exit(-1);
	    }
	} else if ( line[0] == 't' ) {
	    /* start a new triangle strip */

	    n1 = n2 = n3 = n4 = 0;

	    if ( !first ) {
		/* close out the previous structure and start the next */
		glEnd();
		glBegin(GL_TRIANGLE_STRIP);
	    } else {
		first = 0;
	    }

	    printf("new tri strip = %s", line);
	    sscanf(line, "t %d %d %d\n", &n1, &n2, &n3);

	    p1 = geod_to_cart(nodes[n1]);
	    p2 = geod_to_cart(nodes[n2]);
	    p3 = geod_to_cart(nodes[n3]);

            glNormal3d(0.0, 0.0, -1.0);

	    glVertex3d(p1.x - ref.x, p1.y - ref.y, p1.z - ref.z);
	    glVertex3d(p2.x - ref.x, p2.y - ref.y, p2.z - ref.z);
	    glVertex3d(p3.x - ref.x, p3.y - ref.y, p3.z - ref.z);

	    if ( n4 > 0 ) {
		p4 = geod_to_cart(nodes[n4]);
		glVertex3d(p4.x - ref.x, p4.y - ref.y, p4.z - ref.z);
	    }
	} else if ( line[0] == 'q' ) {
	    /* continue a triangle strip */
	    n1 = n2 = 0;

	    printf("continued tri strip = %s", line);
	    sscanf(line, "q %d %d\n", &n1, &n2);
	    printf("read %d %d\n", n1, n2);

	    p1 = geod_to_cart(nodes[n1]);
	    glVertex3d(p1.x - ref.x, p1.y - ref.y, p1.z - ref.z);

	    if ( n2 > 0 ) {
		p2 = geod_to_cart(nodes[n2]);
		glVertex3d(p2.x - ref.x, p2.y - ref.y, p2.z - ref.z);
	    }
	} else {
	    printf("Unknown line in %s = %s\n", path, line);
	}
    }

    glEnd();
    glEndList();

    fclose(f);

    return(area);
}


/* $Log$
/* Revision 1.2  1997/10/30 12:38:45  curt
/* Working on new scenery subsystem.
/*
 * Revision 1.1  1997/10/28 21:14:54  curt
 * Initial revision.
 *
 */
