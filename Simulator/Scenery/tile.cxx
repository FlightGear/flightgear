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
    : nodes(new double[FG_MAX_NODES][3]),
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


