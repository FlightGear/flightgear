/* triload.c -- read in a .node file and fix the z values of the interpolated 
 *              points
 *
 * Written by Curtis Olson, started November 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 */


#include <stdio.h>
#include <string.h>

#include "../Dem2node/demparse.h"
#include "fixnode.h"
#include "triload.h"


int main(int argc, char **argv) {
    char basename[256];
    struct MESH dem_mesh;

    strcpy(basename, argv[1]);

    /* load the input data files */
    triload(basename);

    /* load the corresponding dem file so we can interpolate elev values */
    dem_parse(basename, &dem_mesh);

    fixnodes(basename, &dem_mesh);

    return(0);
}


/* $Log$
/* Revision 1.2  1997/12/02 13:12:07  curt
/* Updated to fix every node.
/*
 * Revision 1.1  1997/11/27 00:17:34  curt
 * Initial revision.
 *
 */
