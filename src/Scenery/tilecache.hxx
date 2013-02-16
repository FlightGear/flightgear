// TileCache.hxx -- routines to handle scenery tile caching
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


#ifndef _TILECACHE_HXX
#define _TILECACHE_HXX

#include <map>

#include <simgear/bucket/newbucket.hxx>
#include "tileentry.hxx"

using std::map;

// A class to store and manage a pile of tiles
class TileCache {
public:
    typedef map < long, TileEntry * > tile_map;
    typedef tile_map::iterator tile_map_iterator;
    typedef tile_map::const_iterator const_tile_map_iterator;
private:
    // cache storage space
    tile_map tile_cache;

    // maximum cache size
    int max_cache_size;

    // pointers to allow an external linear traversal of cache entries
    tile_map_iterator current;

    double current_time;

    // Free a tile cache entry
    void entry_free( long cache_index );

public:
    tile_map_iterator begin() { return tile_cache.begin(); }
    tile_map_iterator end() { return tile_cache.end(); }
    const_tile_map_iterator begin() const { return tile_cache.begin(); }
    const_tile_map_iterator end() const { return tile_cache.end(); }

    // Constructor
    TileCache();

    // Destructor
    ~TileCache();

    // Initialize the tile cache subsystem
    void init( void );

    // Check if the specified "bucket" exists in the cache
    bool exists( const SGBucket& b ) const;

    // Return the index of a tile to be dropped from the cache, return -1 if
    // nothing available to be removed.
    long get_drop_tile();
    
    // Clear all flags indicating tiles belonging to the current view
    void clear_current_view();

    // Clear a cache entry, note that the cache only holds pointers
    // and this does not free the object which is pointed to.
    void clear_entry( long cache_entry );

    // Refresh/reload a tile when it's already in memory.
    void refresh_tile(long tile_index);

    // Clear all completely loaded tiles (ignores partially loaded tiles)
    void clear_cache();

    // Return a pointer to the specified tile cache entry
    inline TileEntry *get_tile( const long tile_index ) const {
	const_tile_map_iterator it = tile_cache.find( tile_index );
	if ( it != tile_cache.end() ) {
	    return it->second;
	} else {
	    return NULL;
	}
    }

    // Return a pointer to the specified tile cache entry
    inline TileEntry *get_tile( const SGBucket& b ) const {
	return get_tile( b.gen_index() );
    }

    // Return the cache size
    inline size_t get_size() const { return tile_cache.size(); }

    // External linear traversal of cache
    inline void reset_traversal() { current = tile_cache.begin(); }
    inline bool at_end() { return current == tile_cache.end(); }
    inline TileEntry *get_current() const {
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
    bool insert_tile( TileEntry* e );

    void set_current_time(double val) { current_time = val; }
    double get_current_time() const { return current_time; }

    // update tile's priority and expiry time according to current request
    void request_tile(TileEntry* t,float priority,bool current_view,double requesttime);
};

#endif // _TILECACHE_HXX
