// triangle.cxx -- "Triangle" interface class
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


#include "triangle.hxx"


// Constructor
FGTriangle::FGTriangle( void ) {
}


// Destructor
FGTriangle::~FGTriangle( void ) {
}


// populate this class based on the specified gpc_polys list
int FGTriangle::build( const FGgpcPolyList& gpc_polys ) {
    int index;
    tripoly poly;

    // traverse the gpc_polys and build a unified node list and a set
    // of Triangle PSLG that reference the node list by index
    // (starting at zero)

    gpc_polygon *gpc_poly;
    const_gpcpoly_iterator current, last;

    // process polygons in priority order
    cout << "prepairing node list and polygons" << endl;

    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	cout << "area type = " << i << endl;
	current = gpc_polys.polys[i].begin();
	last = gpc_polys.polys[i].end();
	for ( ; current != last; ++current ) {
	    gpc_poly = *current;
	    cout << "processing a polygon, contours = " 
		 << gpc_poly->num_contours << endl;

	    poly.erase(poly.begin(), poly.end());

	    if (gpc_poly->num_contours <= 0 ) {
		cout << "FATAL ERROR! no contours in this polygon" << endl;
		exit(-1);
	    }

	    if (gpc_poly->num_contours > 1 ) {
		cout << "FATAL ERROR! no multi-contour support" << endl;
		exit(-1);
	    }

	    for ( int j = 0; j < gpc_poly->num_contours; j++ ) {
		for ( int k = 0; k < gpc_poly->contour[j].num_vertices; k++ ) {
		    Point3D p( gpc_poly->contour[j].vertex[k].x,
			       gpc_poly->contour[j].vertex[k].y,
			       0 );
		    index = trinodes.unique_add( p );
		    poly.push_back(index);
		    // cout << index << endl;
		}
		polylist[i].push_back(poly);
	    }
	}
    }

    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	cout << "polylist[" << i << "] = " << polylist[i].size() << endl;
    }
    return 0;
}


// $Log$
// Revision 1.3  1999/03/19 00:27:10  curt
// Continued work on triangulation preparation.
//
// Revision 1.2  1999/03/18 04:31:11  curt
// Let's not pass copies of huge structures on the stack ... ye might see a
// segfault ... :-)
//
// Revision 1.1  1999/03/17 23:51:59  curt
// Initial revision.
//
