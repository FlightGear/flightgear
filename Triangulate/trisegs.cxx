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
    int extra_index;
    int counter;
    double m, b, y_err, x_err;
    const_point_list_iterator current, last;

    // bool temp = false;
    // if ( s == FGTriSeg( 170, 206 ) ) {
    //   cout << "this is it!" << endl;
    //   temp = true;
    // }

    if ( fabs(p0.x() - p1.x()) > FG_EPSILON ) {
	// use y = mx + b

	// sort these in a sensable order
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

		if ( y_err < 10 * FG_EPSILON ) {
		    cout << "FOUND EXTRA SEGMENT NODE (Y)" << endl;
		    cout << p0 << " < " << *current << " < "
			 << p1 << endl;
		    found_extra = true;
		    extra_index = counter;
		    break;
		}
	    }
	    ++counter;
	}
    } else {
	// use x = constant

	// sort these in a sensable order
	if ( p0.y() > p1.y() ) {
	    Point3D tmp = p0;
	    p0 = p1;
	    p1 = tmp;
	}

	current = nodes.begin();
	last = nodes.end();
	counter = 0;
	for ( ; current != last; ++current ) {
	    // cout << counter << endl;
	    if ( (current->y() > p0.y()) && (current->y() < p1.y()) ) {
		x_err = fabs(current->x() - p0.x());
		if ( x_err < 10*FG_EPSILON ) {
		    cout << "FOUND EXTRA SEGMENT NODE (X)" << endl;
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
	cout << "dividing " << s.get_n1() << " " << extra_index 
	     << " " << s.get_n2() << endl;
	unique_divide_and_add( nodes, FGTriSeg( s.get_n1(), extra_index ) );
	unique_divide_and_add( nodes, FGTriSeg( extra_index, s.get_n2() ) );
    } else {
	// this segment does not need to be divided, lets add it
	unique_add( s );
    }
}


// $Log$
// Revision 1.4  1999/03/27 05:30:17  curt
// Handle corner nodes separately from the rest of the fitted nodes.
// Add fitted nodes in after corners and polygon nodes since the fitted nodes
//   are less important.  Subsequent nodes will "snap" to previous nodes if
//   they are "close enough."
// Need to manually divide segments to prevent "T" intersetions which can
//   confound the triangulator.  Hey, I got to use a recursive method!
// Pass along correct triangle attributes to output file generator.
// Do fine grained node snapping for corners and polygons, but course grain
//   node snapping for fitted terrain nodes.
//
// Revision 1.3  1999/03/23 22:02:57  curt
// Refinements in naming and organization.
//
// Revision 1.2  1999/03/20 20:32:59  curt
// First mostly successful tile triangulation works.  There's plenty of tweaking
// to do, but we are marching in the right direction.
//
// Revision 1.1  1999/03/20 13:21:36  curt
// Initial revision.
//
