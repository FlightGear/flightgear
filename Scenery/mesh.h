/**************************************************************************
 * mesh.h -- data structures and routines for processing terrain meshes
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


#ifndef MESH_H
#define MESH_H


#include <GL/glut.h>


struct mesh {
    /* start coordinates (in arc seconds) */
    double originx, originy;

    /* number of rows and columns */
    int rows, cols;

    /* Distance between row and column data points (in arc seconds) */
    double row_step, col_step;

    /* pointer to the actual mesh data dynamically allocated */
    float *mesh_data;

    /* a temporary values for the parser to use */
    char option_name[32];
    int do_data;
    int cur_row, cur_col;
};


/* return a pointer to a new mesh structure (no data array allocated yet) */
struct mesh *(new_mesh)();

/* initialize the non-array mesh values */
void mesh_init(struct mesh *m);

/* return a pointer to a dynamically allocated array */
float *(new_mesh_data)(int nrows, int ncols);

/* set the option name in the mesh data structure */
void mesh_set_option_name(struct mesh *m, char *name);

/* set an option value in the mesh data structure */
void mesh_set_option_value(struct mesh *m, char *value);

/* do whatever needs to be done with the mesh now that it's been
 * loaded, such as generating the OpenGL call list. */
void mesh_do_it(struct mesh *m);

/* return the current altitude based on mesh data.  We should rewrite
 * this to interpolate exact values, but for now this is good enough */
double mesh_altitude(double lon, double lat);

/* walk through mesh and make opengl calls */
GLint mesh_to_OpenGL(struct mesh *m);


#endif /* MESH_H */


/* $Log$
/* Revision 1.6  1997/08/02 19:10:15  curt
/* Incorporated mesh2GL.c into mesh.c
/*
 * Revision 1.5  1997/07/23 21:52:25  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.4  1997/07/08 18:20:14  curt
 * Working on establishing a hard ground.
 *
 * Revision 1.3  1997/06/22 21:44:41  curt
 * Working on intergrating the VRML (subset) parser.
 *
 * Revision 1.2  1997/05/23 15:40:42  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 16:07:05  curt
 * Initial revision.
 *
 */
