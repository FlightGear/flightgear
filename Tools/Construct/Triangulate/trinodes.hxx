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


#ifndef _TRINODES_HXX
#define _TRINODES_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <Math/point3d.hxx>

#include <Main/construct_types.hxx>


#define FG_PROXIMITY_EPSILON 0.000001
#define FG_COURSE_EPSILON 0.0003


class FGTriNodes {

private:

    point_list node_list;

    // return true of the two points are "close enough" as defined by
    // FG_PROXIMITY_EPSILON
    bool close_enough( const Point3D& p, const Point3D& p ) const;

    // return true of the two points are "close enough" as defined by
    // FG_COURSE_EPSILON
    bool course_close_enough( const Point3D& p1, const Point3D& p2 );

public:

    // Constructor and destructor
    FGTriNodes( void );
    ~FGTriNodes( void );

    // delete all the data out of node_list
    inline void clear() { node_list.clear(); }

    // Add a point to the point list if it doesn't already exist.
    // Returns the index (starting at zero) of the point in the list.
    int unique_add( const Point3D& p );

    // Add the point with no uniqueness checking
    int simple_add( const Point3D& p );

    // Add a point to the point list if it doesn't already exist.
    // Returns the index (starting at zero) of the point in the list.
    // Use a course proximity check
    int course_add( const Point3D& p );

    // Find the index of the specified point (compair to the same
    // tolerance as unique_add().  Returns -1 if not found.
    int find( const Point3D& p ) const;

     // return the master node list
    inline point_list get_node_list() const { return node_list; }

    // return the ith point
    inline Point3D get_node( int i ) const { return node_list[i]; }

    // return the size of the node list
    inline size_t size() const { return node_list.size(); }
};


// return true of the two points are "close enough" as defined by
// FG_PROXIMITY_EPSILON
inline bool FGTriNodes::close_enough( const Point3D& p1, const Point3D& p2 )
    const
{
    if ( ( fabs(p1.x() - p2.x()) < FG_PROXIMITY_EPSILON ) &&
	 ( fabs(p1.y() - p2.y()) < FG_PROXIMITY_EPSILON ) ) {
	return true;
    } else {
	return false;
    }
}


// return true of the two points are "close enough" as defined by
// FG_COURSE_EPSILON
inline bool FGTriNodes::course_close_enough( const Point3D& p1, 
					     const Point3D& p2 )
{
    if ( ( fabs(p1.x() - p2.x()) < FG_COURSE_EPSILON ) &&
	 ( fabs(p1.y() - p2.y()) < FG_COURSE_EPSILON ) ) {
	return true;
    } else {
	return false;
    }
}


#endif // _TRINODES_HXX


