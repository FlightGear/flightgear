// newcache.cxx -- routines to handle scenery tile caching
//
// Written by Curtis Olson, started December 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <GL/gl.h>

#include <plib/ssg.h>		// plib include

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>  // for scenery.center

#include "newcache.hxx"
#include "tileentry.hxx"


SG_USING_NAMESPACE(std);


// Constructor
FGNewCache::FGNewCache( void ) :
    max_cache_size(50)
{
    tile_cache.clear();
}


// Destructor
FGNewCache::~FGNewCache( void ) {
}


// Free a tile cache entry
void FGNewCache::entry_free( long cache_index ) {
    SG_LOG( SG_TERRAIN, SG_DEBUG, "FREEING CACHE ENTRY = " << cache_index );
    FGTileEntry *tile = tile_cache[cache_index];
    tile->disconnect_ssg_nodes();

#ifdef WISH_PLIB_WAS_THREADED
    tile->sched_removal();
#else // plib isn't threaded so we always go here
    tile->free_tile();
    delete tile;
#endif

    tile_cache.erase( cache_index );
}


// Initialize the tile cache subsystem
void FGNewCache::init( void ) {
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing the tile cache." );

    SG_LOG( SG_TERRAIN, SG_INFO, "  max cache size = " 
	    << max_cache_size );
    SG_LOG( SG_TERRAIN, SG_INFO, "  current cache size = " 
	    << tile_cache.size() );

#if 0 // don't clear the cache right now
    clear_cache();
#endif

    SG_LOG( SG_TERRAIN, SG_INFO, "  done with init()"  );
}


// Search for the specified "bucket" in the cache
bool FGNewCache::exists( const SGBucket& b ) const {
    long tile_index = b.gen_index();
    const_tile_map_iterator it = tile_cache.find( tile_index );

    return ( it != tile_cache.end() );
}


// depricated for threading
#if 0
// Fill in a tile cache entry with real data for the specified bucket
void FGNewCache::fill_in( const SGBucket& b ) {
    SG_LOG( SG_TERRAIN, SG_DEBUG, "FILL IN CACHE ENTRY = " << b.gen_index() );

    // clear out a distant entry in the cache if needed.
    make_space();

    // create the entry
    FGTileEntry *e = new FGTileEntry( b );

    // register it in the cache
    long tile_index = b.gen_index();
    tile_cache[tile_index] = e;

    SGPath tile_path;
    if ( globals->get_fg_scenery() != (string)"" ) {
	tile_path.set( globals->get_fg_scenery() );
    } else {
	tile_path.set( globals->get_fg_root() );
	tile_path.append( "Scenery" );
    }
    
    // Load the appropriate data file
    e->load( tile_path, true );
}
#endif


// Ensure at least one entry is free in the cache
void FGNewCache::make_space() {
    SG_LOG( SG_TERRAIN, SG_DEBUG, "Make space in cache" );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "cache entries = " << tile_cache.size() );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "max size = " << max_cache_size );

    if ( (int)tile_cache.size() < max_cache_size ) {
	// space in the cache, return
	return;
    }

    while ( (int)tile_cache.size() >= max_cache_size ) {
	sgdVec3 abs_view_pos;
	float dist;
	float max_dist = 0.0;
	int max_index = -1;

	// we need to free the furthest entry
	tile_map_iterator current = tile_cache.begin();
	tile_map_iterator end = tile_cache.end();
    
	for ( ; current != end; ++current ) {
	    long index = current->first;
	    FGTileEntry *e = current->second;

	    if ( e->is_loaded() && e->get_pending_models() == 0 ) {
		// calculate approximate distance from view point
		sgdCopyVec3( abs_view_pos,
			     globals->get_current_view()->get_abs_view_pos() );

		SG_LOG( SG_TERRAIN, SG_DEBUG, "DIST Abs view pos = " 
			<< abs_view_pos[0] << ","
			<< abs_view_pos[1] << ","
			<< abs_view_pos[2] );
		SG_LOG( SG_TERRAIN, SG_DEBUG,
			"    ref point = " << e->center );

		sgdVec3 center;
		sgdSetVec3( center,
			    e->center.x(), e->center.y(), e->center.z() );
		dist = sgdDistanceVec3( center, abs_view_pos );

		SG_LOG( SG_TERRAIN, SG_DEBUG, "    distance = " << dist );

		if ( dist > max_dist ) {
		    max_dist = dist;
		    max_index = index;
		}
	    }
	}

	// If we made it this far, then there were no open cache entries.
	// We will instead free the furthest cache entry and return it's
	// index.

	if ( max_index >= 0 ) {
	    SG_LOG( SG_TERRAIN, SG_DEBUG, "    max_dist = " << max_dist );
	    SG_LOG( SG_TERRAIN, SG_DEBUG, "    index = " << max_index );
	    entry_free( max_index );
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT, "WHOOPS!!! Dying in make_space()"
                    "tile cache is full, but no entries available to removal.");
	    exit( -1 );
	}
    }
}


// Clear all completely loaded tiles (ignores partially loaded tiles)
void FGNewCache::clear_cache() {
    // This is a hack that should really get cleaned up at some point
    extern ssgBranch *terrain;

    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();
    
    for ( ; current != end; ++current ) {
	long index = current->first;
	SG_LOG( SG_TERRAIN, SG_DEBUG, "clearing " << index );
	FGTileEntry *e = current->second;
        if ( e->is_loaded() && e->get_pending_models() == 0 ) {
            e->tile_bucket.make_bad();
            entry_free(index);
        }
    }

    // and ... just in case we missed something ... 
    terrain->removeAllKids();
}


/**
 * Create a new tile and schedule it for loading.
 */
void
FGNewCache::insert_tile( FGTileEntry *e )
{
    // clear out a distant entry in the cache if needed.
    make_space();

    // register it in the cache
    long tile_index = e->get_tile_bucket().gen_index();
    tile_cache[tile_index] = e;

}
