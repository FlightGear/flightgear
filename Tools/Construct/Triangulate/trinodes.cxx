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


#include "trinodes.hxx"


// Constructor 
FGTriNodes::FGTriNodes( void ) {
}


// Destructor
FGTriNodes::~FGTriNodes( void ) {
}


// Add a point to the point list if it doesn't already exist.  Returns
// the index (starting at zero) of the point in the list.
int FGTriNodes::unique_add( const Point3D& p ) {
    point_list_iterator current, last;
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


// Add a point to the point list if it doesn't already exist.  Returns
// the index (starting at zero) of the point in the list.  Use a
// course proximity check
int FGTriNodes::course_add( const Point3D& p ) {
    point_list_iterator current, last;
    int counter = 0;

    // cout << p.x() << "," << p.y() << endl;

    // see if point already exists
    current = node_list.begin();
    last = node_list.end();
    for ( ; current != last; ++current ) {
	if ( course_close_enough(p, *current) ) {
	    // cout << "found an existing match!" << endl;
	    return counter;
	}
	
	++counter;
    }

    // add to list
    node_list.push_back( p );

    return counter;
}


// Find the index of the specified point (compair to the same
// tolerance as unique_add().  Returns -1 if not found.
int FGTriNodes::find( const Point3D& p ) const {
    const_point_list_iterator current, last;
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

    return -1;
}


