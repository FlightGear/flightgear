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

#include <string>
#include <vector>

#include <Main/construct_types.hxx>

#include "trinodes.hxx"

FG_USING_STD(string);
FG_USING_STD(vector);


#define FG_MAX_VERTICES 100000


typedef vector < point_list > polytype;
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

    // Add a contour
    inline void add_contour( const point_list contour, const int hole_flag ) {
	poly.push_back( contour );
	hole_list.push_back( hole_flag );
    }

    // Get a contour
    inline point_list get_contour( const int i ) const {
	return poly[i];
    }

    // Delete a contour
    inline void delete_contour( const int i ) {
	polytype_iterator start_poly = poly.begin();
	poly.erase( start_poly + i );

	int_list_iterator start_hole = hole_list.begin();
	hole_list.erase( start_hole + i );
    }

    // Add the specified node (index) to the polygon
    inline void add_node( int contour, Point3D p ) {
	if ( contour >= (int)poly.size() ) {
	    // extend polygon
	    point_list empty_contour;
	    empty_contour.clear();
	    for ( int i = 0; i < contour - (int)poly.size() + 1; ++i ) {
		poly.push_back( empty_contour );
		inside_list.push_back( Point3D(0.0) );
		hole_list.push_back( 0 );
	    }
	}
	poly[contour].push_back( p );
    }

    // return size
    inline int contours() const { return poly.size(); }
    inline int contour_size( int contour ) const { 
	return poly[contour].size();
    }

    // return the ith polygon point index from the specified contour
    inline Point3D get_pt( int contour, int i ) const { 
	return poly[contour][i];
    }

    // calculate some "arbitrary" point inside the specified contour for
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

    // erase
    inline void erase() { poly.clear(); }

    // informational

    // return the area of a contour (assumes simple polygons,
    // i.e. non-self intersecting.)
    double area_contour( const int contour );

    // return the smallest interior angle of the polygon
    double minangle_contour( const int contour );

    // output
    void write( const string& file );
};


typedef vector < FGPolygon > poly_list;
typedef poly_list::iterator poly_list_iterator;
typedef poly_list::const_iterator const_poly_list_iterator;


// wrapper functions for gpc polygon clip routines

// Difference
FGPolygon polygon_diff(	const FGPolygon& subject, const FGPolygon& clip );

// Intersection
FGPolygon polygon_int( const FGPolygon& subject, const FGPolygon& clip );

// Exclusive or
FGPolygon polygon_xor( const FGPolygon& subject, const FGPolygon& clip );

// Union
FGPolygon polygon_union( const FGPolygon& subject, const FGPolygon& clip );


#endif // _POLYGON_HXX


