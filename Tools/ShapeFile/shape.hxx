// shape.hxx -- shape/gpc utils
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


#ifndef _SHAPE_HXX
#define _SHAPE_HXX


// include Generic Polygon Clipping Library
extern "C" {
#include <gpc.h>
}

#include <Polygon/names.hxx>


// Initialize structure we use to create polygons for the gpc library
// this must be called once from main for any program that uses this library
bool shape_utils_init();


// initialize a gpc_polygon
void init_shape(gpc_polygon *shape);

// make a gpc_polygon
void add_to_shape(int count, double *coords, gpc_polygon *shape);

// process shape (write polygon to all intersecting tiles)
void process_shape(string path, AreaType area, gpc_polygon *gpc_shape);

// free a gpc_polygon
void free_shape(gpc_polygon *shape);


#endif // _SHAPE_HXX


// $Log$
// Revision 1.2  1999/02/25 21:31:09  curt
// First working version???
//
// Revision 1.1  1999/02/23 01:29:06  curt
// Additional progress.
//
