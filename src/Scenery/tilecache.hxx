// tilecache.hxx -- routines to handle scenery tile caching
//
// Written by Curtis Olson, started January 1998.
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


#ifndef _TILECACHE_HXX
#define _TILECACHE_HXX


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
#include <simgear/xgl/xgl.h>

#include <vector>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/point3d.hxx>

#include "tileentry.hxx"

FG_USING_STD(vector);


// A class to store and manage a pile of tiles
class FGTileCache {

    // cache storage space
    tile_list tile_cache;

public:

    // Constructor
    FGTileCache( void );

    // Initialize the tile cache subsystem 
    void init( void );

    // Search for the specified "bucket" in the cache 
    int exists( const FGBucket& p );

    // Return index of next available slot in tile cache 
    int next_avail( void );

    // Free a tile cache entry
    void entry_free( int index );

    // Fill in a tile cache entry with real data for the specified bucket 
    void fill_in( int index, const FGBucket& p );

    // Return a pointer to the specified tile cache entry 
    FGTileEntry *get_tile( int index ) {
	return &tile_cache[index];
    }

    // Return the cache size
    inline size_t get_size() const { return tile_cache.size(); }

    // Destructor
    ~FGTileCache( void );
};


// the tile cache
extern FGTileCache global_tile_cache;


#endif // _TILECACHE_HXX 
