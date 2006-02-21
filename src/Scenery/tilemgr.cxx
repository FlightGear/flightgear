// tilemgr.cxx -- routines to handle dynamic management of scenery tiles
//
// Written by Curtis Olson, started January 1998.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/ssg.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/vector.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/model/shadowvolume.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewer.hxx>

#include "newcache.hxx"
#include "scenery.hxx"
#include "tilemgr.hxx"

#define TEST_LAST_HIT_CACHE

#if defined(ENABLE_THREADS)
SGLockedQueue<FGTileEntry *> FGTileMgr::attach_queue;
SGLockedQueue<FGDeferredModel *> FGTileMgr::model_queue;
#else
queue<FGTileEntry *> FGTileMgr::attach_queue;
queue<FGDeferredModel *> FGTileMgr::model_queue;
#endif // ENABLE_THREADS
queue<FGTileEntry *> FGTileMgr::delete_queue;

bool FGTileMgr::tile_filter = true;

extern SGShadowVolume *shadows;

// Constructor
FGTileMgr::FGTileMgr():
    state( Start ),
    current_tile( NULL ),
    vis( 16000 )
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
#if defined(ENABLE_THREADS)
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

    return 1;
}


// schedule a tile for loading
void FGTileMgr::sched_tile( const SGBucket& b, const bool is_inner_ring ) {
    // see if tile already exists in the cache
    FGTileEntry *t = tile_cache.get_tile( b );

    if ( t == NULL ) {
        // make space in the cache
        while ( (int)tile_cache.get_size() > tile_cache.get_max_cache_size() ) {
            long index = tile_cache.get_oldest_tile();
            if ( index >= 0 ) {
                FGTileEntry *old = tile_cache.get_tile( index );
                shadows->deleteOccluderFromTile( (ssgBranch *) old->get_terra_transform() );
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
    } else {
        t->set_inner_ring( is_inner_ring );
    }
}


// schedule a needed buckets for loading
void FGTileMgr::schedule_needed( double vis, const SGBucket& curr_bucket) {
    // sanity check (unfortunately needed!)
    if ( longitude < -180.0 || longitude > 180.0 
         || latitude < -90.0 || latitude > 90.0 )
    {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Attempting to schedule tiles for bogus lon and lat  = ("
                << longitude << "," << latitude << ")" );
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "This is a FATAL error.  Exiting!" );
        exit(-1);        
    }

    SG_LOG( SG_TERRAIN, SG_INFO,
            "scheduling needed tiles for " << longitude << " " << latitude );

    // vis = fgGetDouble("/environment/visibility-m");

    double tile_width = curr_bucket.get_width_m();
    double tile_height = curr_bucket.get_height_m();
    // cout << "tile width = " << tile_width << "  tile_height = "
    //      << tile_height << endl;

    xrange = (int)(vis / tile_width) + 1;
    yrange = (int)(vis / tile_height) + 1;
    if ( xrange < 1 ) { xrange = 1; }
    if ( yrange < 1 ) { yrange = 1; }

    // note * 2 at end doubles cache size (for fdm and viewer)
    tile_cache.set_max_cache_size( (2*xrange + 2) * (2*yrange + 2) * 2 );
    // cout << "xrange = " << xrange << "  yrange = " << yrange << endl;
    // cout << "max cache size = " << tile_cache.get_max_cache_size()
    //      << " current cache size = " << tile_cache.get_size() << endl;

    // clear the inner ring flags so we can set them below.  This
    // prevents us from having "true" entries we aren't able to find
    // to get rid of if we teleport a long ways away from the current
    // location.
    tile_cache.clear_inner_ring_flags();

    SGBucket b;

    // schedule center tile first so it can be loaded first
    b = sgBucketOffset( longitude, latitude, 0, 0 );
    sched_tile( b, true );

    int x, y;

    // schedule next ring of 8 tiles
    for ( x = -1; x <= 1; ++x ) {
        for ( y = -1; y <= 1; ++y ) {
            if ( x != 0 || y != 0 ) {
                b = sgBucketOffset( longitude, latitude, x, y );
                sched_tile( b, true );
            }
        }
    }

    // schedule remaining tiles
    for ( x = -xrange; x <= xrange; ++x ) {
        for ( y = -yrange; y <= yrange; ++y ) {
            if ( x < -1 || x > 1 || y < -1 || y > 1 ) {
                SGBucket b = sgBucketOffset( longitude, latitude, x, y );
                sched_tile( b, false );
            }
        }
    }
}


void FGTileMgr::initialize_queue()
{
    // First time through or we have teleported, initialize the
    // system and load all relavant tiles

    SG_LOG( SG_TERRAIN, SG_INFO, "Initialize_queue(): Updating Tile list for "
            << current_bucket );
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

/**
 * return current status of queues
 *
 */

bool FGTileMgr::all_queues_empty() {
	return attach_queue.empty() && model_queue.empty();
}


/**
 * Update the various queues maintained by the tilemagr (private
 * internal function, do not call directly.)
 */
void FGTileMgr::update_queues()
{
    // load the next model in the load queue.  Currently this must
    // happen in the render thread because model loading can trigger
    // texture loading which involves use of the opengl api.  Skip any
    // models belonging to not loaded tiles (i.e. the tile was removed
    // before we were able to load some of the associated models.)
    if ( !model_queue.empty() ) {
        bool processed_one = false;

        while ( model_queue.size() > 200 || processed_one == false ) {
            processed_one = true;

            if ( model_queue.size() > 200 ) {
                SG_LOG( SG_TERRAIN, SG_INFO,
                        "Alert: catching up on model load queue" );
            }

            // cout << "loading next model ..." << endl;
            // load the next tile in the queue
#if defined(ENABLE_THREADS)
            FGDeferredModel* dm = model_queue.pop();
#else
            FGDeferredModel* dm = model_queue.front();
            model_queue.pop();
#endif

            // only load the model if the tile still exists in the
            // tile cache
            FGTileEntry *t = tile_cache.get_tile( dm->get_bucket() );
            if ( t != NULL ) {
                ssgTexturePath( (char *)(dm->get_texture_path().c_str()) );
                try {
                    ssgEntity *obj_model =
                        globals->get_model_lib()->load_model( ".",
                                                  dm->get_model_path(),
                                                  globals->get_props(),
                                                  globals->get_sim_time_sec() );
                    if ( obj_model != NULL ) {
                        dm->get_obj_trans()->addKid( obj_model );
                        shadows->addOccluder( (ssgBranch *) obj_model->getParent(0),
                            SGShadowVolume::occluderTypeTileObject,
                            (ssgBranch *) dm->get_tile()->get_terra_transform());
                    }
                } catch (const sg_io_exception& exc) {
					string m(exc.getMessage());
					m += " ";
					m += exc.getLocation().asString();
                    SG_LOG( SG_ALL, SG_ALERT, m );
                } catch (const sg_exception& exc) { // XXX may be redundant
                    SG_LOG( SG_ALL, SG_ALERT, exc.getMessage());
                }
                
                dm->get_tile()->dec_pending_models();
            }
            delete dm;
        }
    }
    
    // Notify the tile loader that it can load another tile
    loader.update();

    if ( !attach_queue.empty() ) {
#if defined(ENABLE_THREADS)
        FGTileEntry* e = attach_queue.pop();
#else
        FGTileEntry* e = attach_queue.front();
        attach_queue.pop();
#endif
        e->add_ssg_nodes( globals->get_scenery()->get_terrain_branch(),
                          globals->get_scenery()->get_gnd_lights_root(),
                          globals->get_scenery()->get_vasi_lights_root(),
                          globals->get_scenery()->get_rwy_lights_root(),
                          globals->get_scenery()->get_taxi_lights_root() );
        // cout << "Adding ssg nodes for "
    }

    if ( !delete_queue.empty() ) {
        // cout << "delete queue = " << delete_queue.size() << endl;
        bool processed_one = false;

        while ( delete_queue.size() > 30 || processed_one == false ) {
            processed_one = true;

            if ( delete_queue.size() > 30 ) {
                // uh oh, delete queue is blowing up, we aren't clearing
                // it fast enough.  Let's just panic, well not panic, but
                // get real serious and agressively free up some tiles so
                // we don't explode our memory usage.

                SG_LOG( SG_TERRAIN, SG_ALERT,
                        "Warning: catching up on tile delete queue" );
            }

            FGTileEntry* e = delete_queue.front();
            if ( e->free_tile() ) {
                delete_queue.pop();
                delete e;
            }
        }
    }
}


// given the current lon/lat (in degrees), fill in the array of local
// chunks.  If the chunk isn't already in the cache, then read it from
// disk.
int FGTileMgr::update( double visibility_meters )
{
    SGLocation *location = globals->get_current_view()->getSGLocation();
    return update( location, visibility_meters );
}


int FGTileMgr::update( SGLocation *location, double visibility_meters )
{
    SG_LOG( SG_TERRAIN, SG_DEBUG, "FGTileMgr::update()" );

    longitude = location->getLongitude_deg();
    latitude = location->getLatitude_deg();
    // add 1.0m to the max altitude to give a little leeway to the
    // ground reaction code.
    altitude_m = location->getAltitudeASL_ft() * SG_FEET_TO_METER + 1.0;

    // if current altitude is apparently not initialized, set max
    // altitude to something big.
    if ( altitude_m < -1000 ) {
        altitude_m = 10000;
    }
    // SG_LOG( SG_TERRAIN, SG_DEBUG, "FGTileMgr::update() for "
    //         << longitude << " " << latatitude );

    current_bucket.set_bucket( longitude, latitude );
    // SG_LOG( SG_TERRAIN, SG_DEBUG, "Updating tile list for "
    //         << current_bucket );
    fgSetInt( "/environment/current-tile-id", current_bucket.gen_index() );

    // do tile load scheduling. 
    // Note that we need keep track of both viewer buckets and fdm buckets.
    if ( state == Running ) {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "State == Running" );
        if (!(current_bucket == previous_bucket )) {
            // We've moved to a new bucket, we need to schedule any
            // needed tiles for loading.
            SG_LOG( SG_TERRAIN, SG_INFO, "FGTileMgr::update()" );
            schedule_needed(visibility_meters, current_bucket);
        }
    } else if ( state == Start || state == Inited ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "State == Start || Inited" );
//        initialize_queue();
        state = Running;

        // load the next tile in the load queue (or authorize the next
        // load in the case of the threaded tile pager)
        loader.update();
    }

    update_queues();

    // save bucket...
    previous_bucket = current_bucket;

    return 1;
}


// timer event driven call to scheduler for the purpose of refreshing the tile timestamps
void FGTileMgr::refresh_view_timestamps() {
    SG_LOG( SG_TERRAIN, SG_INFO,
            "Refreshing timestamps for " << current_bucket.get_center_lon()
            << " " << current_bucket.get_center_lat() );
    if ( longitude >= -180.0 && longitude <= 180.0 
         && latitude >= -90.0 && latitude <= 90.0 )
    {
        schedule_needed(fgGetDouble("/environment/visibility-m"), current_bucket);
    }
}

void FGTileMgr::prep_ssg_nodes( SGLocation *location, float vis ) {

    // traverse the potentially viewable tile list and update range
    // selector and transform

    float *up = location->get_world_up();

    FGTileEntry *e;
    tile_cache.reset_traversal();

    const double *vp = location->get_absolute_view_pos();
    Point3D viewpos(vp[0], vp[1], vp[2]);
    while ( ! tile_cache.at_end() ) {
        // cout << "processing a tile" << endl;
        if ( (e = tile_cache.get_current()) ) {
            e->prep_ssg_node( viewpos, up, vis);
        } else {
            SG_LOG(SG_INPUT, SG_ALERT, "warning ... empty tile in cache");
        }
        tile_cache.next();
    }
}

bool FGTileMgr::set_tile_filter( bool f ) {
    bool old = tile_filter;
    tile_filter = f;
    return old;
}

int FGTileMgr::tile_filter_cb( ssgEntity *, int )
{
  return tile_filter ? 1 : 0;
}

bool FGTileMgr::scenery_available(double lat, double lon, double range_m)
{
  // sanity check (unfortunately needed!)
  if ( lon < -180.0 || lon > 180.0 || lat < -90.0 || lat > 90.0 )
    return false;
  
  SGBucket bucket(lon, lat);
  FGTileEntry *te = tile_cache.get_tile(bucket);
  if (!te || !te->is_loaded())
    return false;

  // Traverse all tiles required to be there for the given visibility.
  // This uses exactly the same algorithm like the tile scheduler.
  double tile_width = bucket.get_width_m();
  double tile_height = bucket.get_height_m();
  
  int xrange = (int)fabs(range_m / tile_width) + 1;
  int yrange = (int)fabs(range_m / tile_height) + 1;
  
  for ( int x = -xrange; x <= xrange; ++x ) {
    for ( int y = -yrange; y <= yrange; ++y ) {
      // We have already checked for the center tile.
      if ( x != 0 || y != 0 ) {
        SGBucket b = sgBucketOffset( lon, lat, x, y );
        FGTileEntry *te = tile_cache.get_tile(b);
        if (!te || !te->is_loaded())
          return false;
      }
    }
  }

  // Survived all tests.
  return true;
}
