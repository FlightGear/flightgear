// triangle.hxx -- "Triangle" interface class
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


#ifndef _TRIANGLE_HXX
#define _TRIANGLE_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <Array/array.hxx>
#include <Clipper/clipper.hxx>
#include <Math/point3d.hxx>
#include <Polygon/names.hxx>

#define REAL double
extern "C" {
#include <Triangle/triangle.h>
}

#include "trieles.hxx"
#include "trinodes.hxx"
#include "tripoly.hxx"
#include "trisegs.hxx"


class FGTriangle {

private:

    // list of nodes
    FGTriNodes in_nodes;
    FGTriNodes out_nodes;

    // list of segments
    FGTriSegments trisegs;

    // polygon list
    tripoly_list polylist[FG_MAX_AREA_TYPES];
    
    // triangle list
    triele_list elelist;

public:

    // Constructor and destructor
    FGTriangle( void );
    ~FGTriangle( void );

    // add nodes from the dem fit
    int add_nodes();

    // populate this class based on the specified gpc_polys list
    int build( const point_list& corner_list, 
	       const point_list& fit_list,
	       const FGgpcPolyList& gpc_polys );

    // front end triangulator for polygon list
    int run_triangulate();

    inline FGTriNodes get_out_nodes() const { return out_nodes; }
    inline triele_list get_elelist() const { return elelist; }
};


#endif // _TRIANGLE_HXX


// $Log$
// Revision 1.10  1999/03/29 13:11:08  curt
// Shuffled stl type names a bit.
// Began adding support for tri-fanning (or maybe other arrangments too.)
//
// Revision 1.9  1999/03/27 05:30:13  curt
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
// Revision 1.8  1999/03/23 22:02:52  curt
// Refinements in naming and organization.
//
// Revision 1.7  1999/03/22 23:49:03  curt
// Modifications to facilitate conversion to output format.
//
// Revision 1.6  1999/03/20 20:32:56  curt
// First mostly successful tile triangulation works.  There's plenty of tweaking
// to do, but we are marching in the right direction.
//
// Revision 1.5  1999/03/20 02:21:53  curt
// Continue shaping the code towards triangulation bliss.  Added code to
// calculate some point guaranteed to be inside a polygon.
//
// Revision 1.4  1999/03/19 22:29:05  curt
// Working on preparationsn for triangulation.
//
// Revision 1.3  1999/03/19 00:27:11  curt
// Continued work on triangulation preparation.
//
// Revision 1.2  1999/03/18 04:31:12  curt
// Let's not pass copies of huge structures on the stack ... ye might see a
// segfault ... :-)
//
// Revision 1.1  1999/03/17 23:51:59  curt
// Initial revision.
//
