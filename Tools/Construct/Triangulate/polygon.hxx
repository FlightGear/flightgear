// polygon.hxx -- polygon (with holes) management class
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


#ifndef _POLYGON_HXX
#define _POLYGON_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <vector>

#include <Main/construct_types.hxx>

#include "trinodes.hxx"

FG_USING_STD(vector);


typedef vector < int_list > polytype;
typedef polytype::iterator polytype_iterator;
typedef polytype::const_iterator const_polytype_iterator;


class FGPolygon {

private:

    polytype poly;           // polygons
    point_list inside_list;  // point inside list
    int_list hole_list;      // hole flag list

public:

    // Constructor and destructor
    FGPolygon( void );
    ~FGPolygon( void );

    // Add the specified node (index) to the polygon
    inline void add_node( int contour, int index ) {
	if ( contour >= (int)poly.size() ) {
	    // extend polygon
	    int_list empty_contour;
	    empty_contour.clear();
	    for ( int i = 0; i < contour - (int)poly.size() + 1; ++i ) {
		poly.push_back( empty_contour );
		inside_list.push_back( Point3D(0.0) );
		hole_list.push_back( 0 );
	    }
	}
	poly[contour].push_back( index );
    }

    // return size
    inline int contours() const { return poly.size(); }
    inline int contour_size( int contour ) const { 
	return poly[contour].size();
    }

    // return the ith polygon point index from the specified contour
    inline int get_pt_index( int contour, int i ) const { 
	return poly[contour][i];
    }

    // calculate an "arbitrary" point inside the specified contour for
    // assigning attribute areas
    void calc_point_inside( const int contour, const FGTriNodes& trinodes );
    inline Point3D get_point_inside( const int contour ) const { 
	return inside_list[contour];
    }

    // get and set hole flag
    inline int get_hole_flag( const int contour ) const { 
	return hole_list[contour];
    }
    inline void set_hole_flag( const int contour, const int flag ) {
	hole_list[contour] = flag;
    }

    inline void erase() { poly.clear(); }
};


typedef vector < FGPolygon > poly_list;
typedef poly_list::iterator poly_list_iterator;
typedef poly_list::const_iterator const_poly_list_iterator;


#endif // _POLYGON_HXX


