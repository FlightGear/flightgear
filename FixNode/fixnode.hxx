// fixnode.hxx -- traverse the node file and fix the elevation of all the new
//                interpolated points.
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
#include <string>

#include <vector>
#include "Include/fg_stl_config.h"

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#include <DEM/dem.hxx>
#include <Include/fg_types.h>


typedef vector < fgPoint3d > container;
typedef container::iterator iterator;
typedef container::const_iterator const_iterator;


// Initialize a new mesh structure
void load_nodes(const string& basename, container& node_list);


// load the node information
void fix_nodes( const string& basename, fgDEM& dem, container& node_list );


#endif // _FIXNODE_H


// $Log$
// Revision 1.3  1998/09/19 20:43:53  curt
// C++-ified and STL-ified the code.  Combined triload.* and fixnode.* into
// a single file.
//
// Revision 1.2  1998/07/22 21:46:41  curt
// Fixed a bug that was triggering a seg fault.
//
// Revision 1.1  1998/04/08 23:05:56  curt
// Adopted Gnu automake/autoconf system.
//
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
