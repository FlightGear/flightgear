// tile.cxx -- routines to handle a scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998, 1999  Curtis L. Olson  - curt@flightgear.org
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

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_FUNCTIONAL
#include STL_ALGORITHM

#include <Debug/logstream.hxx>
#include <Bucket/newbucket.hxx>

#include "tileentry.hxx"

FG_USING_STD(for_each);
FG_USING_STD(mem_fun_ref);


// Constructor
FGTileEntry::FGTileEntry ( void )
    : ncount(0),
      state(Unused)
{
    nodes.clear();
}


// Destructor
FGTileEntry::~FGTileEntry ( void ) {
    // cout << "nodes = " << nodes.size() << endl;;
    // delete[] nodes;
}


// recurse an ssg tree and call removeKid() on every node from the
// bottom up.  Leaves the original branch in existance, but empty so
// it can be removed by the calling routine.
static void my_remove_branch( ssgBranch * branch ) {
    for ( ssgEntity *k = branch->getKid( 0 );
	  k != NULL; 
	  k = branch->getNextKid() )
    {
	if ( k -> isAKindOf ( ssgTypeBranch() ) ) {
	    my_remove_branch( (ssgBranch *)k );
	    branch -> removeKid ( k );
	} else if ( k -> isAKindOf ( ssgTypeLeaf() ) ) {
	    branch -> removeKid ( k ) ;
	}
    }
}


// Step through the fragment list, deleting the display list, then the
// fragment, until the list is empty.  Also delete the arrays used by
// ssg as well as the whole ssg branch
void FGTileEntry::free_tile() {
    int i;
    FG_LOG( FG_TERRAIN, FG_DEBUG,
	    "FREEING TILE = (" << tile_bucket << ")" );

    // mark tile unused
    mark_unused();

    // delete fragment list and node list
    FG_LOG( FG_TERRAIN, FG_DEBUG,
	    "  deleting " << fragment_list.size() << " fragments" );
    fragment_list.clear();
    FG_LOG( FG_TERRAIN, FG_DEBUG,
	    "  deleting " << nodes.size() << " nodes" );
    nodes.clear();

    // delete the ssg structures
    FG_LOG( FG_TERRAIN, FG_DEBUG,
	    "  deleting (leaf data) vertex, normal, and "
	    << " texture coordinate arrays" );

    for ( i = 0; i < (int)vec3_ptrs.size(); ++i ) {
	delete vec3_ptrs[i];
    }
    vec3_ptrs.clear();

    for ( i = 0; i < (int)vec2_ptrs.size(); ++i ) {
	delete vec2_ptrs[i];
    }
    vec2_ptrs.clear();

    for ( i = 0; i < (int)index_ptrs.size(); ++i ) {
	delete index_ptrs[i];
    }
    index_ptrs.clear();

    // delete the ssg branch

    int pcount = select_ptr->getNumParents();
    if ( pcount > 0 ) {
	// find the first parent (should only be one)
	ssgBranch *parent = select_ptr->getParent( 0 ) ;
	if( parent ) {
	    my_remove_branch( select_ptr );
	    parent->removeKid( select_ptr );
	} else {
	    FG_LOG( FG_TERRAIN, FG_ALERT,
		    "parent pointer is NULL!  Dying" );
	    exit(-1);
	}
    } else {
	FG_LOG( FG_TERRAIN, FG_ALERT,
		"Parent count is zero for an ssg tile!  Dying" );
	exit(-1);
    }
}


// when a tile is still in the cache, but not in the immediate draw
// list, it can still remain in the scene graph, but we use a range
// selector to disable it from ever being drawn.
void 
FGTileEntry::ssg_disable() {
    // cout << "TILE STATE = " << state << endl;
    if ( state == Scheduled_for_use ) {
	state = Scheduled_for_cache;
    } else if ( state == Scheduled_for_cache ) {
	// do nothing
    } else if ( (state == Loaded) || (state == Cached) ) {
	state = Cached;
	// cout << "DISABLING SSG NODE" << endl;
	select_ptr->select(0);
    } else {
	FG_LOG( FG_TERRAIN, FG_ALERT,
		"Trying to disable an unused tile!  Dying" );
	exit(-1);
    }	
    // cout << "TILE STATE = " << state << endl;
}
