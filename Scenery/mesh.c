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


#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>  /* atof(), atoi() */
#include <string.h>

#include "mesh.h"
#include "common.h"


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
}

/* set an option value in the mesh data structure */
void mesh_set_option_value(struct mesh *m, char *value) {
    printf("Setting %s to %s\n", m->option_name, value);

    if ( strcmp(m->option_name, "origin_lat") == 0 ) {
	m->originx = atof(value);
    } else if ( strcmp(m->option_name, "origin_lon") == 0 ) {
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


/* $Log$
/* Revision 1.3  1997/05/23 15:40:41  curt
/* Added GNU copyright headers.
/*
 * Revision 1.2  1997/05/19 18:20:50  curt
 * Slight change to origin key words.
 *
 * Revision 1.1  1997/05/16 16:07:04  curt
 * Initial revision.
 *
 */
