/* fixnode.c -- traverse the node file and fix the elevation of all the new
 *              interpolated points.
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
#include <unistd.h>

#include "fixnode.h"
#include "../Dem2node/mesh.h"
#include "triload.h"


/* load the node information */
void fixnodes(char *filename, struct MESH *m) {
    char toname[256];
    FILE *fd;
    int i;

    printf("Fixing up node elevations\n");

    /* we could just fix the new nodes as the first "for" statement
     * would do, but that leads to funny results (I need to figure out
     * why.)  So, let's try fixing all of them */

    /* for ( i = origcount + 1; i <= nodecount; i++ ) { */
    for ( i = 1; i <= nodecount; i++ ) {
	/* printf("Current: %d %.2f %.2f %.2f\n", i, nodes[i][0],
	       nodes[i][1], nodes[i][2]); */

	nodes[i][2] = mesh_altitude(m, nodes[i][0], nodes[i][1]);

	/* printf("Fixed: %d %.2f %.2f %.2f\n", i, nodes[i][0],
	       nodes[i][1], nodes[i][2]); */
    }


    sprintf(toname, "%s.orig", filename);
    printf("Moving %s to %s\n", filename, toname);
    rename(filename, toname);

    printf("Saving new node file: %s\n", filename);

    fd = fopen(filename, "w");

    fprintf(fd, "%d 2 1 0\n", nodecount);

    for ( i = 1; i <= nodecount; i++ ) {
	fprintf(fd, "%d %.2f %.2f %.2f 0\n", i, nodes[i][0],
	       nodes[i][1], nodes[i][2]);
    }

    fclose(fd);
}


/* $Log$
/* Revision 1.3  1998/01/09 23:03:08  curt
/* Restructured to split 1deg x 1deg dem's into 64 subsections.
/*
 * Revision 1.2  1997/12/02 13:12:07  curt
 * Updated to fix every node.
 *
 * Revision 1.1  1997/11/27 00:17:33  curt
 * Initial revision.
 *
 */
