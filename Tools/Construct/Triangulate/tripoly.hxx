// tripoly.hxx -- "Triangle" polygon management class
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


#ifndef _TRIPOLY_HXX
#define _TRIPOLY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <vector>

#include <Main/construct_types.hxx>

#include "trinodes.hxx"

FG_USING_STD(vector);


class FGTriPoly {

private:

    int_list poly;
    Point3D inside;

public:

    // Constructor and destructor
    FGTriPoly( void );
    ~FGTriPoly( void );

    // Add the specified node (index) to the polygon
    inline void add_node( int index ) { poly.push_back( index ); }

    // return size
    inline int size() const { return poly.size(); }

    // return the ith polygon point index
    inline int get_pt_index( int i ) const { return poly[i]; }

    // calculate an "arbitrary" point inside this polygon for
    // assigning attribute areas
    void calc_point_inside( const FGTriNodes& trinodes );
    inline Point3D get_point_inside() const { return inside; }

    inline void erase() { poly.erase( poly.begin(), poly.end() ); }
};


typedef vector < FGTriPoly > tripoly_list;
typedef tripoly_list::iterator tripoly_list_iterator;
typedef tripoly_list::const_iterator const_tripoly_list_iterator;


#endif // _TRIPOLY_HXX


