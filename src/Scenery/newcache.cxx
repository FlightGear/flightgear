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
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Objects/matlib.hxx>
#include <Objects/newmat.hxx>
#include <Objects/obj.hxx>
#include <Scenery/scenery.hxx>  // for scenery.center

#include "newcache.hxx"
#include "tileentry.hxx"
#include "tilemgr.hxx"		// temp, need to delete later

SG_USING_NAMESPACE(std);

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
    SG_LOG( SG_TERRAIN, SG_INFO, "FREEING CACHE ENTRY = " << cache_index );
    FGTileEntry *e = tile_cache[cache_index];
    e->free_tile();
    delete( e );
    tile_cache.erase( cache_index );
}


// Initialize the tile cache subsystem
void FGNewCache::init( void ) {
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing the tile cache." );

    // expand cache if needed.  For best results ... i.e. to avoid
    // tile load problems and blank areas: 
    max_cache_size = 50;	// a random number to start with
    SG_LOG( SG_TERRAIN, SG_INFO, "  max cache size = " 
	    << max_cache_size );
    SG_LOG( SG_TERRAIN, SG_INFO, "  current cache size = " 
	    << tile_cache.size() );
    
    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();
    
    for ( ; current != end; ++current ) {
	long index = current->first;
	SG_LOG( SG_TERRAIN, SG_DEBUG, "clearing " << index );
	FGTileEntry *e = current->second;
	e->tile_bucket.make_bad();
	entry_free(index);
    }

    // and ... just in case we missed something ... 
    terrain->removeAllKids();

    SG_LOG( SG_TERRAIN, SG_INFO, "  done with init()"  );
}


// Search for the specified "bucket" in the cache
bool FGNewCache::exists( const SGBucket& b ) {
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


static ssgLeaf *gen_lights( ssgVertexArray *lights, int inc, float bright ) {
    // generate a repeatable random seed
    float *p1 = lights->get( 0 );
    unsigned int *seed = (unsigned int *)p1;
    sg_srandom( *seed );

    int size = lights->getNum() / inc;

    // Allocate ssg structure
    ssgVertexArray   *vl = new ssgVertexArray( size + 1 );
    ssgNormalArray   *nl = NULL;
    ssgTexCoordArray *tl = NULL;
    ssgColourArray   *cl = new ssgColourArray( size + 1 );

    sgVec4 color;
    for ( int i = 0; i < lights->getNum(); ++i ) {
	// this loop is slightly less efficient than it otherwise
	// could be, but we want a red light to always be red, and a
	// yellow light to always be yellow, etc. so we are trying to
	// preserve the random sequence.
	float zombie = sg_random();
	if ( i % inc == 0 ) {
	    vl->add( lights->get(i) );

	    // factor = sg_random() ^ 2, range = 0 .. 1 concentrated towards 0
	    float factor = sg_random();
	    factor *= factor;

	    if ( zombie > 0.5 ) {
		// 50% chance of yellowish
		sgSetVec4( color, 0.9, 0.9, 0.3, bright - factor * 0.2 );
	    } else if ( zombie > 0.15 ) {
		// 35% chance of whitish
		sgSetVec4( color, 0.9, 0.9, 0.8, bright - factor * 0.2 );
	    } else if ( zombie > 0.05 ) {
		// 10% chance of orangish
		sgSetVec4( color, 0.9, 0.6, 0.2, bright - factor * 0.2 );
	    } else {
		// 5% chance of redish
		sgSetVec4( color, 0.9, 0.2, 0.2, bright - factor * 0.2 );
	    }
	    cl->add( color );
	}
    }

    // create ssg leaf
    ssgLeaf *leaf = 
	new ssgVtxTable ( GL_POINTS, vl, nl, tl, cl );

    // assign state
    FGNewMat *newmat = material_lib.find( "LIGHTS" );
    leaf->setState( newmat->get_state() );

    return leaf;
}


// Fill in a tile cache entry with real data for the specified bucket
void FGNewCache::fill_in( const SGBucket& b ) {
    SG_LOG( SG_TERRAIN, SG_INFO, "FILL IN CACHE ENTRY = " << b.gen_index() );

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
	SG_LOG( SG_TERRAIN, SG_ALERT, 
		"Attempting to overwrite existing or"
		<< " not properly freed leaf data." );
	exit(-1);
    }

    e->terra_transform = new ssgTransform;
    e->terra_range = new ssgRangeSelector;
    e->tile_bucket = b;

    SGPath tile_path;
    if ( globals->get_fg_scenery() != (string)"" ) {
	tile_path.set( globals->get_fg_scenery() );
    } else {
	tile_path.set( globals->get_fg_root() );
	tile_path.append( "Scenery" );
    }
    tile_path.append( b.gen_base_path() );
    
    // fgObjLoad will generate ground lighting for us ...
    ssgVertexArray *light_pts = new ssgVertexArray( 100 );

    // Load the appropriate data file
    SGPath tile_base = tile_path;
    tile_base.append( b.gen_index_str() );
    ssgBranch *new_tile = fgObjLoad( tile_base.str(), e, light_pts, true );

    if ( new_tile != NULL ) {
	e->terra_range->addKid( new_tile );
    }
  
    // load custom objects
    SG_LOG( SG_TERRAIN, SG_DEBUG, "CUSTOM OBJECTS" );

    SGPath index_path = tile_path;
    index_path.append( b.gen_index_str() );
    index_path.concat( ".ind" );

    SG_LOG( SG_TERRAIN, SG_DEBUG, "Looking in " << index_path.str() );

    sg_gzifstream in( index_path.str() );

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
	    SG_LOG( SG_TERRAIN, SG_DEBUG, "token = " << token
		    << " name = " << name );

	    SGPath custom_path = tile_path;
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
	SG_LOG( SG_TERRAIN, SG_DEBUG, "generating lights" );
	e->lights_transform = new ssgTransform;
	e->lights_range = new ssgRangeSelector;
	e->lights_brightness = new ssgSelector;
	ssgLeaf *lights;

	lights = gen_lights( light_pts, 4, 0.7 );
	e->lights_brightness->addKid( lights );

	lights = gen_lights( light_pts, 2, 0.85 );
	e->lights_brightness->addKid( lights );

	lights = gen_lights( light_pts, 1, 1.0 );
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
    SG_LOG( SG_TERRAIN, SG_INFO, "Make space in cache" );

    
    SG_LOG( SG_TERRAIN, SG_DEBUG, "cache entries = " << tile_cache.size() );
    SG_LOG( SG_TERRAIN, SG_INFO, "max size = " << max_cache_size );

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

	    SG_LOG( SG_TERRAIN, SG_DEBUG, "DIST Abs view pos = " 
		    << abs_view_pos[0] << ","
		    << abs_view_pos[1] << ","
		    << abs_view_pos[2] );
	    SG_LOG( SG_TERRAIN, SG_DEBUG,
		    "    ref point = " << e->center );

	    sgdVec3 center;
	    sgdSetVec3( center, e->center.x(), e->center.y(), e->center.z() );
	    dist = sgdDistanceVec3( center, abs_view_pos );

	    SG_LOG( SG_TERRAIN, SG_DEBUG, "    distance = " << dist );

	    if ( dist > max_dist ) {
		max_dist = dist;
		max_index = index;
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
	    SG_LOG( SG_TERRAIN, SG_ALERT, "WHOOPS!!! Dying in next_avail()" );
	    exit( -1 );
	}
    }
}
