//
// interpolater.hxx -- routines to handle linear interpolation from a table of
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


#ifndef _INTERPOLATER_H
#define _INTERPOLATER_H


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include "Include/compiler.h"

#include STL_STRING
FG_USING_STD(string);

#define MAX_TABLE_SIZE 32


class fgINTERPTABLE {
    int size;
    double table[MAX_TABLE_SIZE][2];

public:

    // Constructor -- loads the interpolation table from the specified
    // file
    fgINTERPTABLE( const string& file );

    // Given an x value, linearly interpolate the y value from the table
    double interpolate(double x);

    // Destructor
    ~fgINTERPTABLE( void );
};


#endif // _INTERPOLATER_H


