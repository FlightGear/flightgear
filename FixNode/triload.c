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

#include "triload.h"


int nodecount;
double nodes[MAX_NODES][3];


/* load the node information */
void triload(char *filename) {
    FILE *node;
    int dim, junk1, junk2;
    int i;

    printf("Loading node file:  %s ...\n", filename);
    if ( (node = fopen(filename, "r")) == NULL ) {
	printf("Cannot open file '%s'\n", filename);
	exit(-1);
    }

    fscanf(node, "%d %d %d %d", &nodecount, &dim, &junk1, &junk2);

    if ( nodecount > MAX_NODES - 1 ) {
	printf("Error, too many nodes, need to increase array size\n");
	exit(-1);
    } else {
	printf("    Expecting %d nodes\n", nodecount);
    }

    for ( i = 1; i <= nodecount; i++ ) {
	fscanf(node, "%d %lf %lf %lf %d\n", &junk1, 
	       &nodes[i][0], &nodes[i][1], &nodes[i][2], &junk2);
	/* printf("%d %.2f %.2f %.2f\n", junk1, nodes[i][0], nodes[i][1], 
	   nodes[i][2]); */
    }

    fclose(node);
}


/* $Log$
/* Revision 1.2  1998/01/09 23:03:09  curt
/* Restructured to split 1deg x 1deg dem's into 64 subsections.
/*
 * Revision 1.1  1997/11/27 00:17:35  curt
 * Initial revision.
 *
 */
