/**************************************************************************
 * mesh.h -- data structures and routines for processing terrain meshes
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef MESH_H
#define MESH_H


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
    int cur_row, cur_col;
};


/* return a pointer to a new mesh structure (no data array allocated yet) */
struct mesh *(new_mesh)();

/* return a pointer to a dynamically allocated array */
float *(new_mesh_data)(int nrows, int ncols);

/* set the option name in the mesh data structure */
void mesh_set_option_name(struct mesh *m, char *name);

/* set an option value in the mesh data structure */
void mesh_set_option_value(struct mesh *m, char *value);

#endif MESH_H


/* $Log$
/* Revision 1.1  1997/05/16 16:07:05  curt
/* Initial revision.
/*
 */
