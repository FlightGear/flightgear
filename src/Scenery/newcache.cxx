// newcache.cxx -- routines to handle scenery tile caching
//
// Written by Curtis Olson, started December 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>  // for scenery.center

#include "newcache.hxx"
#include "tileentry.hxx"


SG_USING_NAMESPACE(std);


// Constructor
FGNewCache::FGNewCache( void ) :
    max_cache_size(100)
{
    tile_cache.clear();
}


// Destructor
FGNewCache::~FGNewCache( void ) {
    clear_cache();
}


// Free a tile cache entry
void FGNewCache::entry_free( long cache_index ) {
    SG_LOG( SG_TERRAIN, SG_DEBUG, "FREEING CACHE ENTRY = " << cache_index );
    FGTileEntry *tile = tile_cache[cache_index];
    tile->disconnect_ssg_nodes();

#ifdef WISH_PLIB_WAS_THREADED // but it isn't
    tile->sched_removal();
#else
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

#if 0 // don't clear the cache
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

// Return the index of the oldest tile in the cache, return -1 if
// nothing available to be removed.
long FGNewCache::get_oldest_tile() {
    // we need to free the furthest entry
    long min_index = -1;
    double timestamp = 0.0;
    double min_time = DBL_MAX;

    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();
    
    for ( ; current != end; ++current ) {
        long index = current->first;
        FGTileEntry *e = current->second;
        if ( e->is_loaded() ) {
            
            timestamp = e->get_timestamp();
            if ( timestamp < min_time ) {
                min_time = timestamp;
                min_index = index;
            }

        } else {
            SG_LOG( SG_TERRAIN, SG_DEBUG, "loaded = " << e->is_loaded()
                    << " time stamp = " << e->get_timestamp() );
        }
    }

    SG_LOG( SG_TERRAIN, SG_DEBUG, "    index = " << min_index );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "    min_time = " << min_time );

    return min_index;
}


// Clear the inner ring flag for all tiles in the cache so that the
// external tile scheduler can flag the inner ring correctly.
void FGNewCache::clear_inner_ring_flags() {
    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();
    
    for ( ; current != end; ++current ) {
        FGTileEntry *e = current->second;
        if ( e->is_loaded() ) {
            e->set_inner_ring( false );
        }
    }
}

// Clear a cache entry, note that the cache only holds pointers
// and this does not free the object which is pointed to.
void FGNewCache::clear_entry( long cache_index ) {
    tile_cache.erase( cache_index );
}


// Clear all completely loaded tiles (ignores partially loaded tiles)
void FGNewCache::clear_cache() {
    std::vector<long> indexList;
    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();
    
    for ( ; current != end; ++current ) {
	long index = current->first;
	SG_LOG( SG_TERRAIN, SG_DEBUG, "clearing " << index );
	FGTileEntry *e = current->second;
        if ( e->is_loaded() ) {
            e->tile_bucket.make_bad();
	    // entry_free modifies tile_cache, so store index and call entry_free() later;
	    indexList.push_back( index);
        }
    }
    for (unsigned int it = 0; it < indexList.size(); it++) {
	entry_free( indexList[ it]);
    }
    // and ... just in case we missed something ... 
    osg::Group* group = globals->get_scenery()->get_terrain_branch();
    group->removeChildren(0, group->getNumChildren());
}


/**
 * Create a new tile and schedule it for loading.
 */
bool FGNewCache::insert_tile( FGTileEntry *e ) {
    // register tile in the cache
    long tile_index = e->get_tile_bucket().gen_index();
    // not needed if timestamps are updated in cull-callback
    // e->set_timestamp(globals->get_sim_time_sec());
    tile_cache[tile_index] = e;

    return true;
}


