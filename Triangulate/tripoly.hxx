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
// (Log is kept at end of this file)


#ifndef _TRIPOLY_HXX
#define _TRIPOLY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <vector>

#include "trinodes.hxx"

FG_USING_STD(vector);


typedef vector < int > tripoly;
typedef tripoly::iterator tripoly_iterator;
typedef tripoly::const_iterator const_tripoly_iterator;


class FGTriPoly {

private:

    tripoly poly;
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


// $Log$
// Revision 1.4  1999/03/23 22:02:56  curt
// Refinements in naming and organization.
//
// Revision 1.3  1999/03/21 14:02:07  curt
// Added a mechanism to dump out the triangle structures for viewing.
// Fixed a couple bugs in first pass at triangulation.
// - needed to explicitely initialize the polygon accumulator in triangle.cxx
//   before each polygon rather than depending on the default behavior.
// - Fixed a problem with region attribute propagation where I wasn't generating
//   the hole points correctly.
//
// Revision 1.2  1999/03/20 20:32:58  curt
// First mostly successful tile triangulation works.  There's plenty of tweaking
// to do, but we are marching in the right direction.
//
// Revision 1.1  1999/03/20 13:21:36  curt
// Initial revision.
//
