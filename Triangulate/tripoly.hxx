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

    // calculate an "arbitrary" point inside this polygon for
    // assigning attribute areas
    void calc_point_inside( const FGTriNodes& trinodes );
};


#endif // _TRIPOLY_HXX


// $Log$
// Revision 1.1  1999/03/20 13:21:36  curt
// Initial revision.
//
