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
      state(Unused),
      vtlist(NULL),
      vnlist(NULL),
      tclist(NULL)
{
    nodes.clear();
}


// Destructor
FGTileEntry::~FGTileEntry ( void ) {
    // cout << "nodes = " << nodes.size() << endl;;
    // delete[] nodes;
}


// Step through the fragment list, deleting the display list, then the
// fragment, until the list is empty.  Also delete the arrays used by
// ssg as well as the whole ssg branch
void
FGTileEntry::free_tile()
{
    FG_LOG( FG_TERRAIN, FG_DEBUG,
	    "FREEING TILE = (" << tile_bucket << ")" );

    // mark tile unused
    mark_unused();

    // delete fragment list
    for_each( begin(), end(),
	      mem_fun_ref( &fgFRAGMENT::deleteDisplayList ));
    fragment_list.erase( begin(), end() );

    // delete the ssg used structures
    if ( vtlist != NULL ) {
	delete vtlist;
    }
    if ( vnlist != NULL ) {
	delete vnlist;
    }
    if ( tclist != NULL ) {
	delete tclist;
    }

    // delete the ssg branch

    // make sure we have a sane number of parents
    int pcount = select_ptr->getNumParents();
    if ( pcount > 0 ) {
	// find the first parent (should only be one)
	ssgBranch *parent = select_ptr->getParent( 0 ) ;
	// find the number of kids this parent has
	int kcount = parent->getNumKids();
	// find the kid that matches our original select_ptr
	bool found_kid = false;
	for ( int i = 0; i < kcount; ++i ) {
	    ssgEntity *kid = parent->getKid( i );
	    if ( kid == select_ptr ) {
		FG_LOG( FG_TERRAIN, FG_INFO,
			"Found a kid to delete " << kid);
		found_kid = true;
	    }
	}
	if ( ! found_kid ) {
	    FG_LOG( FG_TERRAIN, FG_ALERT,
		    "Couldn't find the kid to delete!  Dying" );
	    exit(-1);
	}
    } else {
	FG_LOG( FG_TERRAIN, FG_ALERT,
		"Parent count is zero for an ssg tile!  Dying" );
	exit(-1);
    }
}


// when a tile is still in the cache, but not in the immediate draw l
// ist, it can still remain in the scene graph, but we use a range
// selector to disable it from ever being drawn.
void 
FGTileEntry::ssg_disable() {
    // cout << "TILE STATE = " << state << endl;
    if ( state == Scheduled_for_use ) {
	state = Scheduled_for_cache;
    } else if ( (state == Loaded) || (state == Cached) ) {
	state = Cached;
	// cout << "DISABLING SSG NODE" << endl;
	select_ptr->select(0);

#if 0
	// set a really tiny range
	// cout << "SETTING TINY RANGE" << endl;
	float ranges[2];
	ranges[0] = 0.0f;
	ranges[1] = 0.00001f;
	range_ptr->setRanges( ranges, 2 );

	// transform to a long way away
	// cout << "MOVING FAR AWAY" << endl;
	sgCoord sgcoord;
	sgSetCoord( &sgcoord, 999999.0, 999999.0, 999999.0, 0.0, 0.0, 0.0 );
	transform_ptr->setTransform( &sgcoord );
#endif 
    } else {
	FG_LOG( FG_TERRAIN, FG_ALERT,
		"Trying to disable an unused tile!  Dying" );
	exit(-1);
    }	
    // cout << "TILE STATE = " << state << endl;
}
