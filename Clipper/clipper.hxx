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

typedef vector < gpc_polygon * > polylist;
typedef polylist::iterator polylist_iterator;

#define FG_MAX_AREAS 20

class point2d {
public:
    double x, y;
};


class FGPolyList {
public:
    polylist polys[FG_MAX_AREAS];
    gpc_polygon safety_base;
};


// Initialize Clipper (allocate and/or connect structures)
bool fgClipperInit();


// Load a polygon definition file
bool fgClipperLoadPolygons(const string& path);


// Do actually clipping work
bool fgClipperMaster(const point2d& min, const point2d& max);


#endif // _CLIPPER_HXX


// $Log$
// Revision 1.1  1999/03/01 15:39:39  curt
// Initial revision.
//
