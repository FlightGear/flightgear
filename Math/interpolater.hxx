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
// (Log is kept at end of this file)


#ifndef _INTERPOLATER_H
#define _INTERPOLATER_H


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <string>


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


// $Log$
// Revision 1.3  1998/11/06 21:17:28  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.2  1998/04/22 13:18:10  curt
// C++ - ified comments.  Make file open errors fatal.
//
// Revision 1.1  1998/04/21 19:14:23  curt
// Modified Files:
//     Makefile.am Makefile.in
// Added Files:
//     interpolater.cxx interpolater.hxx
//
