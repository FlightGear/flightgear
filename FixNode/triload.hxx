// triload.hxx -- read in a .node file and fix the z values of the 
//                interpolated points
//
// Written by Curtis Olson, started November 1997.
//
// Copyright (C) 1997 - 1998  Curtis L. Olson  - curt@me.umn.edu
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


#ifndef _TRILOAD_H
#define _TRILOAD_H


#include <stdio.h>
#include <string.h>


#define MAX_NODES 200000
#define MAX_TRIS  400000


extern int nodecount, tricount;


// Initialize a new mesh structure
void triload(char *basename, double nodes[MAX_NODES][3]);


#endif // _TRILOAD_H


// $Log$
// Revision 1.1  1998/04/08 23:06:00  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.4  1998/03/19 02:50:20  curt
// Updated to support -lDEM class.
//
// Revision 1.3  1998/03/03 16:00:59  curt
// More c++ compile tweaks.
//
// Revision 1.2  1998/01/09 23:03:09  curt
// Restructured to split 1deg x 1deg dem's into 64 subsections.
//
// Revision 1.1  1997/11/27 00:17:35  curt
// Initial revision.
//
