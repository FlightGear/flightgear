// convex_hull.hxx -- calculate the convex hull of a set of points
//
// Written by Curtis Olson, started September 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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
//


#ifndef _CONVEX_HULL_HXX
#define _CONVEX_HULL_HXX


#include <list>

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#include "point2d.hxx"


// stl list typedefs
typedef list < point2d > list_container;
typedef list_container::iterator list_iterator;


// calculate the convex hull of a set of points, return as a list of
// point2d.  The algorithm description can be found at:
// http://riot.ieor.berkeley.edu/riot/Applications/ConvexHull/CHDetails.html
list_container convex_hull( list_container input_list );


#endif // _CONVEX_HULL_HXX


// $Log$
// Revision 1.2  1998/09/09 16:26:33  curt
// Continued progress in implementing the convex hull algorithm.
//
// Revision 1.1  1998/09/04 23:04:51  curt
// Beginning of convex hull genereration routine.
//
//
