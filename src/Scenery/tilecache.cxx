// tilecache.cxx -- routines to handle scenery tile caching
//
// Written by Curtis Olson, started January 1998.
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - curt@flightgear.org
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
#include <simgear/xgl/xgl.h>

#include <plib/ssg.h>		// plib include

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgstream.hxx>
#include <simgear/misc/fgpath.hxx>

#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Objects/obj.hxx>
#include <Scenery/scenery.hxx>  // for scenery.center

#include "tilecache.hxx"
#include "tileentry.hxx"

FG_USING_NAMESPACE(std);

// a cheesy hack (to be fixed later)
extern ssgBranch *terrain;
extern ssgEntity *penguin;


// the tile cache
FGTileCache global_tile_cache;


// Constructor
FGTileCache::FGTileCache( void ) {
    tile_cache.clear();
}


// Initialize the tile cache subsystem
void
FGTileCache::init( void )
{
    int i;

    FG_LOG( FG_TERRAIN, FG_INFO, "Initializing the tile cache." );

    // expand cache if needed.  For best results ... i.e. to avoid
    // tile load problems and blank areas: 
    // 
    //   target_cache_size >= (current.options.tile_diameter + 1) ** 2 
    // 
    int side = current_options.get_tile_diameter() + 2;
    int target_cache_size = (side*side);
    FG_LOG( FG_TERRAIN, FG_DEBUG, "  target cache size = " 
	    << target_cache_size );
    FG_LOG( FG_TERRAIN, FG_DEBUG, "  current cache size = " 
	    << tile_cache.size() );
    FGTileEntry e;
    e.mark_unused();
    e.vec3_ptrs.clear();
    e.vec2_ptrs.clear();
    e.index_ptrs.clear();
    
    FG_LOG( FG_TERRAIN, FG_DEBUG, "  size of tile = " 
	    << sizeof( e ) );
    if ( target_cache_size > (int)tile_cache.size() ) {
	// FGTileEntry e;
	int expansion_amt = target_cache_size - (int)tile_cache.size();
	for ( i = 0; i < expansion_amt; ++i ) {
	    tile_cache.push_back( e );
	    FG_LOG( FG_TERRAIN, FG_DEBUG, "  expanding cache size = " 
		    << tile_cache.size() );
	}
    }
    FG_LOG( FG_TERRAIN, FG_DEBUG, "  done expanding cache, size = " 
	    << tile_cache.size() );

    for ( i = 0; i < (int)tile_cache.size(); i++ ) {
	if ( !tile_cache[i].is_unused() ) {
	    entry_free(i);
	}
	tile_cache[i].mark_unused();
	tile_cache[i].tile_bucket.make_bad();
    }

    // and ... just in case we missed something ... 
    terrain->removeAllKids();

    FG_LOG( FG_TERRAIN, FG_DEBUG, "  done with init()"  );
}


// Search for the specified "bucket" in the cache
int
FGTileCache::exists( const FGBucket& p )
{
    int i;

    for ( i = 0; i < (int)tile_cache.size(); i++ ) {
	if ( tile_cache[i].tile_bucket == p ) {
	    FG_LOG( FG_TERRAIN, FG_DEBUG, 
		    "TILE EXISTS in cache ... index = " << i );
	    return( i );
	}
    }
    
    return( -1 );
}


#if 0
static void print_refs( ssgSelector *sel, ssgTransform *trans, 
		 ssgRangeSelector *range) 
{
    cout << "selector -> " << sel->getRef()
	 << "  transform -> " << trans->getRef()
	 << "  range -> " << range->getRef() << endl;
}
#endif


// Fill in a tile cache entry with real data for the specified bucket
void
FGTileCache::fill_in( int index, const FGBucket& p )
{
    // cout << "FILL IN CACHE ENTRY = " << index << endl;

    tile_cache[index].center = Point3D( 0.0 );
    if ( tile_cache[index].vec3_ptrs.size() || 
	 tile_cache[index].vec2_ptrs.size() || 
	 tile_cache[index].index_ptrs.size() )
    {
	FG_LOG( FG_TERRAIN, FG_ALERT, 
		"Attempting to overwrite existing or"
		<< " not properly freed leaf data." );
	exit(-1);
    }

    tile_cache[index].select_ptr = new ssgSelector;
    tile_cache[index].transform_ptr = new ssgTransform;
    tile_cache[index].range_ptr = new ssgRangeSelector;
    tile_cache[index].tile_bucket = p;

    FGPath tile_path( current_options.get_fg_scenery() );
    tile_path.append( p.gen_base_path() );
    
    // Load the appropriate data file
    FGPath tile_base = tile_path;
    tile_base.append( p.gen_index_str() );
    ssgBranch *new_tile = fgObjLoad( tile_base.str(), &tile_cache[index], 
				     true );

    if ( new_tile != NULL ) {
	tile_cache[index].range_ptr->addKid( new_tile );
    }
  
    // load custom objects
    cout << "CUSTOM OBJECTS" << endl;

    FGPath index_path = tile_path;
    index_path.append( p.gen_index_str() );
    index_path.concat( ".ind" );

    cout << "Looking in " << index_path.str() << endl;

    fg_gzifstream in( index_path.str() );

    if ( in.is_open() ) {
	string token, name;

	while ( ! in.eof() ) {
	    in >> token;
	    in >> name;
#if defined ( MACOS ) || defined ( _MSC_VER )
	    in >> ::skipws;
#else
	    in >> skipws;
#endif
	    cout << "token = " << token << " name = " << name << endl;

	    FGPath custom_path = tile_path;
	    custom_path.append( name );
	    ssgBranch *custom_obj = fgObjLoad( custom_path.str(),
					       &tile_cache[index], false );
	    if ( (new_tile != NULL) && (custom_obj != NULL) ) {
		new_tile -> addKid( custom_obj );
	    }
	}
    }

    tile_cache[index].transform_ptr->addKid( tile_cache[index].range_ptr );

    // calculate initial tile offset
    tile_cache[index].SetOffset( scenery.center );
    sgCoord sgcoord;
    sgSetCoord( &sgcoord,
		tile_cache[index].offset.x(), 
		tile_cache[index].offset.y(), tile_cache[index].offset.z(),
		0.0, 0.0, 0.0 );
    tile_cache[index].transform_ptr->setTransform( &sgcoord );

    tile_cache[index].select_ptr->addKid( tile_cache[index].transform_ptr );
    terrain->addKid( tile_cache[index].select_ptr );

    if ( tile_cache[index].is_scheduled_for_cache() ) {
	// cout << "FOUND ONE SCHEDULED FOR CACHE" << endl;
	// load, but not needed now so disable
	tile_cache[index].mark_loaded();
	tile_cache[index].ssg_disable();
	tile_cache[index].select_ptr->select(0);
    } else {
	// cout << "FOUND ONE READY TO LOAD" << endl;
	tile_cache[index].mark_loaded();
	tile_cache[index].select_ptr->select(1);
    }
}


// Free a tile cache entry
void
FGTileCache::entry_free( int cache_index )
{
    // cout << "FREEING CACHE ENTRY = " << cache_index << endl;
    tile_cache[cache_index].free_tile();
}


// Return index of next available slot in tile cache
int
FGTileCache::next_avail( void )
{
    // Point3D delta;
    Point3D abs_view_pos;
    int i;
    // float max, med, min, tmp;
    float dist, max_dist;
    int max_index;
    
    max_dist = 0.0;
    max_index = -1;

    for ( i = 0; i < (int)tile_cache.size(); i++ ) {
	// only look at freeing NON-scheduled (i.e. ready to load
	// cache entries.  This assumes that the cache is always big
	// enough for our tile radius!

	if ( tile_cache[i].is_unused() ) {
	    // favor unused cache slots
	    return(i);
	} else if ( tile_cache[i].is_loaded() || tile_cache[i].is_cached() ) {
	    // calculate approximate distance from view point
	    abs_view_pos = current_view.get_abs_view_pos();

	    FG_LOG( FG_TERRAIN, FG_DEBUG,
		    "DIST Abs view pos = " << abs_view_pos );
	    FG_LOG( FG_TERRAIN, FG_DEBUG,
		    "    ref point = " << tile_cache[i].center );

	    /*
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
	    */

	    dist = tile_cache[i].center.distance3D( abs_view_pos );

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

    if ( max_index >=0 ) {
	FG_LOG( FG_TERRAIN, FG_DEBUG, "    max_dist = " << max_dist );
	FG_LOG( FG_TERRAIN, FG_DEBUG, "    index = " << max_index );
	entry_free( max_index );
	return( max_index );
    } else {
	FG_LOG( FG_TERRAIN, FG_ALERT, "WHOOPS!!! Dying in next_avail()" );
	exit( -1 );
    }

    // avoid a potential compiler warning
    return -1;
}


// Destructor
FGTileCache::~FGTileCache( void ) {
}


