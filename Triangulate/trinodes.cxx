// trinodes.cxx -- "Triangle" nodes management class
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


#include "trinodes.hxx"


// Constructor 
FGTriNodes::FGTriNodes( void ) {
}


// Destructor
FGTriNodes::~FGTriNodes( void ) {
}


// return true of the two points are "close enough" as defined by
// FG_PROXIMITY_EPSILON
inline bool FGTriNodes::close_enough( const Point3D& p1, const Point3D& p2 ) {
    if ( ( fabs(p1.x() - p2.x()) < FG_PROXIMITY_EPSILON ) &&
	 ( fabs(p1.y() - p2.y()) < FG_PROXIMITY_EPSILON ) ) {
	return true;
    } else {
	return false;
    }
}


// Add a point to the point list if it doesn't already exist.  Returns
// the index (starting at zero) of the point in the list.
int FGTriNodes::unique_add( const Point3D& p ) {
    trinode_list_iterator current, last;
    int counter = 0;

    // cout << p.x() << "," << p.y() << endl;

    // see if point already exists
    current = node_list.begin();
    last = node_list.end();
    for ( ; current != last; ++current ) {
	if ( close_enough(p, *current) ) {
	    // cout << "found an existing match!" << endl;
	    return counter;
	}
	
	++counter;
    }

    // add to list
    node_list.push_back( p );

    return counter;
}


// Add the point with no uniqueness checking
int FGTriNodes::simple_add( const Point3D& p ) {
    node_list.push_back( p );

    return node_list.size() - 1;
}


// $Log$
// Revision 1.4  1999/03/22 23:49:04  curt
// Modifications to facilitate conversion to output format.
//
// Revision 1.3  1999/03/20 02:21:54  curt
// Continue shaping the code towards triangulation bliss.  Added code to
// calculate some point guaranteed to be inside a polygon.
//
// Revision 1.2  1999/03/19 00:27:12  curt
// Continued work on triangulation preparation.
//
// Revision 1.1  1999/03/17 23:52:00  curt
// Initial revision.
//


