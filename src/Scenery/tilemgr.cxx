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

#include <plib/ssg.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/vector.hxx>

#include <Main/globals.hxx>
#include <Objects/obj.hxx>

#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "newcache.hxx"
#include "scenery.hxx"
#include "tilemgr.hxx"

#define TEST_LAST_HIT_CACHE

extern ssgRoot *scene;
extern ssgBranch *terrain;
extern ssgBranch *ground;

// the tile manager
FGTileMgr global_tile_mgr;


// a temporary hack until we get everything rewritten with sgdVec3
static inline Point3D operator + (const Point3D& a, const sgdVec3 b)
{
    return Point3D(a.x()+b[0], a.y()+b[1], a.z()+b[2]);
}

#ifdef ENABLE_THREADS
SGLockedQueue<FGTileEntry *> FGTileMgr::attach_queue;
SGLockedQueue<FGDeferredModel *> FGTileMgr::model_queue;
#else
queue<FGTileEntry *> FGTileMgr::attach_queue;
queue<FGDeferredModel *> FGTileMgr::model_queue;
#endif // ENABLE_THREADS


// Constructor
FGTileMgr::FGTileMgr():
    state( Start ),
    vis( 16000 ),
    counter_hack(0)
{
}


// Destructor
FGTileMgr::~FGTileMgr() {
}


// Initialize the Tile Manager subsystem
int FGTileMgr::init() {
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing Tile Manager subsystem." );

    tile_cache.init();
    destroy_queue();

    while ( ! attach_queue.empty() ) {
        attach_queue.pop();
    }

    while ( ! model_queue.empty() ) {
#ifdef ENABLE_THREADS
	FGDeferredModel* dm = model_queue.pop();
#else
        FGDeferredModel* dm = model_queue.front();
        model_queue.pop();
#endif
        delete dm;
    }
    loader.reinit();

#if 0        
    if ( state != Start ) {
	SG_LOG( SG_TERRAIN, SG_INFO,
		"... Reinitializing." );
	destroy_queue();
    } else {
	SG_LOG( SG_TERRAIN, SG_INFO,
		"... First time through." );
	tile_cache.init();
    }
#endif

    hit_list.clear();

    state = Inited;

    previous_bucket.make_bad();
    current_bucket.make_bad();

    longitude = latitude = -1000.0;
    last_longitude = last_latitude = -1000.0;
	
    return 1;
}


// schedule a tile for loading
void FGTileMgr::sched_tile( const SGBucket& b ) {
    // see if tile already exists in the cache
    FGTileEntry *t = tile_cache.get_tile( b );

    if ( t == NULL ) {
        // create a new entry
        FGTileEntry *e = new FGTileEntry( b );

        // insert the tile into the cache
	tile_cache.insert_tile( e );

        // Schedule tile for loading
        loader.add( e );
    }
}


static void CurrentNormalInLocalPlane(sgVec3 dst, sgVec3 src) {
    sgVec3 tmp;
    sgSetVec3(tmp, src[0], src[1], src[2] );
    sgMat4 TMP;
    sgTransposeNegateMat4 ( TMP, globals->get_current_view()->get_UP() ) ;
    sgXformVec3(tmp, tmp, TMP);
    sgSetVec3(dst, tmp[2], tmp[1], tmp[0] );
}


// Determine scenery altitude via ssg.  Normally this just happens
// when we render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  view_pos in current world
// coordinate translated near (0,0,0) (in meters.)  Returns result in
// meters.
bool FGTileMgr::current_elev_ssg( sgdVec3 abs_view_pos, double *terrain_elev ) {
    sgdVec3 view_pos;
    sgdVec3 sc;
    sgdSetVec3( sc, scenery.center.x(), scenery.center.y(), scenery.center.z());
    sgdSubVec3( view_pos, abs_view_pos, sc );

    sgdVec3 orig, dir;
    sgdCopyVec3(orig, view_pos );
    sgdCopyVec3(dir, abs_view_pos );

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
	*terrain_elev = result;
	scenery.cur_radius = geoc.radius();
	sgVec3 tmp;
	sgSetVec3(tmp, hit_list.get_normal(this_hit));
	// cout << "cur_normal: " << tmp[0] << " " << tmp[1] << " "
	//      << tmp[2] << endl;
	ssgState *IntersectedLeafState =
	    ((ssgLeaf*)hit_list.get_entity(this_hit))->getState();
	CurrentNormalInLocalPlane(tmp, tmp);
	sgdSetVec3( scenery.cur_normal, tmp );
	// cout << "NED: " << tmp[0] << " " << tmp[1] << " " << tmp[2] << endl;
	return true;
    } else {
	SG_LOG( SG_TERRAIN, SG_INFO, "no terrain intersection" );
	*terrain_elev = 0.0;
	float *up = globals->get_current_view()->get_world_up();
	sgdSetVec3(scenery.cur_normal, up[0], up[1], up[2]);
	return false;
    }
}


// schedule a needed buckets for loading
void FGTileMgr::schedule_needed() {
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

    double tile_width = current_bucket.get_width_m();
    double tile_height = current_bucket.get_height_m();
    // cout << "tile width = " << tile_width << "  tile_height = "
    //      << tile_height !<< endl;

    xrange = (int)(vis / tile_width) + 1;
    yrange = (int)(vis / tile_height) + 1;
    if ( xrange < 1 ) { xrange = 1; }
    if ( yrange < 1 ) { yrange = 1; }
    // cout << "xrange = " << xrange << "  yrange = " << yrange << endl;

    tile_cache.set_max_cache_size( (2*xrange + 2) * (2*yrange + 2) );

    SGBucket b;

    // schedule center tile first so it can be loaded first
    b = sgBucketOffset( longitude, latitude, 0, 0 );
    sched_tile( b );

    int x, y;

    // schedule next ring of 8 tiles
    for ( x = -1; x <= 1; ++x ) {
	for ( y = -1; y <= 1; ++y ) {
	    if ( x != 0 || y != 0 ) {
		b = sgBucketOffset( longitude, latitude, x, y );
		sched_tile( b );
	    }
	}
    }
    
    // schedule remaining tiles
    for ( x = -xrange; x <= xrange; ++x ) {
	for ( y = -yrange; y <= yrange; ++y ) {
	    if ( x < -1 || x > 1 || y < -1 || y > 1 ) {
		SGBucket b = sgBucketOffset( longitude, latitude, x, y );
		sched_tile( b );
	    }
	}
    }
}


void FGTileMgr::initialize_queue()
{
    // First time through or we have teleported, initialize the
    // system and load all relavant tiles

    SG_LOG( SG_TERRAIN, SG_INFO, "Updating Tile list for " << current_bucket );
    // cout << "tile cache size = " << tile_cache.get_size() << endl;

    // wipe/initialize tile cache
    // tile_cache.init();
    previous_bucket.make_bad();

    // build the local area list and schedule tiles for loading

    // start with the center tile and work out in concentric
    // "rings"

    schedule_needed();

    // do we really want to lose this? CLO
#if 0
    // Now force a load of the center tile and inner ring so we
    // have something to see in our first frame.
    int i;
    for ( i = 0; i < 9; ++i ) {
        if ( load_queue.size() ) {
            SG_LOG( SG_TERRAIN, SG_DEBUG, 
                    "Load queue not empty, loading a tile" );

            SGBucket pending = load_queue.front();
            load_queue.pop_front();
            load_tile( pending );
        }
    }
#endif
}


// forced emptying of the queue
// This is necessay to keep bookeeping straight for the
// tile_cache   -- which actually handles all the
// (de)allocations  
void FGTileMgr::destroy_queue() {
    // load_queue.clear();
}


// given the current lon/lat (in degrees), fill in the array of local
// chunks.  If the chunk isn't already in the cache, then read it from
// disk.
int FGTileMgr::update( double lon, double lat ) {
    SG_LOG( SG_TERRAIN, SG_DEBUG, "FGTileMgr::update()" );

    // FGInterface *f = current_aircraft.fdm_state;

    // lonlat for this update 
    // longitude = f->get_Longitude() * SGD_RADIANS_TO_DEGREES;
    // latitude = f->get_Latitude() * SGD_RADIANS_TO_DEGREES;
    longitude = lon;
    latitude = lat;
    // SG_LOG( SG_TERRAIN, SG_DEBUG, "lon "<< lonlat[LON] <<
    //      " lat " << lonlat[LAT] );

    current_bucket.set_bucket( longitude, latitude );
    // SG_LOG( SG_TERRAIN, SG_DEBUG, "Updating Tile list for " << current_bucket );

    if ( tile_cache.exists( current_bucket ) ) {
        current_tile = tile_cache.get_tile( current_bucket );
        scenery.next_center = current_tile->center;
    } else {
        SG_LOG( SG_TERRAIN, SG_WARN, "Tile not found (Ok if initializing)" );
    }

    if ( state == Running ) {
	if ( !(current_bucket == previous_bucket) ) {
	    // We've moved to a new bucket, we need to schedule any
	    // needed tiles for loading.
	    schedule_needed();
	}
    } else if ( state == Start || state == Inited ) {
	initialize_queue();
	state = Running;
    }

    // load the next tile in the load queue (or authorize the next
    // load in the case of the threaded tile pager)
    loader.update();

    // load the next model in the load queue.  Currently this must
    // happen in the render thread because model loading can trigger
    // texture loading which involves use of the opengl api.
    if ( !model_queue.empty() ) {
        cout << "loading next model ..." << endl;
        // load the next tile in the queue
#ifdef ENABLE_THREADS
	FGDeferredModel* dm = model_queue.pop();
#else
        FGDeferredModel* dm = model_queue.front();
        model_queue.pop();
#endif
	
	ssgTexturePath( (char *)(dm->get_texture_path().c_str()) );
	ssgEntity *obj_model
	    = ssgLoad( (char *)(dm->get_model_path().c_str()) );
	dm->get_obj_trans()->addKid( obj_model );
	dm->get_tile()->dec_pending_models();

	delete dm;
    }

    if ( scenery.center == Point3D(0.0) ) {
	// initializing
	cout << "initializing scenery current elevation  ... " << endl;
	sgdVec3 tmp_abs_view_pos;
	sgVec3 tmp_view_pos;

	Point3D geod_pos = Point3D( longitude * SGD_DEGREES_TO_RADIANS,
				    latitude * SGD_DEGREES_TO_RADIANS,
				    0.0);
	Point3D tmp = sgGeodToCart( geod_pos );
	scenery.center = tmp;
	sgdSetVec3( tmp_abs_view_pos, tmp.x(), tmp.y(), tmp.z() );

	// cout << "abs_view_pos = " << tmp_abs_view_pos << endl;
	prep_ssg_nodes();
	sgSetVec3( tmp_view_pos, 0.0, 0.0, 0.0 );
	double tmp_elev;
	if ( current_elev_ssg(tmp_abs_view_pos, &tmp_elev) ) {
	    scenery.cur_elev = tmp_elev;
	} else {
	    scenery.cur_elev = 0.0;
	}
	cout << "result = " << scenery.cur_elev << endl;
    } else {
	// cout << "abs view pos = " << current_view.abs_view_pos
	//      << " view pos = " << current_view.view_pos << endl;
	double tmp_elev;
	if ( current_elev_ssg(globals->get_current_view()->get_abs_view_pos(),
			      &tmp_elev) )
	{
	    scenery.cur_elev = tmp_elev;
	} else {
	    scenery.cur_elev = 0.0;
	}  
    }

    // cout << "current elevation (ssg) == " << scenery.cur_elev << endl;

    previous_bucket = current_bucket;
    last_longitude = longitude;
    last_latitude  = latitude;

    // activate loader thread one out of every 5 frames
    counter_hack = (counter_hack + 1) % 5;
    if ( !counter_hack ) {
        // Notify the tile loader that it can load another tile
        // loader.update();

	if ( !attach_queue.empty() ) {
#ifdef ENABLE_THREADS
	    FGTileEntry* e = attach_queue.pop();
#else
	    FGTileEntry* e = attach_queue.front();
            attach_queue.pop();
#endif
	    e->add_ssg_nodes( terrain, ground );
	    // cout << "Adding ssg nodes for "
	}
    }

    return 1;
}


void FGTileMgr::prep_ssg_nodes() {
    float vis = 0.0;

#ifndef FG_OLD_WEATHER
    if ( WeatherDatabase ) {
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

    FGTileEntry *e;
    tile_cache.reset_traversal();

    while ( ! tile_cache.at_end() ) {
        // cout << "processing a tile" << endl;
        if ( (e = tile_cache.get_current()) ) {
	    e->prep_ssg_node( scenery.center, vis);
        } else {
            cout << "warning ... empty tile in cache" << endl;
        }
	tile_cache.next();
   }
}
