// clipper.hxx -- top level routines to take a series of arbitrary areas and
//                produce a tight fitting puzzle pieces that combine to make a
//                tile
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)
 


#ifndef _CLIPPER_HXX
#define _CLIPPER_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>


// include Generic Polygon Clipping Library
//
//    http://www.cs.man.ac.uk/aig/staff/alan/software/
//
extern "C" {
#include <gpc.h>
}

#include STL_STRING
#include <vector>

FG_USING_STD(string);
FG_USING_STD(vector);


typedef vector < gpc_polygon * > gpcpoly_container;
typedef gpcpoly_container::iterator gpcpoly_iterator;
typedef gpcpoly_container::const_iterator const_gpcpoly_iterator;


#define FG_MAX_AREA_TYPES 20
#define EXTRA_SAFETY_CLIP
#define FG_MAX_VERTICES 100000


class point2d {
public:
    double x, y;
};


class FGgpcPolyList {
public:
    gpcpoly_container polys[FG_MAX_AREA_TYPES];
    gpc_polygon safety_base;
};


class FGClipper {

private:

    gpc_vertex_list v_list;
    // static gpc_polygon poly;
    FGgpcPolyList polys_in, polys_clipped;

public:

    // Constructor
    FGClipper( void );

    // Destructor
    ~FGClipper( void );

    // Initialize Clipper (allocate and/or connect structures)
    bool init();

    // Load a polygon definition file
    bool load_polys(const string& path);

    // Do actually clipping work
    bool clip_all(const point2d& min, const point2d& max);

    // return output poly list
    inline FGgpcPolyList get_polys_clipped() const { return polys_clipped; }
};


#endif // _CLIPPER_HXX


// $Log$
// Revision 1.5  1999/03/19 00:26:19  curt
// Fixed a clipping bug (polygons specified in wrong order).
// Touched up a few compiler warnings.
//
// Revision 1.4  1999/03/18 04:31:10  curt
// Let's not pass copies of huge structures on the stack ... ye might see a
// segfault ... :-)
//
// Revision 1.3  1999/03/17 23:48:59  curt
// minor renaming and a bit of rearranging.
//
// Revision 1.2  1999/03/13 23:51:34  curt
// Renamed main.cxx to testclipper.cxx
// Converted clipper routines to a class FGClipper.
//
// Revision 1.1  1999/03/01 15:39:39  curt
// Initial revision.
//
