// tilecache.cxx -- routines to handle scenery tile caching
//
// Written by Curtis Olson, started January 1998.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
#include <XGL/xgl.h>

#include <Debug/logstream.hxx>
#include <Airports/genapt.hxx>
// #include <Bucket/bucketutils.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Misc/fgpath.hxx>
#include <Objects/obj.hxx>

#include "tile.hxx"
#include "tilecache.hxx"


// the tile cache
fgTILECACHE global_tile_cache;


// Constructor
fgTILECACHE::fgTILECACHE( void ) {
}


// Initialize the tile cache subsystem
void
fgTILECACHE::init( void )
{
    int i;

    FG_LOG( FG_TERRAIN, FG_INFO, "Initializing the tile cache." );

    for ( i = 0; i < FG_TILE_CACHE_SIZE; i++ ) {
	if ( tile_cache[i].used ) {
	    entry_free(i);
	    tile_cache[i].tile_bucket.make_bad();
	}
	tile_cache[i].used = 0;
    }
}


// Search for the specified "bucket" in the cache
int
fgTILECACHE::exists( const FGBucket& p )
{
    int i;

    for ( i = 0; i < FG_TILE_CACHE_SIZE; i++ ) {
	if ( tile_cache[i].tile_bucket == p ) {
	    FG_LOG( FG_TERRAIN, FG_DEBUG, 
		    "TILE EXISTS in cache ... index = " << i );
	    return( i );
	}
    }
    
    return( -1 );
}


// Fill in a tile cache entry with real data for the specified bucket
void
fgTILECACHE::fill_in( int index, FGBucket& p )
{
    // Load the appropriate data file and build tile fragment list
    FGPath tile_path( current_options.get_fg_root() );
    tile_path.append( "Scenery" );
    tile_path.append( p.gen_base_path() );
    tile_path.append( p.gen_index_str() );

    tile_cache[index].used = true;
    tile_cache[index].tile_bucket = p;
    fgObjLoad( tile_path.str(), &tile_cache[index] );
//     tile_cache[ index ].ObjLoad( tile_path, p );

    // cout << " ncount before = " << tile_cache[index].ncount << "\n";
    // cout << " fragments before = " << tile_cache[index].fragment_list.size()
    //      << "\n";

    string apt_path = tile_path.str() + ".apt";
    fgAptGenerate( apt_path, &tile_cache[index] );

    // cout << " ncount after = " << tile_cache[index].ncount << "\n";
    // cout << " fragments after = " << tile_cache[index].fragment_list.size()
    //      << "\n";
}


// Free a tile cache entry
void
fgTILECACHE::entry_free( int index )
{
    tile_cache[index].release_fragments();
}


// Return index of next available slot in tile cache
int
fgTILECACHE::next_avail( void )
{
    Point3D delta, abs_view_pos;
    int i;
    float max, med, min, tmp;
    float dist, max_dist;
    int max_index;
    
    max_dist = 0.0;
    max_index = 0;

    for ( i = 0; i < FG_TILE_CACHE_SIZE; i++ ) {
	if ( ! tile_cache[i].used ) {
	    return(i);
	} else {
	    // calculate approximate distance from view point
	    abs_view_pos = current_view.get_abs_view_pos();

	    FG_LOG( FG_TERRAIN, FG_DEBUG,
		    "DIST Abs view pos = " << abs_view_pos );
	    FG_LOG( FG_TERRAIN, FG_DEBUG,
		    "    ref point = " << tile_cache[i].center );

	    delta.setx( fabs(tile_cache[i].center.x() - abs_view_pos.x() ) );
	    delta.sety( fabs(tile_cache[i].center.y() - abs_view_pos.y() ) );
	    delta.setz( fabs(tile_cache[i].center.z() - abs_view_pos.z() ) );

	    max = delta.x(); med = delta.y(); min = delta.z();
	    if ( max < med ) {
		tmp = max; max = med; med = tmp;
	    }
	    if ( max < min ) {
		tmp = max; max = min; min = tmp;
	    }
	    dist = max + (med + min) / 4;

	    FG_LOG( FG_TERRAIN, FG_DEBUG, "    distance = " << dist );

	    if ( dist > max_dist ) {
		max_dist = dist;
		max_index = i;
	    }
	}
    }

    // If we made it this far, then there were no open cache entries.
    // We will instead free the furthest cache entry and return it's
    // index.
    
    entry_free( max_index );
    return( max_index );
}


// Destructor
fgTILECACHE::~fgTILECACHE( void ) {
}


