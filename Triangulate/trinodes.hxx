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

#include <Math/point3d.hxx>

#include <Main/construct_types.hxx>


#define FG_PROXIMITY_EPSILON 0.000001
#define FG_COURSE_EPSILON 0.0003


class FGTriNodes {

private:

    point_list node_list;

    // return true of the two points are "close enough" as defined by
    // FG_PROXIMITY_EPSILON
    bool close_enough( const Point3D& p, const Point3D& p );

    // return true of the two points are "close enough" as defined by
    // FG_COURSE_EPSILON
    bool course_close_enough( const Point3D& p1, const Point3D& p2 );

public:

    // Constructor and destructor
    FGTriNodes( void );
    ~FGTriNodes( void );

    // Add a point to the point list if it doesn't already exist.
    // Returns the index (starting at zero) of the point in the list.
    int unique_add( const Point3D& p );

    // Add the point with no uniqueness checking
    int simple_add( const Point3D& p );

    // Add a point to the point list if it doesn't already exist.
    // Returns the index (starting at zero) of the point in the list.
    // Use a course proximity check
    int course_add( const Point3D& p );

    // return the master node list
    inline point_list get_node_list() const { return node_list; }

    // return the ith point
    inline Point3D get_node( int i ) const { return node_list[i]; }
};


// return true of the two points are "close enough" as defined by
// FG_PROXIMITY_EPSILON
inline bool FGTriNodes::close_enough( const Point3D& p1, const Point3D& p2 ) {
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


// $Log$
// Revision 1.7  1999/03/29 13:11:10  curt
// Shuffled stl type names a bit.
// Began adding support for tri-fanning (or maybe other arrangments too.)
//
// Revision 1.6  1999/03/27 05:30:16  curt
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
// Revision 1.5  1999/03/23 22:02:55  curt
// Refinements in naming and organization.
//
// Revision 1.4  1999/03/22 23:49:05  curt
// Modifications to facilitate conversion to output format.
//
// Revision 1.3  1999/03/20 02:21:55  curt
// Continue shaping the code towards triangulation bliss.  Added code to
// calculate some point guaranteed to be inside a polygon.
//
// Revision 1.2  1999/03/19 22:29:06  curt
// Working on preparationsn for triangulation.
//
// Revision 1.1  1999/03/17 23:52:00  curt
// Initial revision.
//
