/**************************************************************************
 * geometry.c -- data structures and routines for processing vrml geometry
 *
 * Written by Curtis Olson, started June 1997.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geometry.h"
#include "mesh.h"


static vrmlGeometryType;
static struct mesh eg;


/* Begin a new vrml geometry statement */
int vrmlInitGeometry(char *type) {
    printf("Beginning geometry item = %s\n", type);

    if ( strcmp(type, "ElevationGrid") == 0 ) {
	vrmlGeometryType = VRML_ELEV_GRID;
	mesh_init(&eg);
    } else {
	vrmlGeometryType = -1;
	printf("Unimplemented geometry type = %s\n", type);
    }

    return(vrmlGeometryType);
}


/* Set current vrml geometry option name */
void vrmlGeomOptionName(char *name) {
    /* printf("Found vrml geometry option = %s\n", name); */

    switch(vrmlGeometryType) {
    case VRML_ELEV_GRID:
	if ( strcmp(name, "xDimension") == 0 ) {
	    mesh_set_option_name(&eg, "rows");
	} else if ( strcmp(name, "zDimension") == 0 ) {
	    mesh_set_option_name(&eg, "cols");
	} else if ( strcmp(name, "xSpacing") == 0 ) {
	    mesh_set_option_name(&eg, "row_step");
	} else if ( strcmp(name, "zSpacing") == 0 ) {
	    mesh_set_option_name(&eg, "col_step");
	} else if ( strcmp(name, "height") == 0 ) {
	    eg.mesh_data = new_mesh_data(eg.rows, eg.cols);
	    mesh_set_option_name(&eg, "do_data");
	} else {
	    printf("Unknown ElevationGrid option = %s\n", name);
	}
	break;
    }
}


/* Set current vrml geometry value */
void vrmlGeomOptionsValue(char *value) {
    /* printf("Found vrml geometry value = %s\n", value); */

    switch(vrmlGeometryType) {
    case VRML_ELEV_GRID:
	mesh_set_option_value(&eg, value);
    }
}


/* We've finished parsing the current geometry.  Now do whatever needs
 * to be done with it (like generating the OpenGL call list for
 * instance */
void vrmlHandleGeometry() {
    switch(vrmlGeometryType) {
    case VRML_ELEV_GRID:
	mesh_do_it(&eg);
    }
}


/* Finish up the current vrml geometry statement */
int vrmlFreeGeometry() {
    printf("Freeing geometry type = %d\n", vrmlGeometryType);

    switch(vrmlGeometryType) {
    case VRML_ELEV_GRID:
	free(eg.mesh_data);
    }
    return(vrmlGeometryType);
}


/* $Log$
/* Revision 1.1  1997/06/29 21:16:48  curt
/* More twiddling with the Scenery Management system.
/*
 * Revision 1.1  1997/06/22 21:42:35  curt
 * Initial revision of VRML (subset) parser.
 *
 */
