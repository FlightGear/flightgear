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
#include <simgear/misc/exception.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewer.hxx>
#include <Model/loader.hxx>
#include <Objects/obj.hxx>

#include "newcache.hxx"
#include "scenery.hxx"
#include "tilemgr.hxx"

#define TEST_LAST_HIT_CACHE

// the tile manager
FGTileMgr global_tile_mgr;


#ifdef ENABLE_THREADS
SGLockedQueue<FGTileEntry *> FGTileMgr::attach_queue;
SGLockedQueue<FGDeferredModel *> FGTileMgr::model_queue;
#else
queue<FGTileEntry *> FGTileMgr::attach_queue;
queue<FGDeferredModel *> FGTileMgr::model_queue;
#endif // ENABLE_THREADS
queue<FGTileEntry *> FGTileMgr::delete_queue;


// Constructor
FGTileMgr::FGTileMgr():
    state( Start ),
    current_tile( NULL ),
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

#if 0

    // instead it's just a lot easier to let any pending work flush
    // through, rather than trying to arrest the queue and nuke all
    // the various work at all the various stages and get everything
    // cleaned up properly.

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
        // make space in the cache
        while ( (int)tile_cache.get_size() > tile_cache.get_max_cache_size() ) {
            long index = tile_cache.get_oldest_tile();
            if ( index >= 0 ) {
                FGTileEntry *old = tile_cache.get_tile( index );
                old->disconnect_ssg_nodes();
                delete_queue.push( old );
                tile_cache.clear_entry( index );
            } else {
                // nothing to free ?!? forge ahead
                break;
            }
        }

        // create a new entry
        FGTileEntry *e = new FGTileEntry( b );

        // insert the tile into the cache
	if ( tile_cache.insert_tile( e ) ) {
	  // Schedule tile for loading
	  loader.add( e );
	} else {
	  // insert failed (cache full with no available entries to
	  // delete.)  Try again later
	  delete e;
	}
    }
}


// schedule a needed buckets for loading
void FGTileMgr::schedule_needed( double vis, SGBucket curr_bucket) {
    // sanity check (unfortunately needed!)
    if ( longitude < -180.0 || longitude > 180.0 
         || latitude < -90.0 || latitude > 90.0 )
    {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Attempting to schedule tiles for bogus latitude and" );
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "longitude.  This is a FATAL error.  Exiting!" );
        exit(-1);        
    }

    SG_LOG( SG_TERRAIN, SG_INFO,
            "scheduling needed tiles for " << longitude << " " << latitude );

    // vis = fgGetDouble("/environment/visibility-m");

    double tile_width = curr_bucket.get_width_m();
    double tile_height = curr_bucket.get_height_m();
    // cout << "tile width = " << tile_width << "  tile_height = "
    //      << tile_height !<< endl;

    xrange = (int)(vis / tile_width) + 1;
    yrange = (int)(vis / tile_height) + 1;
    if ( xrange < 1 ) { xrange /= 1; }
    if ( yrange < 1 ) { yrange = 1; }

    // note * 2 at end doubles cache size (for fdm and viewer)
    tile_cache.set_max_cache_size( (2*xrange + 2) * (2*yrange + 2) * 2 );

    /*
    cout << "xrange = " << xrange << "  yrange = " << yrange << endl;
    cout << "max cache size = " << tile_cache.get_max_cache_size()
         << " current cache size = " << tile_cache.get_size() << endl;
    */

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

    double visibility_meters = fgGetDouble("/environment/visibility-m");
    schedule_needed(visibility_meters, current_bucket);

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


// given the current lon/lat (in degrees), fill in the array of local
// chunks.  If the chunk isn't already in the cache, then read it from
// disk.
int FGTileMgr::update( double lon, double lat, double visibility_meters ) {
	sgdVec3 abs_pos_vector;
        sgdCopyVec3(abs_pos_vector , globals->get_current_view()->get_absolute_view_pos());
        return update( lon, lat, visibility_meters, abs_pos_vector,
                       current_bucket, previous_bucket,
                       globals->get_scenery()->get_center() );
}


int FGTileMgr::update( double lon, double lat, double visibility_meters,
                       sgdVec3 abs_pos_vector, SGBucket p_current,
                       SGBucket p_previous, Point3D center ) {
    // SG_LOG( SG_TERRAIN, SG_DEBUG, "FGTileMgr::update() for "
    //         << lon << " " << lat );

    longitude = lon;
    latitude = lat;
    current_bucket = p_current;
    previous_bucket = p_previous;

    // SG_LOG( SG_TERRAIN, SG_DEBUG, "lon "<< lonlat[LON] <<
    //      " lat " << lonlat[LAT] );

    // SG_LOG( SG_TERRAIN, SG_DEBUG, "Updating Tile list for " << current_bucket );

    setCurrentTile( longitude, latitude);

    // do tile load scheduling. 
    // Note that we need keep track of both viewer buckets and fdm buckets.
    if ( state == Running ) {
       SG_LOG( SG_TERRAIN, SG_DEBUG, "State == Running" );
       if (!(current_bucket == previous_bucket )) {
	 // We've moved to a new bucket, we need to schedule any
	 // needed tiles for loading.
	 schedule_needed(visibility_meters, current_bucket);
       }
    } else if ( state == Start || state == Inited ) {
	SG_LOG( SG_TERRAIN, SG_INFO, "State == Start || Inited" );
	initialize_queue();
	state = Running;

	// load the next tile in the load queue (or authorize the next
	// load in the case of the threaded tile pager)
	loader.update();
    }

    
    // load the next model in the load queue.  Currently this must
    // happen in the render thread because model loading can trigger
    // texture loading which involves use of the opengl api.
    if ( !model_queue.empty() ) {
	// cout << "loading next model ..." << endl;
	// load the next tile in the queue
#ifdef ENABLE_THREADS
	FGDeferredModel* dm = model_queue.pop();
#else
        FGDeferredModel* dm = model_queue.front();
        model_queue.pop();
#endif
	
	ssgTexturePath( (char *)(dm->get_texture_path().c_str()) );
	try
	{
	    ssgEntity *obj_model =
		globals->get_model_loader()->load_model(dm->get_model_path());
	    if ( obj_model != NULL ) {
		dm->get_obj_trans()->addKid( obj_model );
	    }
	}
	catch (const sg_exception& exc)
	{
	    SG_LOG( SG_ALL, SG_ALERT, exc.getMessage() );
	}

	dm->get_tile()->dec_pending_models();
	delete dm;
    }
    
    // cout << "current elevation (ssg) == " << scenery.get_cur_elev() << endl;


    // save bucket...
    previous_bucket = current_bucket;

    // activate loader thread one out of every 5 frames
    if ( counter_hack == 0 ) {
      // Notify the tile loader that it can load another tile
      loader.update();
    } else {
      counter_hack = (counter_hack + 1) % 5;
    }

    if ( !attach_queue.empty() ) {
#ifdef ENABLE_THREADS
	FGTileEntry* e = attach_queue.pop();
#else
	FGTileEntry* e = attach_queue.front();
	attach_queue.pop();
#endif
	e->add_ssg_nodes( globals->get_scenery()->get_terrain_branch(),
			  globals->get_scenery()->get_gnd_lights_root(),
			  globals->get_scenery()->get_rwy_lights_root(),
			  globals->get_scenery()->get_taxi_lights_root() );
	// cout << "Adding ssg nodes for "
    }

    if ( !delete_queue.empty() ) {
        // cout << "delete queue = " << delete_queue.size() << endl;

        while ( delete_queue.size() > 30 ) {
            // uh oh, delete queue is blowing up, we aren't clearing
            // it fast enough.  Let's just panic, well not panic, but
            // get real serious and agressively free up some tiles so
            // we don't explode our memory usage.

            SG_LOG( SG_TERRAIN, SG_ALERT,
                    "Alert: catching up on tile delete queue" );

            FGTileEntry* e = delete_queue.front();
            while ( !e->free_tile() );
            delete_queue.pop();
            delete e;
        }

        FGTileEntry* e = delete_queue.front();
        if ( e->free_tile() ) {
            delete_queue.pop();
            delete e;
        }
    }

    // no reason to update this if we haven't moved...
    if ( longitude != last_longitude || latitude != last_latitude ) {
        // update current elevation... 
        if ( updateCurrentElevAtPos( abs_pos_vector, center )
             )
            /*if ( updateCurrentElevAtPos( abs_pos_vector,
                                     globals->get_scenery()->get_next_center() )
                                     )*/
        {
            last_longitude = longitude;
            last_latitude = latitude;
        }
    }

    return 1;
}

// timer event driven call to scheduler for the purpose of refreshing the tile timestamps
void FGTileMgr::refresh_view_timestamps() {
    SG_LOG( SG_TERRAIN, SG_INFO,
        "Refreshing timestamps for " << current_bucket.get_center_lon()
        << " " << current_bucket.get_center_lat() );
    schedule_needed(fgGetDouble("/environment/visibility-m"), current_bucket);
}


// check and set current tile and scenery center...
void FGTileMgr::setCurrentTile(double longitude, double latitude) {

    // check tile cache entry...
    current_bucket.set_bucket( longitude, latitude );
    if ( tile_cache.exists( current_bucket ) ) {
        current_tile = tile_cache.get_tile( current_bucket );
        globals->get_scenery()->set_next_center( current_tile->center );
    } else {
        SG_LOG( SG_TERRAIN, SG_WARN, "Tile not found (Ok if initializing)" );
        globals->get_scenery()->set_next_center( Point3D(0.0) );
    }
}


int FGTileMgr::updateCurrentElevAtPos(sgdVec3 abs_pos_vector, Point3D center) {

  sgdVec3 sc;

  sgdSetVec3( sc,
    center[0],
    center[1],
    center[2]);

    // overridden with actual values if a terrain intersection is
    // found
    double hit_elev = -9999.0;
    double hit_radius = 0.0;
    sgdVec3 hit_normal = { 0.0, 0.0, 0.0 };
    
    bool hit = false;
    if ( fabs(sc[0]) > 1.0 || fabs(sc[1]) > 1.0 || fabs(sc[2]) > 1.0 ) {
        // scenery center has been properly defined so any hit should
        // be valid (and not just luck)
        hit = fgCurrentElev(abs_pos_vector,
                            sc,
                            // current_tile->get_terra_transform(),
                            &hit_list,
                            &hit_elev,
                            &hit_radius,
                            hit_normal);
    }

    if ( hit ) {
        // cout << "elev = " << hit_elev << " " << hit_radius << endl;
        globals->get_scenery()->set_cur_elev( hit_elev );
        globals->get_scenery()->set_cur_radius( hit_radius );
        globals->get_scenery()->set_cur_normal( hit_normal );
    } else {
        globals->get_scenery()->set_cur_elev( -9999.0 );
        globals->get_scenery()->set_cur_radius( 0.0 );
        globals->get_scenery()->set_cur_normal( hit_normal );
    }
    return hit;
}


void FGTileMgr::prep_ssg_nodes(float vis) {

    // traverse the potentially viewable tile list and update range
    // selector and transform

    // just setup and call new function...

    sgVec3 up;
    sgCopyVec3( up, globals->get_current_view()->get_world_up() );

    Point3D center;
    center = globals->get_scenery()->get_center();
    prep_ssg_nodes( vis, up, center );

}


void FGTileMgr::prep_ssg_nodes(float vis, sgVec3 up, Point3D center) {

    // traverse the potentially viewable tile list and update range
    // selector and transform

    FGTileEntry *e;
    tile_cache.reset_traversal();

    while ( ! tile_cache.at_end() ) {
        // cout << "processing a tile" << endl;
	if ( (e = tile_cache.get_current()) ) {
	    e->prep_ssg_node( center, up, vis);
        } else {
	    SG_LOG(SG_INPUT, SG_ALERT, "warning ... empty tile in cache");
        }
	tile_cache.next();
   }
}

