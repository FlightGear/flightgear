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


#include <Include/fg_constants.h>
#include <Math/point3d.hxx>

#include "trinodes.hxx"
#include "trisegs.hxx"


// Constructor 
FGTriSegments::FGTriSegments( void ) {
}


// Destructor
FGTriSegments::~FGTriSegments( void ) {
}


// Add a segment to the segment list if it doesn't already exist.
// Returns the index (starting at zero) of the segment in the list.
int FGTriSegments::unique_add( const FGTriSeg& s )
{
    triseg_list_iterator current, last;
    int counter = 0;

    // cout << s.get_n1() << "," << s.get_n2() << endl;

    // check if segment already exists
    current = seg_list.begin();
    last = seg_list.end();
    for ( ; current != last; ++current ) {
	if ( s == *current ) {
	    // cout << "found an existing segment match" << endl;
	    return counter;
	}
	
	++counter;
    }

    // add to list
    seg_list.push_back( s );

    return counter;
}


// Divide segment if there are other points on it, return the divided
// list of segments
void FGTriSegments::unique_divide_and_add( const point_list& nodes, 
					   const FGTriSeg& s )
{
    Point3D p0 = nodes[ s.get_n1() ];
    Point3D p1 = nodes[ s.get_n2() ];

    bool found_extra = false;
    int extra_index = 0;
    int counter;
    double m, b, y_err, x_err;
    const_point_list_iterator current, last;

    // bool temp = false;
    // if ( s == FGTriSeg( 170, 206 ) ) {
    //   cout << "this is it!" << endl;
    //   temp = true;
    // }

    if ( fabs(p0.x() - p1.x()) > 3 * FG_EPSILON ) {
	// use y = mx + b

	// sort these in a sensible order
	if ( p0.x() > p1.x() ) {
	    Point3D tmp = p0;
	    p0 = p1;
	    p1 = tmp;
	}

	m = (p0.y() - p1.y()) / (p0.x() - p1.x());
	b = p1.y() - m * p1.x();

	// if ( temp ) {
	//   cout << "m = " << m << " b = " << b << endl;
	// }

	current = nodes.begin();
	last = nodes.end();
	counter = 0;
	for ( ; current != last; ++current ) {
	    if ( (current->x() > (p0.x() + FG_EPSILON)) 
		 && (current->x() < (p1.x() - FG_EPSILON)) ) {

		// if ( temp ) {
		//   cout << counter << endl;
		// }

		y_err = fabs(current->y() - (m * current->x() + b));

		if ( y_err < 20 * FG_EPSILON ) {
		    // cout << "FOUND EXTRA SEGMENT NODE (Y)" << endl;
		    // cout << p0 << " < " << *current << " < "
		    //      << p1 << endl;
		    found_extra = true;
		    extra_index = counter;
		    break;
		}
	    }
	    ++counter;
	}
    } else {
	// use x = constant

	// cout << "FOUND VERTICLE SEGMENT" << endl;

	// sort these in a sensable order
	if ( p0.y() > p1.y() ) {
	    Point3D tmp = p0;
	    p0 = p1;
	    p1 = tmp;
	}

	// cout << " p0 = " << p0 << " p1 = " << p1 << endl;

	current = nodes.begin();
	last = nodes.end();
	counter = 0;
	for ( ; current != last; ++current ) {
	    // cout << counter << endl;
	    if ( (current->y() > (p0.y() + FG_EPSILON))
		 && (current->y() < (p1.y() - FG_EPSILON)) ) {
		x_err = fabs(current->x() - p0.x());
		// cout << "  found a potential point, x err = " 
		//      << x_err << endl;
		if ( x_err < 20 * FG_EPSILON ) {
		    // cout << "FOUND EXTRA SEGMENT NODE (X)" << endl;
		    // cout << p0 << " < " << *current << " < "
		    //      << p1 << endl;
		    found_extra = true;
		    extra_index = counter;
		    break;
		}
	    }
	    ++counter;
	}
    }

    if ( found_extra ) {
	// recurse with two sub segments
	// cout << "dividing " << s.get_n1() << " " << extra_index 
	//      << " " << s.get_n2() << endl;
	unique_divide_and_add( nodes, FGTriSeg( s.get_n1(), extra_index ) );
	unique_divide_and_add( nodes, FGTriSeg( extra_index, s.get_n2() ) );
    } else {
	// this segment does not need to be divided, lets add it
	unique_add( s );
    }
}


