/* main.c -- main loop
 *
 * Written by Curtis Olson, started February 1998.
 *
 * Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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

#include "rawdem.h"


int main(int argc, char **argv) {
    fgRAWDEM raw;
    char basename[256], output_dir[256], hdr_file[256], dem_file[256];
    int i;

    if ( argc != 3 ) {
	printf("Usage: %s <input_file_basename> <output_dir>\n", argv[0]);
	exit(-1);
    }

    /* get basename */
    strcpy(basename, argv[1]);

    /* get output dir */
    strcpy(output_dir, argv[2]);

    /* generate header file name */
    strcpy(hdr_file, basename);
    strcat(hdr_file, ".HDR");

    /* generate input file name (raw dem) */
    strcpy(dem_file, basename);
    strcat(dem_file, ".DEM");
    
    printf("Header file = %s  Input file = %s\n", hdr_file, dem_file);
    printf("Output Directory = %s\n", output_dir);

    /* scan the header file and extract important values */
    rawReadDemHdr(&raw, hdr_file);

    /* open up the raw data file */
    rawOpenDemFile(&raw, dem_file);

    for ( i = 41; i <= 90; i++ ) {
	rawProcessStrip(&raw, i, output_dir);
    }

    /* close the raw data file */
    rawCloseDemFile(&raw);

    return(0);
}


/* $Log$
/* Revision 1.2  1998/03/03 13:10:28  curt
/* Close to a working version.
/*
 * Revision 1.1  1998/03/02 23:31:01  curt
 * Initial revision.
 *
 */
