// area.h -- routines to assist with inserting "areas" into FG terrain
//
// Written by Curtis Olson, started February 1998.
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


#ifndef _AREA_H
#define _AREA_H


#include <list>

#include "point2d.hxx"


// generate an area for a runway (return result points in degrees)
list < point2d >
gen_runway_area( double lon, double lat, double heading, 
		      double length, double width);


#endif // _AREA_H


// $Log$
// Revision 1.1  1999/04/05 21:32:43  curt
// Initial revision
//
// Revision 1.2  1998/09/04 23:04:49  curt
// Beginning of convex hull genereration routine.
//
// Revision 1.1  1998/09/01 19:34:33  curt
// Initial revision.
//
// Revision 1.1  1998/07/20 12:54:05  curt
// Initial revision.
//
//
