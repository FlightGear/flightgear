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
#include <simgear/xgl/xgl.h>

#include <plib/ssg.h>		// plib include

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/fgstream.hxx>
#include <simgear/misc/fgpath.hxx>

#include <Main/globals.hxx>
#include <Objects/matlib.hxx>
#include <Objects/newmat.hxx>
#include <Objects/obj.hxx>
#include <Scenery/scenery.hxx>  // for scenery.center

#include "newcache.hxx"
#include "tileentry.hxx"
#include "tilemgr.hxx"		// temp, need to delete later

FG_USING_NAMESPACE(std);

// a cheesy hack (to be fixed later)
extern ssgBranch *terrain;
extern ssgBranch *ground;


// the tile cache
FGNewCache global_tile_cache;


// Constructor
FGNewCache::FGNewCache( void ) {
    tile_cache.clear();
}


// Destructor
FGNewCache::~FGNewCache( void ) {
}


// Free a tile cache entry
void FGNewCache::entry_free( long cache_index ) {
    FG_LOG( FG_TERRAIN, FG_INFO, "FREEING CACHE ENTRY = " << cache_index );
    FGTileEntry *e = tile_cache[cache_index];
    e->free_tile();
    delete( e );
    tile_cache.erase( cache_index );
}


// Initialize the tile cache subsystem
void FGNewCache::init( void ) {
    FG_LOG( FG_TERRAIN, FG_INFO, "Initializing the tile cache." );

    // expand cache if needed.  For best results ... i.e. to avoid
    // tile load problems and blank areas: 
    max_cache_size = 50;	// a random number to start with
    FG_LOG( FG_TERRAIN, FG_INFO, "  max cache size = " 
	    << max_cache_size );
    FG_LOG( FG_TERRAIN, FG_INFO, "  current cache size = " 
	    << tile_cache.size() );
    
    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();
    
    for ( ; current != end; ++current ) {
	long index = current->first;
	cout << "clearing " << index << endl;
	FGTileEntry *e = current->second;
	e->tile_bucket.make_bad();
	entry_free(index);
    }

    // and ... just in case we missed something ... 
    terrain->removeAllKids();

    FG_LOG( FG_TERRAIN, FG_INFO, "  done with init()"  );
}


// Search for the specified "bucket" in the cache
bool FGNewCache::exists( const FGBucket& b ) {
    long tile_index = b.gen_index();
    tile_map_iterator it = tile_cache.find( tile_index );

    return ( it != tile_cache.end() );
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


static ssgLeaf *gen_lights( ssgVertexArray *lights, float bright ) {
    // Allocate ssg structure
    ssgNormalArray   *nl = NULL;
    ssgTexCoordArray *tl = NULL;
    ssgColourArray   *cl = new ssgColourArray( 1 );

    // default to slightly yellow lights for now
    sgVec4 color;
    sgSetVec4( color, 1.0 * bright, 1.0 * bright, 0.7 * bright, 1.0 );
    cl->add( color );

    // create ssg leaf
    ssgLeaf *leaf = 
	new ssgVtxTable ( GL_POINTS, lights, nl, tl, cl );

    // assign state
    FGNewMat *newmat = material_lib.find( "LIGHTS" );
    leaf->setState( newmat->get_state() );

    return leaf;
}


// Fill in a tile cache entry with real data for the specified bucket
void FGNewCache::fill_in( const FGBucket& b ) {
    FG_LOG( FG_TERRAIN, FG_INFO, "FILL IN CACHE ENTRY = " << b.gen_index() );

    // clear out a distant entry in the cache if needed.
    make_space();

    // create the entry
    FGTileEntry *e = new FGTileEntry;

    // register it in the cache
    long tile_index = b.gen_index();
    tile_cache[tile_index] = e;

    // update the contents
    e->center = Point3D( 0.0 );
    if ( e->vec3_ptrs.size() || e->vec2_ptrs.size() || e->index_ptrs.size() ) {
	FG_LOG( FG_TERRAIN, FG_ALERT, 
		"Attempting to overwrite existing or"
		<< " not properly freed leaf data." );
	exit(-1);
    }

    e->terra_transform = new ssgTransform;
    e->terra_range = new ssgRangeSelector;
    e->tile_bucket = b;

    FGPath tile_path;
    if ( globals->get_options()->get_fg_scenery() != "" ) {
	tile_path.set( globals->get_options()->get_fg_scenery() );
    } else {
	tile_path.set( globals->get_options()->get_fg_root() );
	tile_path.append( "Scenery" );
    }
    tile_path.append( b.gen_base_path() );
    
    // fgObjLoad will generate ground lighting for us ...
    ssgVertexArray *light_pts = new ssgVertexArray( 100 );

    // Load the appropriate data file
    FGPath tile_base = tile_path;
    tile_base.append( b.gen_index_str() );
    ssgBranch *new_tile = fgObjLoad( tile_base.str(), e, light_pts, true );

    if ( new_tile != NULL ) {
	e->terra_range->addKid( new_tile );
    }
  
    // load custom objects
    cout << "CUSTOM OBJECTS" << endl;

    FGPath index_path = tile_path;
    index_path.append( b.gen_index_str() );
    index_path.concat( ".ind" );

    cout << "Looking in " << index_path.str() << endl;

    fg_gzifstream in( index_path.str() );

    if ( in.is_open() ) {
	string token, name;

	while ( ! in.eof() ) {
	    in >> token;
	    in >> name;
#if defined ( macintosh ) || defined ( _MSC_VER )
	    in >> ::skipws;
#else
	    in >> skipws;
#endif
	    cout << "token = " << token << " name = " << name << endl;

	    FGPath custom_path = tile_path;
	    custom_path.append( name );
	    ssgBranch *custom_obj = 
		fgObjLoad( custom_path.str(), e, NULL, false );
	    if ( (new_tile != NULL) && (custom_obj != NULL) ) {
		new_tile -> addKid( custom_obj );
	    }
	}
    }

    e->terra_transform->addKid( e->terra_range );

    // calculate initial tile offset
    e->SetOffset( scenery.center );
    sgCoord sgcoord;
    sgSetCoord( &sgcoord,
		e->offset.x(), e->offset.y(), e->offset.z(),
		0.0, 0.0, 0.0 );
    e->terra_transform->setTransform( &sgcoord );
    terrain->addKid( e->terra_transform );

    e->lights_transform = NULL;
    e->lights_range = NULL;
    /* uncomment this section for testing ground lights */
    if ( light_pts->getNum() ) {
	cout << "generating lights" << endl;
	e->lights_transform = new ssgTransform;
	e->lights_range = new ssgRangeSelector;
	e->lights_brightness = new ssgSelector;
	ssgLeaf *lights;

	lights = gen_lights( light_pts, 0.6 );
	e->lights_brightness->addKid( lights );

	lights = gen_lights( light_pts, 0.8 );
	e->lights_brightness->addKid( lights );

	lights = gen_lights( light_pts, 1.0 );
	e->lights_brightness->addKid( lights );

	e->lights_range->addKid( e->lights_brightness );
	e->lights_transform->addKid( e->lights_range );
	e->lights_transform->setTransform( &sgcoord );
	ground->addKid( e->lights_transform );
    }
    /* end of ground light section */
}


// Ensure at least one entry is free in the cache
void FGNewCache::make_space() {
    FG_LOG( FG_TERRAIN, FG_INFO, "Make space in cache" );

    
    cout << "cache size = " << tile_cache.size() << endl;
    cout << "max size = " << max_cache_size << endl;

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

	    // calculate approximate distance from view point
	    sgdCopyVec3( abs_view_pos,
			 globals->get_current_view()->get_abs_view_pos() );

	    FG_LOG( FG_TERRAIN, FG_DEBUG, "DIST Abs view pos = " 
		    << abs_view_pos[0] << ","
		    << abs_view_pos[1] << ","
		    << abs_view_pos[2] );
	    FG_LOG( FG_TERRAIN, FG_DEBUG,
		    "    ref point = " << e->center );

	    sgdVec3 center;
	    sgdSetVec3( center, e->center.x(), e->center.y(), e->center.z() );
	    dist = sgdDistanceVec3( center, abs_view_pos );

	    FG_LOG( FG_TERRAIN, FG_DEBUG, "    distance = " << dist );

	    if ( dist > max_dist ) {
		max_dist = dist;
		max_index = index;
	    }
	}

	// If we made it this far, then there were no open cache entries.
	// We will instead free the furthest cache entry and return it's
	// index.

	if ( max_index >= 0 ) {
	    FG_LOG( FG_TERRAIN, FG_DEBUG, "    max_dist = " << max_dist );
	    FG_LOG( FG_TERRAIN, FG_DEBUG, "    index = " << max_index );
	    entry_free( max_index );
	} else {
	    FG_LOG( FG_TERRAIN, FG_ALERT, "WHOOPS!!! Dying in next_avail()" );
	    exit( -1 );
	}
    }
}
