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


#if 0
// Ensure at least one entry is free in the cache
bool FGNewCache::make_space() {
    SG_LOG( SG_TERRAIN, SG_DEBUG, "Make space in cache" );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "cache entries = " << tile_cache.size() );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "max size = " << max_cache_size );

    if ( (int)tile_cache.size() < max_cache_size ) {
	// space in the cache, return
	return true;
    }

    while ( (int)tile_cache.size() >= max_cache_size ) {
	sgdVec3 abs_view_pos;
	float dist;
        double timestamp = 0.0;
	int max_index = -1;
        double min_time = 2419200000.0f;  // one month should be enough
        double max_time = 0;

	// we need to free the furthest entry
	tile_map_iterator current = tile_cache.begin();
	tile_map_iterator end = tile_cache.end();
    
	for ( ; current != end; ++current ) {
	    long index = current->first;
	    FGTileEntry *e = current->second;
	    // if ( e->is_loaded() && (e->get_pending_models() == 0) ) {
	    if ( e->is_loaded() ) {

                timestamp = e->get_timestamp();
                if ( timestamp < min_time ) {
                    max_index = index;
                    min_time = timestamp;
                }
                if ( timestamp > max_time ) {
                    max_time = timestamp;
                }

	    } else {
                SG_LOG( SG_TERRAIN, SG_DEBUG, "loaded = " << e->is_loaded()
                        << " pending models = " << e->get_pending_models()
                        << " time stamp = " << e->get_timestamp() );
            }
	}

	// If we made it this far, then there were no open cache entries.
	// We will instead free the oldest cache entry and return true
        
        SG_LOG( SG_TERRAIN, SG_DEBUG, "    min_time = " << min_time );
        SG_LOG( SG_TERRAIN, SG_DEBUG, "    index = " << max_index );
        SG_LOG( SG_TERRAIN, SG_DEBUG, "    max_time = " << max_time );
	if ( max_index >= 0 ) {
	    entry_free( max_index );
	    return true;
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT, "WHOOPS!!! can't make_space(), tile "
                    "cache is full, but no entries available for removal." );
	    return false;
	}
    }

    SG_LOG( SG_TERRAIN, SG_ALERT, "WHOOPS!!! Hit an unhandled condition in  "
            "FGNewCache::make_space()." );
    return false;
}
#endif


// Return the index of the oldest tile in the cache, return -1 if
// nothing available to be removed.
long FGNewCache::get_oldest_tile() {
    // we need to free the furthest entry
    long min_index = -1;
    double timestamp = 0.0;
    double min_time = 2419200000.0f; // one month should be enough
    double max_time = 0;

    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();
    
    for ( ; current != end; ++current ) {
        long index = current->first;
        FGTileEntry *e = current->second;
        if ( e->is_loaded() && (e->get_pending_models() == 0) ) {
            
            timestamp = e->get_timestamp();
            if ( timestamp < min_time ) {
                min_index = index;
                min_time = timestamp;
            }
            if ( timestamp > max_time ) {
                max_time = timestamp;
            }

        } else {
            SG_LOG( SG_TERRAIN, SG_DEBUG, "loaded = " << e->is_loaded()
                    << " pending models = " << e->get_pending_models()
                    << " time stamp = " << e->get_timestamp() );
        }
    }

    SG_LOG( SG_TERRAIN, SG_DEBUG, "    min_time = " << min_time );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "    index = " << min_index );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "    max_time = " << max_time );

    return min_index;
}


// Clear the inner ring flag for all tiles in the cache so that the
// external tile scheduler can flag the inner ring correctly.
void FGNewCache::clear_inner_ring_flags() {
    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();
    
    for ( ; current != end; ++current ) {
        FGTileEntry *e = current->second;
        if ( e->is_loaded() && (e->get_pending_models() == 0) ) {
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
        if ( e->is_loaded() && (e->get_pending_models() == 0) ) {
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
    // set time of insertion for tracking age of tiles...
    e->set_timestamp(globals->get_sim_time_sec());

    // register it in the cache
    long tile_index = e->get_tile_bucket().gen_index();
    tile_cache[tile_index] = e;

    return true;
}


// Note this is the old version of FGNewCache::make_space(), currently disabled
// It uses distance from a center point to determine tiles to be discarded...
#if 0
// Ensure at least one entry is free in the cache
bool FGNewCache::make_space() {
    SG_LOG( SG_TERRAIN, SG_DEBUG, "Make space in cache" );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "cache entries = " << tile_cache.size() );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "max size = " << max_cache_size );

    if ( (int)tile_cache.size() < max_cache_size ) {
	// space in the cache, return
	return true;
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

	    if ( e->is_loaded() && (e->get_pending_models() == 0) ) {
		// calculate approximate distance from view point
		sgdCopyVec3( abs_view_pos,
			     globals->get_current_view()->get_absolute_view_pos() );

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
	    } else {
                SG_LOG( SG_TERRAIN, SG_INFO, "loaded = " << e->is_loaded()
                        << " pending models = " << e->get_pending_models() );
            }
	}

	// If we made it this far, then there were no open cache entries.
	// We will instead free the furthest cache entry and return true
        
        SG_LOG( SG_TERRAIN, SG_INFO, "    max_dist = " << max_dist );
        SG_LOG( SG_TERRAIN, SG_INFO, "    index = " << max_index );
	if ( max_index >= 0 ) {
	    entry_free( max_index );
	    return true;
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT, "WHOOPS!!! can't make_space(), tile "
                    "cache is full, but no entries available for removal." );
	    return false;
	}
    }

    SG_LOG( SG_TERRAIN, SG_ALERT, "WHOOPS!!! Hit an unhandled condition in  "
            "FGNewCache::make_space()." );
    return false;
}
#endif


