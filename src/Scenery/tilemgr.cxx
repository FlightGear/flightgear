// tilemgr.cxx -- routines to handle dynamic management of scenery tiles
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
#include <simgear/xgl/xgl.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/vector.hxx>

// #include <Aircraft/aircraft.hxx>
#include <Main/globals.hxx>
#include <Main/options.hxx>
#include <Objects/obj.hxx>

#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "scenery.hxx"
#include "tilecache.hxx"
#include "tilemgr.hxx"

#define TEST_LAST_HIT_CACHE

extern ssgRoot *scene;
extern ssgBranch *terrain;

// the tile manager
FGTileMgr global_tile_mgr;


// a temporary hack until we get everything rewritten with sgdVec3
static inline Point3D operator + (const Point3D& a, const sgdVec3 b)
{
    return Point3D(a.x()+b[0], a.y()+b[1], a.z()+b[2]);
}


// Constructor
FGTileMgr::FGTileMgr ( void ):
    state( Start )
{
}


// Destructor
FGTileMgr::~FGTileMgr ( void ) {
}


// Initialize the Tile Manager subsystem
int FGTileMgr::init( void ) {
    FG_LOG( FG_TERRAIN, FG_INFO, "Initializing Tile Manager subsystem." );

    if ( state != Start ) {
	FG_LOG( FG_TERRAIN, FG_INFO,
		"... Reinitializing." );
	destroy_queue();
    } else {
	FG_LOG( FG_TERRAIN, FG_INFO,
		"... First time through." );
    }

    global_tile_cache.init();
    hit_list.clear();

    state = Inited;

    tile_diameter = current_options.get_tile_diameter();
    FG_LOG( FG_TERRAIN, FG_INFO, "Tile Diameter = " << tile_diameter);
    
    previous_bucket.make_bad();
    current_bucket.make_bad();

    scroll_direction = SCROLL_INIT;
    tile_index = -9999;

    longitude = latitude = -1000.0;
    last_longitude = last_latitude = -1000.0;
	
    return 1;
}


// schedule a tile for loading
int FGTileMgr::sched_tile( const FGBucket& b ) {
    // see if tile already exists in the cache
    int cache_index = global_tile_cache.exists( b );

    if ( cache_index >= 0 ) {
        // tile exists in cache, reenable it.
        // cout << "REENABLING DISABLED TILE" << endl;
        FGTileEntry *t = global_tile_cache.get_tile( cache_index );
        t->select_ptr->select( 1 );
        t->mark_loaded();
    } else {
        // find the next available cache entry and mark it as
        // scheduled
        cache_index = global_tile_cache.next_avail();
        FGTileEntry *t = global_tile_cache.get_tile( cache_index );
        t->mark_scheduled_for_use();

        // register a load request
        FGLoadRec request;
        request.b = b;
        request.cache_index = cache_index;
        load_queue.push_back( request );
    }

    return cache_index;
}


// load a tile
void FGTileMgr::load_tile( const FGBucket& b, int cache_index) {

    FG_LOG( FG_TERRAIN, FG_DEBUG, "Loading tile " << b );
    global_tile_cache.fill_in(cache_index, b);
    FG_LOG( FG_TERRAIN, FG_DEBUG, "Loaded for cache index: " << cache_index );
}


// Determine scenery altitude via ssg.  Normally this just happens
// when we render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  view_pos in current world
// coordinate translated near (0,0,0) (in meters.)  Returns result in
// meters.

bool
FGTileMgr::current_elev_ssg( const Point3D& abs_view_pos, 
			     const Point3D& view_pos )
{
    sgdVec3 orig, dir;

    sgdSetVec3(orig, view_pos.x(), view_pos.y(), view_pos.z() );
    sgdSetVec3(dir, abs_view_pos.x(), abs_view_pos.y(), abs_view_pos.z() );

    hit_list.Intersect( terrain, orig, dir );

    int this_hit=0;
    Point3D geoc;
    double result = -9999;

    int hitcount = hit_list.num_hits();
    for ( int i = 0; i < hitcount; ++i ) {
	geoc = sgCartToPolar3d( scenery.center + hit_list.get_point(i) );      
	double lat_geod, alt, sea_level_r;
	sgGeocToGeod(geoc.lat(), geoc.radius(), &lat_geod, 
		     &alt, &sea_level_r);
	if ( alt > result && alt < 10000 ) {
	    result = alt;
	    this_hit = i;
	}
    }

    if ( result > -9000 ) {
	scenery.cur_elev = result;
	scenery.cur_radius = geoc.radius();
	sgVec3 tmp;
	sgSetVec3(tmp, hit_list.get_normal(this_hit));
	ssgState *IntersectedLeafState =
	    ((ssgLeaf*)hit_list.get_entity(this_hit))->getState();
	globals->get_current_view()->CurrentNormalInLocalPlane(tmp, tmp);
	sgdSetVec3( scenery.cur_normal, tmp );
	// cout << "NED: " << tmp[0] << " " << tmp[1] << " " << tmp[2] << endl;
	return true;
    } else {
	FG_LOG( FG_TERRAIN, FG_INFO, "no terrain intersection" );
	scenery.cur_elev = 0.0;
	float *up = globals->get_current_view()->get_local_up();
	sgdSetVec3(scenery.cur_normal, up[0], up[1], up[2]);
	return false;
    }
}


FGBucket FGTileMgr::BucketOffset( int dx, int dy )
{
    double clat, clon, span;
    if( scroll_direction == SCROLL_INIT ) {
	// use current latitude and longitude
	// walk dy units in the lat direction
	clat = current_bucket.get_center_lat() + dy * FG_BUCKET_SPAN;

	// find the lon span for the new latitude
	span = bucket_span( clat );
	
	// walk dx units in the lon direction
	clon = longitude + dx * span;
    } else	{
	// use previous latitude and longitude
	// walk dy units in the lat direction
	clat = previous_bucket.get_center_lat() + dy * FG_BUCKET_SPAN;

	// find the lon span for the new latitude
	span = bucket_span( clat );

	// walk dx units in the lon direction
	clon = last_longitude + dx * span;
    }    
	
    while ( clon < -180.0 ) clon += 360.0;
    while ( clon >= 180.0 ) clon -= 360.0;
    pending.set_bucket( clon, clat );

    FG_LOG( FG_TERRAIN, FG_DEBUG, "    fgBucketOffset " << pending );
    return pending;
}


// schedule a tile row(column) for loading
void FGTileMgr::scroll( void )
{
    FG_LOG( FG_TERRAIN, FG_DEBUG, "schedule_row: Scrolling" );

    int i, dw, dh;
	
    switch( scroll_direction ) {
    case SCROLL_NORTH:
	FG_LOG( FG_TERRAIN, FG_DEBUG, 
		"  (North) Loading " << tile_diameter << " tiles" );
	dw = tile_diameter / 2;
	dh = dw + 1;
	for ( i = 0; i < tile_diameter; i++ ) {
	    sched_tile( BucketOffset( i - dw, dh ) );
	}
	break;
    case SCROLL_EAST:
	FG_LOG( FG_TERRAIN, FG_DEBUG, 
		"  (East) Loading " << tile_diameter << " tiles" );
	dh = tile_diameter / 2;
	dw = dh + 1;
	for ( i = 0; i < tile_diameter; i++ ) {
	    sched_tile( BucketOffset( dw, i - dh ) );
	}
	break;
    case SCROLL_SOUTH:
	FG_LOG( FG_TERRAIN, FG_DEBUG, 
		"  (South) Loading " << tile_diameter << " tiles" );
	dw = tile_diameter / 2;
	dh = -dw - 1;
	for ( i = 0; i < tile_diameter; i++ ) {
	    sched_tile( BucketOffset( i - dw, dh ) );
	}
	break;
    case SCROLL_WEST:
	FG_LOG( FG_TERRAIN, FG_DEBUG, 
		"  (West) Loading " << tile_diameter << " tiles" );
	dh = tile_diameter / 2;
	dw = -dh - 1;
	for ( i = 0; i < tile_diameter; i++ ) {
	    sched_tile( BucketOffset( dw, i - dh ) );
	}
	break;
    default:
	FG_LOG( FG_TERRAIN, FG_WARN, "UNKNOWN SCROLL DIRECTION in schedule_row" );
	;
    }
    FG_LOG( FG_TERRAIN, FG_DEBUG, "\tschedule_row returns" );
}


void FGTileMgr::initialize_queue()
{
    // First time through or we have teleported, initialize the
    // system and load all relavant tiles

    FG_LOG( FG_TERRAIN, FG_INFO, "Updating Tile list for " << current_bucket );
    FG_LOG( FG_TERRAIN, FG_INFO, "  Updating Tile list for " << current_bucket );
    FG_LOG( FG_TERRAIN, FG_INFO, "  Loading " 
            << tile_diameter * tile_diameter << " tiles" );

    int i;
    scroll_direction = SCROLL_INIT;

    // wipe/initialize tile cache
    global_tile_cache.init();
    previous_bucket.make_bad();

    // build the local area list and schedule tiles for loading

    // start with the center tile and work out in concentric
    // "rings"

    sched_tile( current_bucket );

    for ( i = 3; i <= tile_diameter; i = i + 2 ) {
        int j;
        int span = i / 2;

        // bottom row
        for ( j = -span; j <= span; ++j ) {
            sched_tile( BucketOffset( j, -span ) );
        }

        // top row
        for ( j = -span; j <= span; ++j ) {
            sched_tile( BucketOffset( j, span ) );
        }

        // middle rows
        for ( j = -span + 1; j <= span - 1; ++j ) {
            sched_tile( BucketOffset( -span, j ) );
            sched_tile( BucketOffset( span, j ) );
        }

    }

    // Now force a load of the center tile and inner ring so we
    // have something to see in our first frame.
    for ( i = 0; i < 9; ++i ) {
        if ( load_queue.size() ) {
            FG_LOG( FG_TERRAIN, FG_DEBUG, 
                    "Load queue not empty, loading a tile" );

            FGLoadRec pending = load_queue.front();
            load_queue.pop_front();
            load_tile( pending.b, pending.cache_index );
        }
    }
}


// forced emptying of the queue
// This is necessay to keep bookeeping straight for the
// tile_cache   -- which actually handles all the
// (de)allocations  
void FGTileMgr::destroy_queue() {
    while( load_queue.size() ) {
	FG_LOG( FG_TERRAIN, FG_INFO, 
		"Load queue not empty, popping a tile" );
	FGLoadRec pending = load_queue.front();
	load_queue.pop_front();
        FGTileEntry *t = global_tile_cache.get_tile( pending.cache_index );
	// just t->mark_unused() should be enough
	// but a little paranoia doesn't hurt us here
	if(t->is_scheduled_for_use())
	    t->mark_unused();
	else
	    load_tile( pending.b, pending.cache_index );
    }
}


// given the current lon/lat (in degrees), fill in the array of local
// chunks.  If the chunk isn't already in the cache, then read it from
// disk.
int FGTileMgr::update( double lon, double lat ) {
    // FG_LOG( FG_TERRAIN, FG_DEBUG, "FGTileMgr::update()" );

    // FGInterface *f = current_aircraft.fdm_state;

    // lonlat for this update 
    // longitude = f->get_Longitude() * RAD_TO_DEG;
    // latitude = f->get_Latitude() * RAD_TO_DEG;
    longitude = lon;
    latitude = lat;
    // FG_LOG( FG_TERRAIN, FG_DEBUG, "lon "<< lonlat[LON] <<
    //      " lat " << lonlat[LAT] );

    current_bucket.set_bucket( longitude, latitude );
    // FG_LOG( FG_TERRAIN, FG_DEBUG, "Updating Tile list for " << current_bucket );

    tile_index = global_tile_cache.exists(current_bucket);
    // FG_LOG( FG_TERRAIN, FG_DEBUG, "tile index " << tile_index );

    if ( tile_index >= 0 ) {
        current_tile = global_tile_cache.get_tile(tile_index);
        scenery.next_center = current_tile->center;
    } else {
        FG_LOG( FG_TERRAIN, FG_WARN, "Tile not found (Ok if initializing)" );
    }

    if ( state == Running ) {
	if( current_bucket == previous_bucket) {
	    FG_LOG( FG_TERRAIN, FG_DEBUG, "Same bucket as last time" );
	    scroll_direction = SCROLL_NONE;
	} else {
	    // We've moved to a new bucket, we need to scroll our
	    // structures, and load in the new tiles
	    // CURRENTLY THIS ASSUMES WE CAN ONLY MOVE TO ADJACENT TILES.
	    // AT ULTRA HIGH SPEEDS THIS ASSUMPTION MAY NOT BE VALID IF
	    // THE AIRCRAFT CAN SKIP A TILE IN A SINGLE ITERATION.

	    if ( (current_bucket.get_lon() > previous_bucket.get_lon()) ||
		 ( (current_bucket.get_lon() == previous_bucket.get_lon()) && 
		   (current_bucket.get_x() > previous_bucket.get_x()) ) )
		{
		    scroll_direction = SCROLL_EAST;
		}
	    else if ( (current_bucket.get_lon() < previous_bucket.get_lon()) ||
		      ( (current_bucket.get_lon() == previous_bucket.get_lon()) && 
			(current_bucket.get_x() < previous_bucket.get_x()) ) )
		{   
		    scroll_direction = SCROLL_WEST;
		}   

	    if ( (current_bucket.get_lat() > previous_bucket.get_lat()) ||
		 ( (current_bucket.get_lat() == previous_bucket.get_lat()) && 
		   (current_bucket.get_y() > previous_bucket.get_y()) ) )
		{   
		    scroll_direction = SCROLL_NORTH;
		}
	    else if ( (current_bucket.get_lat() < previous_bucket.get_lat()) ||
		      ( (current_bucket.get_lat() == previous_bucket.get_lat()) && 
			(current_bucket.get_y() < previous_bucket.get_y()) ) )
		{
		    scroll_direction = SCROLL_SOUTH;
		}

	    scroll();
	}

    } else if ( state == Start || state == Inited ) {
	initialize_queue();
	state = Running;
    }

    if ( load_queue.size() ) {
	FG_LOG( FG_TERRAIN, FG_DEBUG, "Load queue not empty, loading a tile" );

	FGLoadRec pending = load_queue.front();
	load_queue.pop_front();
	load_tile( pending.b, pending.cache_index );
    }

    if ( scenery.center == Point3D(0.0) ) {
	// initializing
	// cout << "initializing ... " << endl;
	Point3D geod_pos = Point3D( longitude * DEG_TO_RAD,
				    latitude * DEG_TO_RAD,
				    0.0);
	Point3D tmp_abs_view_pos = sgGeodToCart( geod_pos );
	scenery.center = tmp_abs_view_pos;
	// cout << "abs_view_pos = " << tmp_abs_view_pos << endl;
	prep_ssg_nodes();
	current_elev_ssg( tmp_abs_view_pos,
			  Point3D( 0.0 ) );
    } else {
	// cout << "abs view pos = " << current_view.abs_view_pos
	//      << " view pos = " << current_view.view_pos << endl;
	current_elev_ssg( globals->get_current_view()->get_abs_view_pos(),
			  globals->get_current_view()->get_view_pos() );
    }

    // cout << "current elevation (ssg) == " << scenery.cur_elev << endl;

    previous_bucket = current_bucket;
    last_longitude = longitude;
    last_latitude  = latitude;

    return 1;
}

// Prepare the ssg nodes ... for each tile, set it's proper
// transform and update it's range selector based on current
// visibilty
void FGTileMgr::prep_ssg_node( int idx ) {
}

void FGTileMgr::prep_ssg_nodes( void ) {
    FGTileEntry *t;
    float ranges[2];
    ranges[0] = 0.0f;
    double vis = 0.0;

#ifndef FG_OLD_WEATHER
    if ( WeatherDatabase != NULL ) {
	vis = WeatherDatabase->getWeatherVisibility();
    } else {
	vis = 16000;
    }
#else
    vis = current_weather.get_visibility();
#endif
    // cout << "visibility = " << vis << endl;

    // traverse the potentially viewable tile list and update range
    // selector and transform
    for ( int i = 0; i < (int)global_tile_cache.get_size(); i++ ) {
        t = global_tile_cache.get_tile( i );

        if ( t->is_loaded() ) {
	    // set range selector (LOD trick) to be distance to center
	    // of tile + bounding radius

            ranges[1] = vis + t->bounding_radius;
            t->range_ptr->setRanges( ranges, 2 );

            // calculate tile offset
            t->SetOffset( scenery.center );

            // calculate ssg transform
            sgCoord sgcoord;
            sgSetCoord( &sgcoord,
                        t->offset.x(), t->offset.y(), t->offset.z(),
                        0.0, 0.0, 0.0 );
            t->transform_ptr->setTransform( &sgcoord );
        }
    }
}
