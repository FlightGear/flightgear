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


#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "../Dem2node/demparse.h"
#include "fixnode.h"
#include "triload.h"


/* Original DEM which is used to interpolate z values */
struct MESH dem_mesh;


/* find all the matching files in the specified directory and fix them */
void process_files(char *root_path) {
    DIR *d;
    struct dirent *de;
    char file_path[256];
    char *ptr;
    int len;

    if ( (d = opendir(root_path)) == NULL ) {
        printf("cannot open directory '%s'.", root_path);
	exit(-1);
    }

    while ( (de = readdir(d)) != NULL ) {
	len = strlen(de->d_name);
	if ( len > 7 ) {
	    ptr = de->d_name;
	    ptr += (len - 7);
	    /* printf("--> %s \n", ptr); */

	    if ( strcmp(ptr, ".1.node") == 0 ) {
		strcpy(file_path, root_path);
		strcat(file_path, "/");
		strcat(file_path, de->d_name);
		printf("File = %s\n", file_path);

		/* load the input data files */
		triload(file_path);

		fixnodes(file_path, &dem_mesh);
	    }
	}
    }
}


/* main */
int main(int argc, char **argv) {
    char demfile[256], root_path[256];

    if ( argc != 3 ) {
	printf("Usage %s demfile root_path\n", argv[0]);
	exit(-1);
    }

    strcpy(demfile, argv[1]);
    strcpy(root_path, argv[2]);

    /* load the corresponding dem file so we can interpolate elev values */
    dem_parse(demfile, &dem_mesh);

    /* process all the *.1.node files in the specified directory */
    process_files(root_path);

    return(0);
}


/* $Log$
/* Revision 1.3  1998/01/09 23:03:08  curt
/* Restructured to split 1deg x 1deg dem's into 64 subsections.
/*
 * Revision 1.2  1997/12/02 13:12:07  curt
 * Updated to fix every node.
 *
 * Revision 1.1  1997/11/27 00:17:34  curt
 * Initial revision.
 *
 */
