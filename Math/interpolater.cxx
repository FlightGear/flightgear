//
// interpolater.cxx -- routines to handle linear interpolation from a table of
//                     x,y   The table must be sorted by "x" in ascending order
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#include <string.h>

#include <Debug/fg_debug.h>
#include <zlib/zlib.h>

#include "interpolater.hxx"


// Constructor -- loads the interpolation table from the specified
// file
fgINTERPTABLE::fgINTERPTABLE( char *file ) {
    char gzfile[256], line[256];
    gzFile fd;

    fgPrintf( FG_MATH, FG_INFO, "Initializing Interpolator for %s\n", file);

    // First try "file.gz"
    strcpy(gzfile, file);
    strcat(gzfile, ".gz");
    if ( (fd = gzopen(gzfile, "r")) == NULL ) {
        // Next try "path"
        if ( (fd = gzopen(file, "r")) == NULL ) {
            fgPrintf(FG_MATH, FG_EXIT, "Cannot open file: %s\n", file);
        }
    }

    size = 0;
    while ( gzgets(fd, line, 250) != NULL ) {
	if ( size < MAX_TABLE_SIZE ) {
	    sscanf(line, "%lf %lf\n", &(table[size][0]), &(table[size][1]));
	    size++;
	} else {
            fgPrintf( FG_MATH, FG_EXIT, 
		      "fgInterpolateInit(): Exceed max table size = %d\n",
		      MAX_TABLE_SIZE );
	}
    }

    gzclose(fd);
}


// Given an x value, linearly interpolate the y value from the table
double fgINTERPTABLE::interpolate(double x) {
    int i;
    double y;

    i = 0;

    while ( (x > table[i][0]) && (i < size) ) {
	i++;
    }

    // printf ("i = %d ", i);

    if ( (i == 0) && (x < table[0][0]) ) {
	fgPrintf( FG_MATH, FG_ALERT, 
		  "fgInterpolateInit(): lookup error, x to small = %.2f\n", x);
	return(0.0);
    }

    if ( x > table[i][0] ) {
	fgPrintf( FG_MATH, FG_ALERT, 
		  "fgInterpolateInit(): lookup error, x to big = %.2f\n",  x);
	return(0.0);
    }

    // y = y1 + (y0 - y1)(x - x1) / (x0 - x1)
    y = table[i][1] + 
	( (table[i-1][1] - table[i][1]) * 
	  (x - table[i][0]) ) /
	(table[i-1][0] - table[i][0]);

    return(y);
}


// Destructor
fgINTERPTABLE::~fgINTERPTABLE( void ) {
}


// $Log$
// Revision 1.2  1998/04/22 13:18:10  curt
// C++ - ified comments.  Make file open errors fatal.
//
// Revision 1.1  1998/04/21 19:14:23  curt
// Modified Files:
//     Makefile.am Makefile.in
// Added Files:
//     interpolater.cxx interpolater.hxx
//
