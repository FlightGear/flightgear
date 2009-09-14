// tile.cxx -- routines to handle a scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@infoplane.com
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


#include <Include/compiler.h>

#include STL_FUNCTIONAL
#include STL_ALGORITHM

#include <Debug/logstream.hxx>
#include <Scenery/tile.hxx>
#include "Bucket/newbucket.hxx"

#include "tile.hxx"

FG_USING_STD(for_each);
FG_USING_STD(mem_fun_ref);


// Constructor
fgTILE::fgTILE ( void )
    : nodes(new double[MAX_NODES][3]),
      ncount(0),
      used(false)
{
}


// Destructor
fgTILE::~fgTILE ( void ) {
//     free(nodes);
    delete[] nodes;
}

// Step through the fragment list, deleting the display list, then
// the fragment, until the list is empty.
void
fgTILE::release_fragments()
{
    FG_LOG( FG_TERRAIN, FG_DEBUG,
	    "FREEING TILE = (" << tile_bucket << ")" );
    for_each( begin(), end(),
	      mem_fun_ref( &fgFRAGMENT::deleteDisplayList ));
    fragment_list.erase( begin(), end() );
    used = false;
}


// $Log$
// Revision 1.16  1999/03/25 19:03:24  curt
// Converted to use new bucket routines.
//
// Revision 1.15  1999/03/02 01:03:29  curt
// Tweaks for building with native SGI compilers.
//
// Revision 1.14  1999/02/02 20:13:40  curt
// MSVC++ portability changes by Bernie Bright:
//
// Lib/Serial/serial.[ch]xx: Initial Windows support - incomplete.
// Simulator/Astro/stars.cxx: typo? included <stdio> instead of <cstdio>
// Simulator/Cockpit/hud.cxx: Added Standard headers
// Simulator/Cockpit/panel.cxx: Redefinition of default parameter
// Simulator/Flight/flight.cxx: Replaced cout with FG_LOG.  Deleted <stdio.h>
// Simulator/Main/fg_init.cxx:
// Simulator/Main/GLUTmain.cxx:
// Simulator/Main/options.hxx: Shuffled <fg_serial.hxx> dependency
// Simulator/Objects/material.hxx:
// Simulator/Time/timestamp.hxx: VC++ friend kludge
// Simulator/Scenery/tile.[ch]xx: Fixed using std::X declarations
// Simulator/Main/views.hxx: Added a constant
//
// Revision 1.13  1998/11/09 23:40:46  curt
// Bernie Bright <bbright@c031.aone.net.au> writes:
// I've made some changes to the Scenery handling.  Basically just tidy ups.
// The main difference is in tile.[ch]xx where I've changed list<fgFRAGMENT> to
// vector<fgFRAGMENT>.  Studying our usage patterns this seems reasonable.
// Lists are good if you need to insert/delete elements randomly but we
// don't do that.  All access seems to be sequential.  Two additional
// benefits are smaller memory usage - each list element requires pointers
// to the next and previous elements, and faster access - vector iterators
// are smaller and faster than list iterators.  This should also help
// Charlie Hotchkiss' problem when compiling with Borland and STLport.
//
// ./Lib/Bucket/bucketutils.hxx
//   Convenience functions for fgBUCKET.
//
// ./Simulator/Scenery/tile.cxx
// ./Simulator/Scenery/tile.hxx
//   Changed fragment list to a vector.
//   Added some convenience member functions.
//
// ./Simulator/Scenery/tilecache.cxx
// ./Simulator/Scenery/tilecache.hxx
//   use const fgBUCKET& instead of fgBUCKET* where appropriate.
//
// ./Simulator/Scenery/tilemgr.cxx
// ./Simulator/Scenery/tilemgr.hxx
//   uses all the new convenience functions.
//
// Revision 1.12  1998/10/16 00:55:45  curt
// Converted to Point3D class.
//
// Revision 1.11  1998/09/02 14:37:08  curt
// Use erase() instead of while ( size() ) pop_front();
//
// Revision 1.10  1998/08/25 16:52:42  curt
// material.cxx material.hxx obj.cxx obj.hxx texload.c texload.h moved to
//   ../Objects
//
// Revision 1.9  1998/08/24 20:11:39  curt
// Tweaks ...
//
// Revision 1.8  1998/08/22  14:49:58  curt
// Attempting to iron out seg faults and crashes.
// Did some shuffling to fix a initialization order problem between view
// position, scenery elevation.
//
// Revision 1.7  1998/08/20 15:12:05  curt
// Used a forward declaration of classes fgTILE and fgMATERIAL to eliminate
// the need for "void" pointers and casts.
// Quick hack to count the number of scenery polygons that are being drawn.
//
// Revision 1.6  1998/08/12 21:13:05  curt
// material.cxx: don't load textures if they are disabled
// obj.cxx: optimizations from Norman Vine
// tile.cxx: minor tweaks
// tile.hxx: addition of num_faces
// tilemgr.cxx: minor tweaks
//
// Revision 1.5  1998/07/24 21:42:08  curt
// material.cxx: whups, double method declaration with no definition.
// obj.cxx: tweaks to avoid errors in SGI's CC.
// tile.cxx: optimizations by Norman Vine.
// tilemgr.cxx: optimizations by Norman Vine.
//
// Revision 1.4  1998/07/22 21:41:42  curt
// Add basic fgFACE methods contributed by Charlie Hotchkiss.
// intersect optimization from Norman Vine.
//
// Revision 1.3  1998/07/16 17:34:24  curt
// Ground collision detection optimizations contributed by Norman Vine.
//
// Revision 1.2  1998/07/12 03:18:28  curt
// Added ground collision detection.  This involved:
// - saving the entire vertex list for each tile with the tile records.
// - saving the face list for each fragment with the fragment records.
// - code to intersect the current vertical line with the proper face in
//   an efficient manner as possible.
// Fixed a bug where the tiles weren't being shifted to "near" (0,0,0)
//
// Revision 1.1  1998/05/23 14:09:21  curt
// Added tile.cxx and tile.hxx.
// Working on rewriting the tile management system so a tile is just a list
// fragments, and the fragment record contains the display list for that fragment.
//
