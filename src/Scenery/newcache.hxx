// newcache.hxx -- routines to handle scenery tile caching
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


#ifndef _NEWCACHE_HXX
#define _NEWCACHE_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>

#include <map>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/point3d.hxx>

#include "tileentry.hxx"
#include "FGTileLoader.hxx"

SG_USING_STD(map);

// A class to store and manage a pile of tiles
class FGNewCache {

    typedef map < long, FGTileEntry * > tile_map;
    typedef tile_map::iterator tile_map_iterator;
    typedef tile_map::const_iterator const_tile_map_iterator;

    // cache storage space
    tile_map tile_cache;

    // maximum cache size
    int max_cache_size;

    // pointers to allow an external linear traversal of cache entries
    tile_map_iterator current;

    // Free a tile cache entry
    void entry_free( long cache_index );

    /**
     * Queue tiles for loading.
     */
    FGTileLoader loader;

public:

    // Constructor
    FGNewCache();

    // Destructor
    ~FGNewCache();

    // Initialize the tile cache subsystem 
    void init( void );

    // Check if the specified "bucket" exists in the cache
    bool exists( const SGBucket& b ) const;

    // Ensure at least one entry is free in the cache
    void make_space();

    // Fill in a tile cache entry with real data for the specified bucket 
    // void fill_in( const SGBucket& b );

    // Return a pointer to the specified tile cache entry 
    inline FGTileEntry *get_tile( const long tile_index ) {
	tile_map_iterator it = tile_cache.find( tile_index );
	if ( it != tile_cache.end() ) {
	    return it->second;
	} else {
	    return NULL;
	}
    }

    // Return a pointer to the specified tile cache entry 
    inline FGTileEntry *get_tile( const SGBucket& b ) {
	return get_tile( b.gen_index() );
    }

    // Return the cache size
    inline size_t get_size() const { return tile_cache.size(); }

    // External linear traversal of cache
    inline void reset_traversal() { current = tile_cache.begin(); }
    inline bool at_end() { return current == tile_cache.end(); }
    inline FGTileEntry *get_current() {
	// cout << "index = " << current->first << endl;
	return current->second;
    }
    inline void next() { ++current; }

    inline int get_max_cache_size() const { return max_cache_size; }
    inline void set_max_cache_size( int m ) { max_cache_size = m; }

    /**
     * Create a new tile and enqueue it for loading.
     * @param b 
     */
    void load_tile( const SGBucket& b );
};


#endif // _NEWCACHE_HXX 
