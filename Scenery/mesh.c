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

#include "../constants.h"
#include "scenery.h"
#include "mesh.h"
#include "common.h"


/* Temporary hack until we get the scenery management system running */
extern GLint mesh_hack;
extern struct mesh eg;

/* initialize the non-array mesh values */
void mesh_init(struct mesh *m) {
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
struct mesh *(new_mesh)() {
    struct mesh *mesh_ptr;

    mesh_ptr = (struct mesh *)malloc(sizeof(struct mesh));

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
void mesh_set_option_name(struct mesh *m, char *name) {
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
void mesh_set_option_value(struct mesh *m, char *value) {
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
void mesh_do_it(struct mesh *m) {
    mesh_hack = mesh2GL(m);
}


/* return the current altitude based on mesh data.  We should rewrite
 * this to interpolate exact values, but for now this is good enough */
double mesh_altitude(double lon, double lat) {
    /* we expect incoming (lon,lat) to be in arcsec for now */

    double xlocal, ylocal, dx, dy, zA, zB, elev;
    int x1, y1, z1, x2, y2, z2, x3, y3, z3;
    int xindex, yindex;
    int skip;

    skip = cur_scenery_params.terrain_skip;
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

	if ( dx > EPSILON ) {
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

	if ( dy > EPSILON ) {
	    elev = dx * (zB - zA) / dy    + zA;
	} else {
	    elev = zA;
	}
    }

    return(elev);
}


/* $Log$
/* Revision 1.17  1997/07/18 23:41:26  curt
/* Tweaks for building with Cygnus Win32 compiler.
/*
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
