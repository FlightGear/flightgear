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
#include <string.h>
#include <GL/glut.h>
#include "../XGL/xgl.h"

#include "obj.h"
#include "scenery.h"

#include "../Math/mat3.h"



#define MAXNODES 100000

float nodes[MAXNODES][3];
float normals[MAXNODES][3];


/* given three points defining a triangle, calculate the normal */
void calc_normal(float p1[3], float p2[3], float p3[3], double normal[3])
{
    double v1[3], v2[3];
    float temp;

    v1[0] = p2[0] - p1[0]; v1[1] = p2[1] - p1[1]; v1[2] = p2[2] - p1[2];
    v2[0] = p3[0] - p1[0]; v2[1] = p3[1] - p1[1]; v2[2] = p3[2] - p1[2];

    MAT3cross_product(normal, v1, v2);
    MAT3_NORMALIZE_VEC(normal,temp);

    /* printf("  Normal = %.2f %.2f %.2f\n", normal[0], normal[1], normal[2]);*/
}


/* Load a .obj file and generate the GL call list */
GLint fgObjLoad(char *path) {
    char line[256], winding_str[256];
    double v1[3], v2[3], approx_normal[3], dot_prod, temp;
    struct fgCartesianPoint ref;
    GLint area;
    FILE *f;
    int first, ncount, vncount, n1, n2, n3, n4;
    int i, winding;
    int last1, last2, odd;

    if ( (f = fopen(path, "r")) == NULL ) {
	printf("Cannot open file: %s\n", path);
	exit(-1);
    }

    area = xglGenLists(1);
    xglNewList(area, GL_COMPILE);

    first = 1;
    ncount = 1;
    vncount = 1;

    while ( fgets(line, 250, f) != NULL ) {
	if ( line[0] == '#' ) {
	    /* comment -- ignore */
	} else if ( strncmp(line, "v ", 2) == 0 ) {
	    /* node (vertex) */
	    if ( ncount < MAXNODES ) {
		/* printf("vertex = %s", line); */
		sscanf(line, "v %f %f %f\n", 
		       &nodes[ncount][0], &nodes[ncount][1], &nodes[ncount][2]);
		if ( ncount == 1 ) {
		    /* first node becomes the reference point */
		    ref.x = nodes[ncount][0];
		    ref.y = nodes[ncount][1];
		    ref.z = nodes[ncount][2];
		    scenery.center = ref;
		}
		ncount++;
	    } else {
		printf("Read too many nodes ... dying :-(\n");
		exit(-1);
	    }
	} else if ( strncmp(line, "vn ", 3) == 0 ) {
	    /* vertex normal */
	    if ( vncount < MAXNODES ) {
		/* printf("vertex normal = %s", line); */
		sscanf(line, "vn %f %f %f\n", 
		       &normals[vncount][0], &normals[vncount][1], 
		       &normals[vncount][2]);
		vncount++;
	    } else {
		printf("Read too many vertex normals ... dying :-(\n");
		exit(-1);
	    }
	} else if ( strncmp(line, "winding ", 8) == 0 ) {
	    sscanf(line+8, "%s", winding_str);
	    printf("WINDING = %s\n", winding_str);

	    /* can't call xglFrontFace() between xglBegin() & xglEnd() */
	    xglEnd();
	    first = 1;

	    if ( strcmp(winding_str, "cw") == 0 ) {
		xglFrontFace( GL_CW );
		winding = 0;
	    } else {
		xglFrontFace ( GL_CCW );
		winding = 1;
	    }
	} else if ( line[0] == 't' ) {
	    /* start a new triangle strip */

	    n1 = n2 = n3 = n4 = 0;

	    if ( !first ) {
		/* close out the previous structure and start the next */
		xglEnd();
	    } else {
		first = 0;
	    }

	    printf("new tri strip = %s", line);
	    sscanf(line, "t %d %d %d %d\n", &n1, &n2, &n3, &n4);

	    /* printf("(t) = "); */

	    /* try to get the proper rotation by calculating an
             * approximate normal and seeing if it is close to the
	     * precalculated normal */
	    /* v1[0] = nodes[n2][0] - nodes[n1][0];
	    v1[1] = nodes[n2][1] - nodes[n1][1];
	    v1[2] = nodes[n2][2] - nodes[n1][2];
	    v2[0] = nodes[n3][0] - nodes[n1][0];
	    v2[1] = nodes[n3][1] - nodes[n1][1];
	    v2[2] = nodes[n3][2] - nodes[n1][2];
	    MAT3cross_product(approx_normal, v1, v2);
	    MAT3_NORMALIZE_VEC(approx_normal,temp);
	    printf("Approx normal = %.2f %.2f %.2f\n", approx_normal[0], 
		   approx_normal[1], approx_normal[2]);
	    dot_prod = MAT3_DOT_PRODUCT(normals[n1], approx_normal);
	    printf("Dot product = %.4f\n", dot_prod); */
	    /* angle = acos(dot_prod); */
	    /* printf("Normal ANGLE = %.3f rads.\n", angle); */

	    /* if ( dot_prod < -0.5 ) {
		xglFrontFace( GL_CW );
	    } else {
		xglFrontFace( GL_CCW );
	    } */

	    xglBegin(GL_TRIANGLE_STRIP);

	    if ( winding ) {
		odd = 1; 
	    } else {
		odd = 0;
	    }

	    if ( odd ) {
		calc_normal(nodes[n1], nodes[n2], nodes[n3], approx_normal);
	    } else {
		calc_normal(nodes[n2], nodes[n1], nodes[n3], approx_normal);
	    }
	    xglNormal3dv(approx_normal);

            /* xglNormal3d(normals[n1][0], normals[n1][1], normals[n1][2]); */
	    xglVertex3d(nodes[n1][0] - ref.x, nodes[n1][1] - ref.y, 
			nodes[n1][2] - ref.z);

	    /* xglNormal3d(normals[n2][0], normals[n2][1], normals[n2][2]); */
	    xglVertex3d(nodes[n2][0] - ref.x, nodes[n2][1] - ref.y, 
			nodes[n2][2] - ref.z);

	    /* xglNormal3d(normals[n3][0], normals[n3][1], normals[n3][2]); */
	    xglVertex3d(nodes[n3][0] - ref.x, nodes[n3][1] - ref.y, 
			nodes[n3][2] - ref.z);

	    odd = 1 - odd;
	    last1 = n2;
	    last2 = n3;

	    if ( n4 > 0 ) {
		if ( odd ) {
		    calc_normal(nodes[last1], nodes[last2], nodes[n4], 
				approx_normal);
		} else {
		    calc_normal(nodes[last2], nodes[last1], nodes[n4], 
				approx_normal);
		}
		calc_normal(nodes[n3], nodes[n2], nodes[n4], approx_normal);
		xglNormal3dv(approx_normal);

		/*xglNormal3d(normals[n4][0], normals[n4][1], normals[n4][2]);*/
		xglVertex3d(nodes[n4][0] - ref.x, nodes[n4][1] - ref.y, 
			    nodes[n4][2] - ref.z);

		odd = 1 - odd;
		last1 = n3;
		last2 = n4;
	    }
	} else if ( line[0] == 'f' ) {
	    /* unoptimized face */

	    if ( !first ) {
		/* close out the previous structure and start the next */
		xglEnd();
	    } else {
		first = 0;
	    }

	    xglBegin(GL_TRIANGLES);

	    /* printf("new triangle = %s", line);*/
	    sscanf(line, "f %d %d %d\n", &n1, &n2, &n3);

            xglNormal3d(normals[n1][0], normals[n1][1], normals[n1][2]);
	    xglVertex3d(nodes[n1][0] - ref.x, nodes[n1][1] - ref.y, 
			nodes[n1][2] - ref.z);

            xglNormal3d(normals[n2][0], normals[n2][1], normals[n2][2]);
	    xglVertex3d(nodes[n2][0] - ref.x, nodes[n2][1] - ref.y, 
			nodes[n2][2] - ref.z);

            xglNormal3d(normals[n3][0], normals[n3][1], normals[n3][2]);
	    xglVertex3d(nodes[n3][0] - ref.x, nodes[n3][1] - ref.y, 
			nodes[n3][2] - ref.z);
	} else if ( line[0] == 'q' ) {
	    /* continue a triangle strip */
	    n1 = n2 = 0;

	    /* printf("continued tri strip = %s ", line); */
	    sscanf(line, "q %d %d\n", &n1, &n2);
	    /* printf("read %d %d\n", n1, n2); */

	    if ( odd ) {
		calc_normal(nodes[last1], nodes[last2], nodes[n1], 
			    approx_normal);
	    } else {
		calc_normal(nodes[last2], nodes[last1], nodes[n1], 
			    approx_normal);
	    }
	    xglNormal3dv(approx_normal);

            /* xglNormal3d(normals[n1][0], normals[n1][1], normals[n1][2]); */
	    xglVertex3d(nodes[n1][0] - ref.x, nodes[n1][1] - ref.y, 
			nodes[n1][2] - ref.z);
	    
	    odd = 1 - odd;
	    last1 = last2;
	    last2 = n1;

	    if ( n2 > 0 ) {
		/* printf(" (cont)\n"); */
		if ( odd ) {
		    calc_normal(nodes[last1], nodes[last2], nodes[n2], 
				approx_normal);
		} else {
		    calc_normal(nodes[last2], nodes[last1], nodes[n2], 
				approx_normal);
		}
		xglNormal3dv(approx_normal);

		/*xglNormal3d(normals[n2][0], normals[n2][1], normals[n2][2]);*/
		xglVertex3d(nodes[n2][0] - ref.x, nodes[n2][1] - ref.y, 
			    nodes[n2][2] - ref.z);

		odd = 1 -odd;
		last1 = last2;
		last2 = n2;
	    }
	} else {
	    printf("Unknown line in %s = %s\n", path, line);
	}
    }

    xglEnd();

    /* Draw normal vectors (for visually verifying normals)*/
    /*
    xglBegin(GL_LINES);
    xglColor3f(0.0, 0.0, 0.0);
    for ( i = 0; i < ncount; i++ ) {
        xglVertex3d(nodes[i][0] - ref.x,
 		    nodes[i][1] - ref.y,
 		    nodes[i][2] - ref.z);
 	xglVertex3d(nodes[i][0] - ref.x + 500*normals[i][0],
 		    nodes[i][1] - ref.y + 500*normals[i][1],
 		    nodes[i][2] - ref.z + 500*normals[i][2]);
    } 
    xglEnd();
    */

    xglFrontFace ( GL_CCW );

    xglEndList();

    fclose(f);

    return(area);
}


/* $Log$
/* Revision 1.13  1997/12/18 23:32:36  curt
/* First stab at sky dome actually starting to look reasonable. :-)
/*
 * Revision 1.12  1997/12/17 23:13:47  curt
 * Began working on rendering the sky.
 *
 * Revision 1.11  1997/12/15 23:55:01  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.10  1997/12/12 21:41:28  curt
 * More light/material property tweaking ... still a ways off.
 *
 * Revision 1.9  1997/12/12 19:52:57  curt
 * Working on lightling and material properties.
 *
 * Revision 1.8  1997/12/10 01:19:51  curt
 * Tweaks for verion 0.15 release.
 *
 * Revision 1.7  1997/12/08 22:51:17  curt
 * Enhanced to handle ccw and cw tri-stripe winding.  This is a temporary
 * admission of defeat.  I will eventually go back and get all the stripes
 * wound the same way (ccw).
 *
 * Revision 1.6  1997/11/25 19:25:35  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.5  1997/11/15 18:16:39  curt
 * minor tweaks.
 *
 * Revision 1.4  1997/11/14 00:26:49  curt
 * Transform scenery coordinates earlier in pipeline when scenery is being
 * created, not when it is being loaded.  Precalculate normals for each node
 * as average of the normals of each containing polygon so Garoude shading is
 * now supportable.
 *
 * Revision 1.3  1997/10/31 04:49:12  curt
 * Tweaking vertex orders.
 *
 * Revision 1.2  1997/10/30 12:38:45  curt
 * Working on new scenery subsystem.
 *
 * Revision 1.1  1997/10/28 21:14:54  curt
 * Initial revision.
 *
 */
