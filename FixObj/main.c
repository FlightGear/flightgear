/* main.c -- read and fix the stripping order of a .obj file
 *
 * Written by Curtis Olson, started December 1997.
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

#include "obj.h"


int main(int argc, char **argv) {
    char infile[256], outfile[256];

    if ( argc != 3 ) {
	printf("Usage %s: infile outfile\n", argv[0]);
    }

    strcpy(infile, argv[1]);
    strcpy(outfile, argv[2]);

    /* load the input data files */
    obj_fix(infile, outfile);

    return(0);
}


/* $Log$
/* Revision 1.2  1998/01/09 23:03:12  curt
/* Restructured to split 1deg x 1deg dem's into 64 subsections.
/*
 * Revision 1.1  1997/12/08 19:28:54  curt
 * Initial revision.
 *
 */
