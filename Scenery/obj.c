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
#include "../Math/mat3.h"
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


/* given three points defining a triangle, calculate the normal */
void calc_normal(struct fgCartesianPoint p1, struct fgCartesianPoint p2, 
		 struct fgCartesianPoint p3, double normal[3])
{
    double v1[3], v2[3];
    float temp;

    v1[0] = p2.x - p1.x; v1[1] = p2.y - p1.y; v1[2] = p2.z - p1.z;
    v2[0] = p3.x - p1.x; v2[1] = p3.y - p1.y; v2[2] = p3.z - p1.z;

    MAT3cross_product(normal, v1, v2);
    MAT3_NORMALIZE_VEC(normal,temp);

    printf("Normal = %.2f %.2f %.2f\n", normal[0], normal[1], normal[2]);
}


/* Load a .obj file and generate the GL call list */
GLint fgObjLoad(char *path) {
    char line[256];
    static GLfloat color[4] = { 0.5, 0.5, 0.25, 1.0 };
    struct fgCartesianPoint p1, p2, p3, p4, ref, last1, last2;
    GLint area;
    FILE *f;
    double normal[3];
    int first, ncount, n1, n2, n3, n4;
    int toggle = 0;

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
	    toggle = 0;

	    if ( !first ) {
		/* close out the previous structure and start the next */
		glEnd();
		glBegin(GL_TRIANGLE_STRIP);
	    } else {
		first = 0;
	    }

	    printf("new tri strip = %s", line);
	    sscanf(line, "t %d %d %d %d\n", &n1, &n2, &n3, &n4);

	    p1 = geod_to_cart(nodes[n1]);
	    p2 = geod_to_cart(nodes[n2]);
	    p3 = geod_to_cart(nodes[n3]);

	    printf("(t) = ");
	    calc_normal(p1, p2, p3, normal);
            glNormal3d(normal[0], normal[1], normal[2]);
	    glVertex3d(p1.x - ref.x, p1.y - ref.y, p1.z - ref.z);
	    glVertex3d(p2.x - ref.x, p2.y - ref.y, p2.z - ref.z);
	    glVertex3d(p3.x - ref.x, p3.y - ref.y, p3.z - ref.z);

	    last1 = p2;
	    last2 = p3;

	    if ( n4 > 0 ) {
		p4 = geod_to_cart(nodes[n4]);
		printf("(t) cont = ");
		calc_normal(last2, last1, p4, normal);
		glNormal3d(normal[0], normal[1], normal[2]);
		glVertex3d(p4.x - ref.x, p4.y - ref.y, p4.z - ref.z);

		last1 = last2;
		last2 = p4;
		toggle = 1 - toggle;
	    }
	} else if ( line[0] == 'q' ) {
	    /* continue a triangle strip */
	    n1 = n2 = 0;

	    printf("continued tri strip = %s", line);
	    sscanf(line, "q %d %d\n", &n1, &n2);
	    printf("read %d %d\n", n1, n2);

	    p1 = geod_to_cart(nodes[n1]);

	    printf("(q) = ");
	    if ( toggle ) {
		calc_normal(last1, last2, p1, normal);
	    } else {
		calc_normal(last1, p1, last2, normal);
	    }
	    glNormal3d(normal[0], normal[1], normal[2]);
	    glVertex3d(p1.x - ref.x, p1.y - ref.y, p1.z - ref.z);

	    last1 = last2;
	    last2 = p1;
	    toggle = 1 - toggle;

	    if ( n2 > 0 ) {
		p2 = geod_to_cart(nodes[n2]);

		printf("(q) cont = ");
		calc_normal(last1, last2, p2, normal);
		glNormal3d(normal[0], normal[1], normal[2]);
		glVertex3d(p2.x - ref.x, p2.y - ref.y, p2.z - ref.z);

		last1 = last2;
		last2 = p2;
		toggle = 1 - toggle;
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
/* Revision 1.3  1997/10/31 04:49:12  curt
/* Tweaking vertex orders.
/*
 * Revision 1.2  1997/10/30 12:38:45  curt
 * Working on new scenery subsystem.
 *
 * Revision 1.1  1997/10/28 21:14:54  curt
 * Initial revision.
 *
 */
