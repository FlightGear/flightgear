/* -*- Mode: C++ -*-
 *
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


#include <config.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Include/fg_constants.h>
#include <Main/fg_debug.h>
#include <Math/mat3.h>
#include <Math/fg_random.h>
#include <Scenery/obj.h>
#include <Scenery/scenery.h>



#define MAXNODES 100000

static double nodes[MAXNODES][3];
static double normals[MAXNODES][3];


/* given three points defining a triangle, calculate the normal */
void calc_normal(double p1[3], double p2[3], double p3[3], double normal[3])
{
    double v1[3], v2[3];
    double temp;

    v1[0] = p2[0] - p1[0]; v1[1] = p2[1] - p1[1]; v1[2] = p2[2] - p1[2];
    v2[0] = p3[0] - p1[0]; v2[1] = p3[1] - p1[1]; v2[2] = p3[2] - p1[2];

    MAT3cross_product(normal, v1, v2);
    MAT3_NORMALIZE_VEC(normal,temp);

    /* fgPrintf( FG_TERRAIN, FG_DEBUG, "  Normal = %.2f %.2f %.2f\n", 
                 normal[0], normal[1], normal[2]);*/
}


#define FG_TEX_CONSTANT 128.0

float calc_lon(double x, double y, double z) {
    float tmp;
    tmp = (RAD_TO_DEG*atan2(y, x)) * FG_TEX_CONSTANT;

    // printf("lon = %.2f\n", (float)tmp);
    return (float)tmp;
}


float calc_lat(double x, double y, double z) {
    float tmp;

    tmp = (90.0 - RAD_TO_DEG*atan2( sqrt(x*x + y*y), z )) * FG_TEX_CONSTANT;

    // printf("lat = %.2f\n", (float)tmp);
    return (float)tmp;
}


/* Load a .obj file and generate the GL call list */
GLint fgObjLoad(char *path, struct fgCartesianPoint *ref, double *radius) {
    char line[256], winding_str[256];
    double approx_normal[3], normal[3], scale;
    double x, y, z, xmax, xmin, ymax, ymin, zmax, zmin;
    GLfloat sgenparams[] = { 1.0, 0.0, 0.0, 0.0 };
    GLint tile;
    FILE *f;
    int first, ncount, vncount, n1, n2, n3, n4;
    static int use_per_vertex_norms = 1;
    int winding;
    int last1, last2, odd;

    if ( (f = fopen(path, "r")) == NULL ) {
	fgPrintf(FG_TERRAIN, FG_ALERT, "Cannot open file: %s\n", path);
	return(-1);
    }

    tile = xglGenLists(1);
    xglNewList(tile, GL_COMPILE);

    /*
    xglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    xglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    xglTexGenfv(GL_S, GL_OBJECT_PLANE, sgenparams);
    xglTexGenfv(GL_T, GL_OBJECT_PLANE, sgenparams);
    // xglTexGenfv(GL_S, GL_SPHERE_MAP, 0);
    // xglTexGenfv(GL_T, GL_SPHERE_MAP, 0);
    xglEnable(GL_TEXTURE_GEN_S);
    xglEnable(GL_TEXTURE_GEN_T);
    */

    first = 1;
    ncount = 1;
    vncount = 1;

    while ( fgets(line, 250, f) != NULL ) {
	if ( line[0] == '#' ) {
	    /* comment -- ignore */
	} else if ( line[0] == '\n' ) {
	    /* empty line -- ignore */
	} else if ( strncmp(line, "v ", 2) == 0 ) {
	    /* node (vertex) */
	    if ( ncount < MAXNODES ) {
		/* fgPrintf( FG_TERRAIN, FG_DEBUG, "vertex = %s", line); */
		sscanf(line, "v %lf %lf %lf\n", &x, &y, &z);
		nodes[ncount][0] = x;
		nodes[ncount][1] = y;
		nodes[ncount][2] = z;

		/* first time through set min's and max'es */
		if ( ncount == 1 ) {
		    xmin = x;
		    xmax = x;
		    ymin = y;
		    ymax = y;
		    zmin = z;
		    zmax = z;
		}
    
		/* keep track of min/max vertex values */
		if ( x < xmin ) xmin = x;
		if ( x > xmax ) xmax = x;
		if ( y < ymin ) ymin = y;
		if ( y > ymax ) ymax = y;
		if ( z < zmin ) zmin = z;
		if ( z > zmax ) zmax = z;		

		ncount++;
	    } else {
		fgPrintf( FG_TERRAIN, FG_EXIT, 
			  "Read too many nodes ... dying :-(\n");
	    }
	} else if ( strncmp(line, "vn ", 3) == 0 ) {
	    /* vertex normal */
	    if ( vncount < MAXNODES ) {
		/* fgPrintf( FG_TERRAIN, FG_DEBUG, "vertex normal = %s", line); */
		sscanf(line, "vn %lf %lf %lf\n", 
		       &normals[vncount][0], &normals[vncount][1], 
		       &normals[vncount][2]);
		vncount++;
	    } else {
		fgPrintf( FG_TERRAIN, FG_EXIT, 
			  "Read too many vertex normals ... dying :-(\n");
	    }
	} else if ( strncmp(line, "winding ", 8) == 0 ) {
	    sscanf(line+8, "%s", winding_str);
	    fgPrintf( FG_TERRAIN, FG_DEBUG, "    WINDING = %s\n", winding_str);

	    /* can't call xglFrontFace() between xglBegin() & xglEnd() */
	    xglEnd();
	    first = 1;

	    if ( strcmp(winding_str, "cw") == 0 ) {
		xglFrontFace( GL_CW );
		winding = 0;
	    } else {
		glFrontFace ( GL_CCW );
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

	    /* fgPrintf( FG_TERRAIN, FG_DEBUG, "    new tri strip = %s", 
	       line); */
	    sscanf(line, "t %d %d %d %d\n", &n1, &n2, &n3, &n4);

	    /* fgPrintf( FG_TERRAIN, FG_DEBUG, "(t) = "); */

	    xglBegin(GL_TRIANGLE_STRIP);

	    if ( winding ) {
		odd = 1; 
		scale = 1.0;
	    } else {
		odd = 0;
		scale = 1.0;
	    }

	    if ( use_per_vertex_norms ) {
		MAT3_SCALE_VEC(normal, normals[n1], scale);
		xglNormal3dv(normal);
		xglTexCoord2f(calc_lon(nodes[n1][0], nodes[n1][1], nodes[n1][2]), calc_lat(nodes[n1][0], nodes[n1][1], nodes[n1][2]));
		xglVertex3d(nodes[n1][0], nodes[n1][1], nodes[n1][2]);

		MAT3_SCALE_VEC(normal, normals[n2], scale);
		xglNormal3dv(normal);
		xglTexCoord2f(calc_lon(nodes[n2][0], nodes[n2][1], nodes[n2][2]), calc_lat(nodes[n2][0], nodes[n2][1], nodes[n2][2]));
		xglVertex3d(nodes[n2][0], nodes[n2][1], nodes[n2][2]);

		MAT3_SCALE_VEC(normal, normals[n3], scale);
		xglNormal3dv(normal);
		xglTexCoord2f(calc_lon(nodes[n3][0], nodes[n3][1], nodes[n3][2]), calc_lat(nodes[n3][0], nodes[n3][1], nodes[n3][2]));
		xglVertex3d(nodes[n3][0], nodes[n3][1], nodes[n3][2]);
	    } else {
		if ( odd ) {
		    calc_normal(nodes[n1], nodes[n2], nodes[n3], approx_normal);
		} else {
		    calc_normal(nodes[n2], nodes[n1], nodes[n3], approx_normal);
		}
		MAT3_SCALE_VEC(normal, approx_normal, scale);
		xglNormal3dv(normal);

		xglTexCoord2f(calc_lon(nodes[n1][0], nodes[n1][1], nodes[n1][2]), calc_lat(nodes[n1][0], nodes[n1][1], nodes[n1][2]));
		xglVertex3d(nodes[n1][0], nodes[n1][1], nodes[n1][2]);
		xglTexCoord2f(calc_lon(nodes[n2][0], nodes[n2][1], nodes[n2][2]), calc_lat(nodes[n2][0], nodes[n2][1], nodes[n2][2]));
		xglVertex3d(nodes[n2][0], nodes[n2][1], nodes[n2][2]);
		xglTexCoord2f(calc_lon(nodes[n3][0], nodes[n3][1], nodes[n3][2]), calc_lat(nodes[n3][0], nodes[n3][1], nodes[n3][2]));
		xglVertex3d(nodes[n3][0], nodes[n3][1], nodes[n3][2]);
	    }

	    odd = 1 - odd;
	    last1 = n2;
	    last2 = n3;

	    if ( n4 > 0 ) {
		if ( use_per_vertex_norms ) {
		    MAT3_SCALE_VEC(normal, normals[n4], scale);
		} else {
		    calc_normal(nodes[n3], nodes[n2], nodes[n4], approx_normal);
		    MAT3_SCALE_VEC(normal, approx_normal, scale);
		}
		xglNormal3dv(normal);
		xglTexCoord2f(calc_lon(nodes[n4][0], nodes[n4][1], nodes[n4][2]), calc_lat(nodes[n4][0], nodes[n4][1], nodes[n4][2]));
		xglVertex3d(nodes[n4][0], nodes[n4][1], nodes[n4][2]);

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

	    /* fgPrintf( FG_TERRAIN, FG_DEBUG, "new triangle = %s", line);*/
	    sscanf(line, "f %d %d %d\n", &n1, &n2, &n3);

            xglNormal3d(normals[n1][0], normals[n1][1], normals[n1][2]);
	    xglTexCoord2f(calc_lon(nodes[n1][0], nodes[n1][1], nodes[n1][2]), calc_lat(nodes[n1][0], nodes[n1][1], nodes[n1][2]));
	    xglVertex3d(nodes[n1][0], nodes[n1][1], nodes[n1][2]);

            xglNormal3d(normals[n2][0], normals[n2][1], normals[n2][2]);
	    xglTexCoord2f(calc_lon(nodes[n2][0], nodes[n2][1], nodes[n2][2]), calc_lat(nodes[n2][0], nodes[n2][1], nodes[n2][2]));
	    xglVertex3d(nodes[n2][0], nodes[n2][1], nodes[n2][2]);
		
            xglNormal3d(normals[n3][0], normals[n3][1], normals[n3][2]);
	    xglTexCoord2f(calc_lon(nodes[n3][0], nodes[n3][1], nodes[n3][2]), calc_lat(nodes[n3][0], nodes[n3][1], nodes[n3][2]));
	    xglVertex3d(nodes[n3][0], nodes[n3][1], nodes[n3][2]);
	} else if ( line[0] == 'q' ) {
	    /* continue a triangle strip */
	    n1 = n2 = 0;

	    /* fgPrintf( FG_TERRAIN, FG_DEBUG, "continued tri strip = %s ", 
	       line); */
	    sscanf(line, "q %d %d\n", &n1, &n2);
	    /* fgPrintf( FG_TERRAIN, FG_DEBUG, "read %d %d\n", n1, n2); */

	    if ( use_per_vertex_norms ) {
		MAT3_SCALE_VEC(normal, normals[n1], scale);
		xglNormal3dv(normal);
	    } else {
		if ( odd ) {
		    calc_normal(nodes[last1], nodes[last2], nodes[n1], 
				approx_normal);
		} else {
		    calc_normal(nodes[last2], nodes[last1], nodes[n1], 
				approx_normal);
		}
		MAT3_SCALE_VEC(normal, approx_normal, scale);
		xglNormal3dv(normal);
	    }

	    xglTexCoord2f(calc_lon(nodes[n1][0], nodes[n1][1], nodes[n1][2]), calc_lat(nodes[n1][0], nodes[n1][1], nodes[n1][2]));
	    xglVertex3d(nodes[n1][0], nodes[n1][1], nodes[n1][2]);
    
	    odd = 1 - odd;
	    last1 = last2;
	    last2 = n1;

	    if ( n2 > 0 ) {
		/* fgPrintf( FG_TERRAIN, FG_DEBUG, " (cont)\n"); */

		if ( use_per_vertex_norms ) {
		    MAT3_SCALE_VEC(normal, normals[n2], scale);
		    xglNormal3dv(normal);
		} else {
		    if ( odd ) {
			calc_normal(nodes[last1], nodes[last2], nodes[n2], 
				    approx_normal);
		    } else {
			calc_normal(nodes[last2], nodes[last1], nodes[n2], 
				    approx_normal);
		    }
		    MAT3_SCALE_VEC(normal, approx_normal, scale);
		    xglNormal3dv(normal);
		}

		xglTexCoord2f(calc_lon(nodes[n2][0], nodes[n2][1], nodes[n2][2]), calc_lat(nodes[n2][0], nodes[n2][1], nodes[n2][2]));
		xglVertex3d(nodes[n2][0], nodes[n2][1], nodes[n2][2]);

		odd = 1 -odd;
		last1 = last2;
		last2 = n2;
	    }
	} else {
	    fgPrintf( FG_TERRAIN, FG_WARN, "Unknown line in %s = %s\n", 
		      path, line);
	}
    }

    xglEnd();

    /* Draw normal vectors (for visually verifying normals)*/
    /*
    xglBegin(GL_LINES);
    xglColor3f(0.0, 0.0, 0.0);
    for ( i = 0; i < ncount; i++ ) {
        xglVertex3d(nodes[i][0],
 		    nodes[i][1] ,
 		    nodes[i][2]);
 	xglVertex3d(nodes[i][0] + 500*normals[i][0],
 		    nodes[i][1] + 500*normals[i][1],
 		    nodes[i][2] + 500*normals[i][2]);
    } 
    xglEnd();
    */

    // xglDisable(GL_TEXTURE_GEN_S);
    // xglDisable(GL_TEXTURE_GEN_T);

    xglFrontFace ( GL_CCW );

    xglEndList();

    fclose(f);

    /* reference point is the "center" */
    ref->x = (xmin + xmax) / 2.0;
    ref->y = (ymin + ymax) / 2.0;
    ref->z = (zmin + zmax) / 2.0;

    return(tile);
}


/* $Log$
/* Revision 1.26  1998/04/03 22:11:36  curt
/* Converting to Gnu autoconf system.
/*
 * Revision 1.25  1998/03/14 00:30:50  curt
 * Beginning initial terrain texturing experiments.
 *
 * Revision 1.24  1998/02/09 21:30:18  curt
 * Fixed a nagging problem with terrain tiles not "quite" matching up perfectly.
 *
 * Revision 1.23  1998/02/09 15:07:52  curt
 * Minor tweaks.
 *
 * Revision 1.22  1998/02/01 03:39:54  curt
 * Minor tweaks.
 *
 * Revision 1.21  1998/01/31 00:43:25  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.20  1998/01/29 00:51:39  curt
 * First pass at tile cache, dynamic tile loading and tile unloading now works.
 *
 * Revision 1.19  1998/01/27 03:26:42  curt
 * Playing with new fgPrintf command.
 *
 * Revision 1.18  1998/01/19 19:27:16  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.17  1998/01/13 00:23:10  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.16  1997/12/30 23:09:40  curt
 * Worked on winding problem without luck, so back to calling glFrontFace()
 * 3 times for each scenery area.
 *
 * Revision 1.15  1997/12/30 20:47:51  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.14  1997/12/30 01:38:46  curt
 * Switched back to per vertex normals and smooth shading for terrain.
 *
 * Revision 1.13  1997/12/18 23:32:36  curt
 * First stab at sky dome actually starting to look reasonable. :-)
 *
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
