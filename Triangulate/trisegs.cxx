// trisegs.cxx -- "Triangle" segment management class
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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


#include "trisegs.hxx"


// Constructor 
FGTriSegments::FGTriSegments( void ) {
}


// Destructor
FGTriSegments::~FGTriSegments( void ) {
}


// Add a point to the point list if it doesn't already exist.
// Returns the index (starting at zero) of the point in the list.
int FGTriSegments::unique_add( const FGTriSeg& s ) {
    triseg_list_iterator current, last;
    int counter = 0;

    cout << s.get_n1() << "," << s.get_n2() << endl;

    // see if point already exists
    current = seg_list.begin();
    last = seg_list.end();
    for ( ; current != last; ++current ) {
	if ( s == *current ) {
	    cout << "found an existing segment match" << endl;
	    return counter;
	}
	
	++counter;
    }

    // add to list
    seg_list.push_back( s );

    return counter;
}


// $Log$
// Revision 1.2  1999/03/20 20:32:59  curt
// First mostly successful tile triangulation works.  There's plenty of tweaking
// to do, but we are marching in the right direction.
//
// Revision 1.1  1999/03/20 13:21:36  curt
// Initial revision.
//
