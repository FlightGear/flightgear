// fixnode.h -- traverse the node file and fix the elevation of all the new
//              interpolated points.
//
// Written by Curtis Olson, started November 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#ifndef _FIXNODE_H
#define _FIXNODE_H


#include <stdio.h>
#include <string.h>

#include "triload.h"
#include "../DEM/dem.h"


// load the node information
void fixnodes( char *basename, fgDEM dem, 
	       float dem_data[DEM_SIZE_1][DEM_SIZE_1], 
	       double nodes[MAX_NODES][3] );


#endif // _FIXNODE_H


// $Log$
// Revision 1.4  1998/03/19 02:50:19  curt
// Updated to support -lDEM class.
//
// Revision 1.3  1998/03/03 16:00:58  curt
// More c++ compile tweaks.
//
// Revision 1.2  1997/12/02 13:12:07  curt
// Updated to fix every node.
//
// Revision 1.1  1997/11/27 00:17:33  curt
// Initial revision.
//
