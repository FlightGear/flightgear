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


#include <Include/compiler.h>

#ifdef __MWERKS__
#include <stdlib.h> // for exit()
#endif

#include STL_STRING

#include <Debug/logstream.hxx>
#include <Include/fg_zlib.h>
#include <Misc/fgstream.hxx>

#include "interpolater.hxx"


// Constructor -- loads the interpolation table from the specified
// file
fgINTERPTABLE::fgINTERPTABLE( const string& file ) {
    FG_LOG( FG_MATH, FG_INFO, "Initializing Interpolator for " << file );

    fg_gzifstream in( file );
    if ( !in ) {
        FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    size = 0;
    in >> skipcomment;
    while ( in ) {
	if ( size < MAX_TABLE_SIZE ) {
	    in >> table[size][0] >> table[size][1];
	    size++;
	} else {
            FG_LOG( FG_MATH, FG_ALERT,
		    "fgInterpolateInit(): Exceed max table size = "
		    << MAX_TABLE_SIZE );
	    exit(-1);
	}
    }
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
	FG_LOG( FG_MATH, FG_ALERT, 
		"fgInterpolateInit(): lookup error, x to small = " << x );
	return(0.0);
    }

    if ( x > table[i][0] ) {
	FG_LOG( FG_MATH, FG_ALERT, 
		"fgInterpolateInit(): lookup error, x to big = " << x );
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


