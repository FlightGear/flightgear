// point2d.cxx -- 2d coordinate routines
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


#include <math.h>

#include "point2d.hxx"


// convert a point from cartesian to polar coordinates
point2d cart_to_polar_2d(point2d in) {
    point2d result;
    result.dist = sqrt( in.x * in.x + in.y * in.y );
    result.theta = atan2(in.y, in.x);    

    return(result);
}


// $Log$
// Revision 1.1  1999/04/05 21:32:43  curt
// Initial revision
//
// Revision 1.1  1998/09/04 23:04:53  curt
// Beginning of convex hull genereration routine.
//
//
