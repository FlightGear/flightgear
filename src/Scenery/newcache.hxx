// newcache.hxx -- routines to handle scenery tile caching
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


#ifndef _NEWCACHE_HXX
#define _NEWCACHE_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <map>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/point3d.hxx>

#include "tileentry.hxx"

SG_USING_STD(map);

// A class to store and manage a pile of tiles
class FGNewCache {
public:
    typedef map < long, FGTileEntry * > tile_map;
    typedef tile_map::iterator tile_map_iterator;
    typedef tile_map::const_iterator const_tile_map_iterator;
private:
    // cache storage space
    tile_map tile_cache;

    // maximum cache size
    int max_cache_size;

    // pointers to allow an external linear traversal of cache entries
    tile_map_iterator current;

    // Free a tile cache entry
    void entry_free( long cache_index );

public:
    tile_map_iterator begin() { return tile_cache.begin(); }
    tile_map_iterator end() { return tile_cache.end(); }
    const_tile_map_iterator begin() const { return tile_cache.begin(); }
    const_tile_map_iterator end() const { return tile_cache.end(); }
    
    // Constructor
    FGNewCache();

    // Destructor
    ~FGNewCache();

    // Initialize the tile cache subsystem 
    void init( void );

    // Check if the specified "bucket" exists in the cache
    bool exists( const SGBucket& b ) const;

#if 0
    // Ensure at least one entry is free in the cache
    bool make_space();
#endif

    // Return the index of the oldest tile in the cache, return -1 if
    // nothing available to be removed.
    long get_oldest_tile();

    // Clear the inner ring flag for all tiles in the cache so that
    // the external tile scheduler can flag the inner ring correctly.
    void clear_inner_ring_flags();

    // Clear a cache entry, note that the cache only holds pointers
    // and this does not free the object which is pointed to.
    void clear_entry( long cache_entry );

    // Clear all completely loaded tiles (ignores partially loaded tiles)
    void clear_cache();

    // Return a pointer to the specified tile cache entry 
    inline FGTileEntry *get_tile( const long tile_index ) const {
	const_tile_map_iterator it = tile_cache.find( tile_index );
	if ( it != tile_cache.end() ) {
	    return it->second;
	} else {
	    return NULL;
	}
    }

    // Return a pointer to the specified tile cache entry 
    inline FGTileEntry *get_tile( const SGBucket& b ) const {
	return get_tile( b.gen_index() );
    }

    // Return the cache size
    inline size_t get_size() const { return tile_cache.size(); }

    // External linear traversal of cache
    inline void reset_traversal() { current = tile_cache.begin(); }
    inline bool at_end() { return current == tile_cache.end(); }
    inline FGTileEntry *get_current() const {
	// cout << "index = " << current->first << endl;
	return current->second;
    }
    inline void next() { ++current; }

    inline int get_max_cache_size() const { return max_cache_size; }
    inline void set_max_cache_size( int m ) { max_cache_size = m; }

    /**
     * Create a new tile and enqueue it for loading.
     * @param b 
     * @return success/failure
     */
    bool insert_tile( FGTileEntry* e );
};


#endif // _NEWCACHE_HXX 
