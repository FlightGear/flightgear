/**************************************************************************
 * mesh.c -- data structures and routines for processing terrain meshes
 *
 * Written by Curtis Olson, started May 1997.
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


#ifndef __CYGWIN32__
#  include <malloc.h>
#endif

#ifdef WIN32
#  include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>  /* atof(), atoi() */
#include <string.h>

#include <GL/glut.h>

#include "../Include/constants.h"
#include "../Include/types.h"
#include "../Math/fg_geodesy.h"
#include "../Math/fg_random.h"
#include "../Math/mat3.h"
#include "../Math/polar.h"

#include "mesh.h"
#include "common.h"
#include "scenery.h"


/* Temporary hack until we get the scenery management system running */
extern GLint area_terrain;
extern struct MESH eg;

/* initialize the non-array mesh values */
void mesh_init(struct MESH *m) {
    m->originx = 0.0;
    m->originy = 0.0;

    m->rows = 0;
    m->cols = 0;

    m->row_step = 0.0;
    m->col_step = 0.0;

    m->cur_row = 0;
    m->cur_col = 0;
    m->do_data = 0;
}


/* return a pointer to a new mesh structure (no data array allocated yet) */
struct MESH *(new_mesh)() {
    struct MESH *mesh_ptr;

    mesh_ptr = (struct MESH *)malloc(sizeof(struct MESH));

    if ( mesh_ptr == 0 ) {
	printf("Virtual memory exceeded\n");
	exit(-1);
    }

    mesh_ptr->cur_row = 0;
    mesh_ptr->cur_col = 0;

    return(mesh_ptr);
}


/* return a pointer to a dynamically allocated array */
float *(new_mesh_data)(int nrows, int ncols) {
    float *mesh_data_ptr;

    mesh_data_ptr = (float *)malloc(nrows * ncols * sizeof(float));

    if ( mesh_data_ptr == 0 ) {
	printf("Virtual memory exceeded\n");
	exit(-1);
    }

    printf("Allocated float(%d, %d)\n", nrows, ncols);

    return(mesh_data_ptr);
}


/* set the option name in the mesh data structure */
void mesh_set_option_name(struct MESH *m, char *name) {
    if ( strlen(name) < MAX_IDENT_LEN ) {
	strcpy(m->option_name, name);
    } else {
	strncpy(m->option_name, name, MAX_IDENT_LEN - 1);
	m->option_name[MAX_IDENT_LEN - 1] = '\0';
    }
    if ( strcmp(m->option_name, "do_data") == 0 ) {
	m->do_data = 1;
    } else {
	m->do_data = 0;
    }
}


/* set an option value in the mesh data structure */
void mesh_set_option_value(struct MESH *m, char *value) {
    /* printf("Setting %s to %s\n", m->option_name, value); */

    if ( m->do_data ) {
	/* mesh data is a pseudo 2d array */
	/* printf("Setting mesh_data[%d][%d] to %s\n", m->cur_row, m->cur_col, 
	       value); */
	m->mesh_data[m->cur_row * m->cols + m->cur_col] = atof(value);
	m->cur_col++;
	if ( m->cur_col >= m->cols ) {
	    m->cur_col = 0;
	    m->cur_row++;
	    if ( m->cur_row > m->rows ) {
		m->do_data = 0;
	    }
	}
    } else if ( strcmp(m->option_name, "origin_lon") == 0 ) {
	m->originx = atof(value);
    } else if ( strcmp(m->option_name, "origin_lat") == 0 ) {
	m->originy = atof(value);
    } else if ( strcmp(m->option_name, "rows") == 0 ) {
	m->rows = atoi(value);
    } else if ( strcmp(m->option_name, "cols") == 0 ) {
	m->cols = atoi(value);
    } else if ( strcmp(m->option_name, "row_step") == 0 ) {
	m->row_step = atof(value);
    } else if ( strcmp(m->option_name, "col_step") == 0 ) {
	m->col_step = atof(value);
    } else {
	printf("Unknown option %s with value %s, ignoring ...\n", 
	       m->option_name, value);
    }
}


/* do whatever needs to be done with the mesh now that it's been
   loaded, such as generating the OpenGL call list. */
void mesh_do_it(struct MESH *m) {
    area_terrain = mesh_to_OpenGL(m);
}


/* return the current altitude based on mesh data.  We should rewrite
 * this to interpolate exact values, but for now this is good enough */
double mesh_altitude(double lon, double lat) {
    /* we expect incoming (lon,lat) to be in arcsec for now */

    double xlocal, ylocal, dx, dy, zA, zB, elev;
    int x1, y1, z1, x2, y2, z2, x3, y3, z3;
    int xindex, yindex;
    int skip;

    skip = scenery.terrain_skip;
    /* determine if we are in the lower triangle or the upper triangle 
       ______
       |   /|
       |  / |
       | /  |
       |/   |
       ------

       then calculate our end points
     */

    xlocal = (lon - eg.originx) / eg.col_step;
    ylocal = (lat - eg.originy) / eg.row_step;

    xindex = (int)(xlocal / skip) * skip;
    yindex = (int)(ylocal / skip) * skip;

    if ( (xindex < 0) || (xindex + skip >= eg.cols) ||
	 (yindex < 0) || (yindex + skip >= eg.rows) ) {
	return(-9999);
    }

    dx = xlocal - xindex;
    dy = ylocal - yindex;

    if ( dx > dy ) {
	/* lower triangle */
	/* printf("  Lower triangle\n"); */

	x1 = xindex; 
	y1 = yindex; 
	z1 = eg.mesh_data[y1 * eg.cols + x1];

	x2 = xindex + skip; 
	y2 = yindex; 
	z2 = eg.mesh_data[y2 * eg.cols + x2];
				  
	x3 = xindex + skip; 
	y3 = yindex + skip; 
	z3 = eg.mesh_data[y3 * eg.cols + x3];

	/* printf("  dx = %.2f  dy = %.2f\n", dx, dy);
	printf("  (x1,y1,z1) = (%d,%d,%d)\n", x1, y1, z1);
	printf("  (x2,y2,z2) = (%d,%d,%d)\n", x2, y2, z2);
	printf("  (x3,y3,z3) = (%d,%d,%d)\n", x3, y3, z3); */

	zA = dx * (z2 - z1) / skip + z1;
	zB = dx * (z3 - z1) / skip + z1;
	
	/* printf("  zA = %.2f  zB = %.2f\n", zA, zB); */

	if ( dx > FG_EPSILON ) {
	    elev = dy * (zB - zA) / dx + zA;
	} else {
	    elev = zA;
	}
    } else {
	/* upper triangle */
	/* printf("  Upper triangle\n"); */

	x1 = xindex; 
	y1 = yindex; 
	z1 = eg.mesh_data[y1 * eg.cols + x1];

	x2 = xindex; 
	y2 = yindex + skip; 
	z2 = eg.mesh_data[y2 * eg.cols + x2];
				  
	x3 = xindex + skip; 
	y3 = yindex + skip; 
	z3 = eg.mesh_data[y3 * eg.cols + x3];

	/* printf("  dx = %.2f  dy = %.2f\n", dx, dy);
	printf("  (x1,y1,z1) = (%d,%d,%d)\n", x1, y1, z1);
	printf("  (x2,y2,z2) = (%d,%d,%d)\n", x2, y2, z2);
	printf("  (x3,y3,z3) = (%d,%d,%d)\n", x3, y3, z3); */
 
	zA = dy * (z2 - z1) / skip + z1;
	zB = dy * (z3 - z1) / skip + z1;
	
	/* printf("  zA = %.2f  zB = %.2f\n", zA, zB );
	printf("  xB - xA = %.2f\n", eg.col_step * dy / eg.row_step); */

	if ( dy > FG_EPSILON ) {
	    elev = dx * (zB - zA) / dy    + zA;
	} else {
	    elev = zA;
	}
    }

    return(elev);
}


/* walk through mesh and make opengl calls */
GLint mesh_to_OpenGL(struct MESH *m) {
    GLint mesh;
    /* static GLfloat color[4] = { 0.5, 0.4, 0.25, 1.0 }; */ /* dark desert */
    /* static GLfloat color[4] = { 0.5, 0.5, 0.25, 1.0 }; */
    static GLfloat color[4] = { 1.0, 0.0, 0.0, 1.0 };
    double centerx, centery;
    double randx, randy;

    float x1, y1, x2, y2, z11, z12, z21, z22;
    struct fgCartesianPoint p11, p12, p21, p22, c;
    double gc_lon, gc_lat, sl_radius;

    MAT3vec v1, v2, normal; 
    int i, j, istep, jstep, iend, jend;
    float temp;

    printf("In mesh2GL(), generating GL call list.\n");

    /* Detail level.  This is how big a step we take as we walk
     * through the DEM data set.  This value is initialized in
     * .../Scenery/scenery.c:fgSceneryInit() */
    istep = jstep = scenery.terrain_skip ;

    centerx = m->originx + (m->rows * m->row_step) / 2.0;
    centery = m->originy + (m->cols * m->col_step) / 2.0;
    fgGeodToGeoc(centery*ARCSEC_TO_RAD, 0, &sl_radius, &gc_lat);
    c = fgPolarToCart(centerx*ARCSEC_TO_RAD, gc_lat, sl_radius);
    scenery.center = c;

    mesh = glGenLists(1);
    glNewList(mesh, GL_COMPILE);

    /* glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color ); */
    glColor3fv(color);

    iend = m->cols - 1;
    jend = m->rows - 1;
    
    y1 = m->originy;
    y2 = y1 + (m->col_step * istep);
    
    for ( i = 0; i < iend; i += istep ) {
        x1 = m->originx;
        x2 = x1 + (m->row_step * jstep);

        glBegin(GL_TRIANGLE_STRIP);

        for ( j = 0; j < jend; j += jstep ) {
            z11 = m->mesh_data[i         * m->cols + j        ];
            z12 = m->mesh_data[(i+istep) * m->cols + j        ];
            z21 = m->mesh_data[i         * m->cols + (j+jstep)];
            z22 = m->mesh_data[(i+istep) * m->cols + (j+jstep)];

            /* printf("A geodetic point is (%.2f, %.2f, %.2f)\n", 
               x1, y1, z11); */
            gc_lon = x1*ARCSEC_TO_RAD;
            fgGeodToGeoc(y1*ARCSEC_TO_RAD, z11, &sl_radius, &gc_lat);
            /* printf("A geocentric point is (%.2f, %.2f, %.2f)\n", gc_lon, 
               gc_lat, sl_radius+z11); */
            p11 = fgPolarToCart(gc_lon, gc_lat, sl_radius+z11);
            /* printf("A cart point is (%.8f, %.8f, %.8f)\n", 
               p11.x, p11.y, p11.z); */

            gc_lon = x1*ARCSEC_TO_RAD;
            fgGeodToGeoc(y2*ARCSEC_TO_RAD, z12, &sl_radius, &gc_lat);
            p12 = fgPolarToCart(gc_lon, gc_lat, sl_radius+z12);

            gc_lon = x2*ARCSEC_TO_RAD;
            fgGeodToGeoc(y1*ARCSEC_TO_RAD, z21, &sl_radius, &gc_lat);
            p21 = fgPolarToCart(gc_lon, gc_lat, sl_radius+z21);

            gc_lon = x2*ARCSEC_TO_RAD;
            fgGeodToGeoc(y2*ARCSEC_TO_RAD, z22, &sl_radius, &gc_lat);
            p22 = fgPolarToCart(gc_lon, gc_lat, sl_radius+z22);

            v1[0] = p22.y - p11.y; v1[1] = p22.z - p11.z; v1[2] = z22 - z11;
            v2[0] = p12.y - p11.y; v2[1] = p12.z - p11.z; v2[2] = z12 - z11;
            MAT3cross_product(normal, v1, v2);
            MAT3_NORMALIZE_VEC(normal,temp);
            glNormal3d(normal[0], normal[1], normal[2]);
            /* printf("normal 1 = (%.2f %.2f %.2f\n", normal[0], normal[1], 
                   normal[2]); */
            
            if ( j == 0 ) {
                /* first time through */
                glVertex3d(p12.x - c.x, p12.y - c.y, p12.z - c.z);
                glVertex3d(p11.x - c.x, p11.y - c.y, p11.z - c.z);
            }
            
            glVertex3d(p22.x - c.x, p22.y - c.y, p22.z - c.z);
    
            v2[0] = p21.y - p11.y; v2[1] = p21.z - p11.z; v2[2] = z21 - z11;
            MAT3cross_product(normal, v2, v1);
            MAT3_NORMALIZE_VEC(normal,temp);
            glNormal3d(normal[0], normal[1], normal[2]);
            /* printf("normal 2 = (%.2f %.2f %.2f\n", normal[0], normal[1], 
                   normal[2]); */

            glVertex3d(p21.x - c.x, p21.y - c.y, p21.z - c.z);

            x1 += m->row_step * jstep;
            x2 += m->row_step * jstep;
        }
        glEnd();

        y1 += m->col_step * istep;
        y2 += m->col_step * istep;
    }

    /* this will go, it's only here for testing/debugging */

    for ( i = 0; i < 200; i++ ) {
        randx = fg_random() * 3600.0;
        randy = fg_random() * 3600.0;

        /* mesh_make_test_object(m->originx + randx, m->originy + randy); */
    }

    glEndList();

    return(mesh);
}


/* $Log$
/* Revision 1.25  1998/01/07 03:31:27  curt
/* Miscellaneous tweaks.
/*
 * Revision 1.24  1997/12/15 23:54:59  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.23  1997/10/30 12:38:44  curt
 * Working on new scenery subsystem.
 *
 * Revision 1.22  1997/10/28 21:00:21  curt
 * Changing to new terrain format.
 *
 * Revision 1.21  1997/08/27 03:30:27  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.20  1997/08/19 23:55:08  curt
 * Worked on better simulating real lighting.
 *
 * Revision 1.19  1997/08/06 00:24:28  curt
 * Working on correct real time sun lighting.
 *
 * Revision 1.18  1997/08/02 19:10:14  curt
 * Incorporated mesh2GL.c into mesh.c
 *
 * Revision 1.17  1997/07/18 23:41:26  curt
 * Tweaks for building with Cygnus Win32 compiler.
 *
 * Revision 1.16  1997/07/16 20:04:51  curt
 * Minor tweaks to aid Win32 port.
 *
 * Revision 1.15  1997/07/14 16:26:04  curt
 * Testing/playing -- placed objects randomly across the entire terrain.
 *
 * Revision 1.14  1997/07/12 04:01:14  curt
 * Added #include <Windows32/Base.h> to help Win32 compiling.
 *
 * Revision 1.13  1997/07/12 02:27:11  curt
 * Looking at potential scenery transformation/coordinate system problems.
 *
 * Revision 1.12  1997/07/11 03:23:19  curt
 * Solved some scenery display/orientation problems.  Still have a positioning
 * (or transformation?) problem.
 *
 * Revision 1.11  1997/07/11 01:30:02  curt
 * More tweaking of terrian floor.
 *
 * Revision 1.10  1997/07/10 04:26:38  curt
 * We now can interpolated ground elevation for any position in the grid.  We
 * can use this to enforce a "hard" ground.  We still need to enforce some
 * bounds checking so that we don't try to lookup data points outside the
 * grid data set.
 *
 * Revision 1.9  1997/07/10 02:22:10  curt
 * Working on terrain elevation interpolation routine.
 *
 * Revision 1.8  1997/07/09 21:31:15  curt
 * Working on making the ground "hard."
 *
 * Revision 1.7  1997/07/08 18:20:13  curt
 * Working on establishing a hard ground.
 *
 * Revision 1.6  1997/06/29 21:16:49  curt
 * More twiddling with the Scenery Management system.
 *
 * Revision 1.5  1997/06/22 21:44:41  curt
 * Working on intergrating the VRML (subset) parser.
 *
 * Revision 1.4  1997/05/30 19:30:17  curt
 * The LaRCsim flight model is starting to look like it is working.
 *
 * Revision 1.3  1997/05/23 15:40:41  curt
 * Added GNU copyright headers.
 *
 * Revision 1.2  1997/05/19 18:20:50  curt
 * Slight change to origin key words.
 *
 * Revision 1.1  1997/05/16 16:07:04  curt
 * Initial revision.
 *
 */
