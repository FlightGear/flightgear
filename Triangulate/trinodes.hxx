// trinodes.hxx -- "Triangle" nodes management class
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


#ifndef _TRINODES_HXX
#define _TRINODES_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <vector>

#include <Math/point3d.hxx>

FG_USING_STD(vector);


#define FG_PROXIMITY_EPSILON 0.000001


typedef vector < Point3D > point_container;
typedef point_container::iterator point_iterator;
typedef point_container::const_iterator const_point_iterator;


class FGTriNodes {

private:

    point_container point_list;

    // return true of the two points are "close enough" as defined by
    // FG_PROXIMITY_EPSILON
    bool close_enough( const Point3D& p, const Point3D& p );

public:

    // Constructor and destructor
    FGTriNodes( void );
    ~FGTriNodes( void );

    // Add a point to the point list if it doesn't already exist.
    // Returns the index (starting at zero) of the point in the list.
    int unique_add( const Point3D& p );

    // return the master node list
    inline point_container get_node_list() const { return point_list; }
};


#endif // _TRINODES_HXX


// $Log$
// Revision 1.2  1999/03/19 22:29:06  curt
// Working on preparationsn for triangulation.
//
// Revision 1.1  1999/03/17 23:52:00  curt
// Initial revision.
//
