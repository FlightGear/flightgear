// vector.hxx -- additional vector routines
//
// Written by Curtis Olson, started December 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#ifndef _VECTOR_HXX
#define _VECTOR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include "mat3.h"


// Map a vector onto the plane specified by normal
#if defined( USE_XTRA_MAT3_INLINES )
#  define map_vec_onto_cur_surface_plane(normal, v0, vec, result) { \
	double scale = ((normal[0]*vec[0]+normal[1]*vec[1]+normal[2]*vec[2]) / \
	       (normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2])); \
	result[0] = vec[0]-normal[0]*scale; \
	result[1] = vec[1]-normal[1]*scale; \
	result[2] = vec[2]-normal[2]*scale; \
  }
#else
  void map_vec_onto_cur_surface_plane(MAT3vec normal, MAT3vec v0, MAT3vec vec,
				    MAT3vec result);
#endif //defined( USE_XTRA_MAT3_INLINES )


// Given a point p, and a line through p0 with direction vector d,
// find the shortest distance from the point to the line
double fgPointLine(MAT3vec p, MAT3vec p0, MAT3vec d);


// Given a point p, and a line through p0 with direction vector d,
// find the shortest distance (squared) from the point to the line
double fgPointLineSquared(MAT3vec p, MAT3vec p0, MAT3vec d);


#endif // _VECTOR_HXX


// $Log$
// Revision 1.1  1999/04/05 21:32:33  curt
// Initial revision
//
// Revision 1.4  1998/10/16 23:36:39  curt
// c++-ifying.
//
// Revision 1.3  1998/08/24 20:04:13  curt
// Various "inline" code optimizations contributed by Norman Vine.
//
// Revision 1.2  1998/07/24 21:34:38  curt
// fgPointLine() rewritten into fgPointLineSquared() ... this ultimately saves
// us from doing a sqrt().
//
// Revision 1.1  1998/07/08 14:40:10  curt
// polar3d.[ch] renamed to polar3d.[ch]xx, vector.[ch] renamed to vector.[ch]xx
// Updated fg_geodesy comments to reflect that routines expect and produce
//   meters.
//
// Revision 1.4  1998/04/21 17:03:51  curt
// Prepairing for C++ integration.
//
// Revision 1.3  1998/01/22 02:59:39  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.2  1998/01/19 19:27:14  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.1  1997/12/22 04:13:18  curt
// Initial revision.
//

