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

#include <algorithm>
#include <functional>

#include <osgViewer/Viewer>
#include <osgDB/Registry>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/tsync/terrasync.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/viewer.hxx>
#include <Scripting/NasalSys.hxx>

#include "scenery.hxx"
#include "SceneryPager.hxx"
#include "tilemgr.hxx"

using flightgear::SceneryPager;


FGTileMgr::FGTileMgr():
    state( Start ),
    last_state( Running ),
    longitude(-1000.0),
    latitude(-1000.0),
    scheduled_visibility(100.0),
    _terra_sync(NULL),
    _visibilityMeters(fgGetNode("/environment/visibility-m", true)),
    _maxTileRangeM(fgGetNode("/sim/rendering/static-lod/bare", true)),
    _disableNasalHooks(fgGetNode("/sim/temp/disable-scenery-nasal", true)),
    _pager(FGScenery::getPagerSingleton())
{
}


FGTileMgr::~FGTileMgr()
{
    // remove all nodes we might have left behind
    osg::Group* group = globals->get_scenery()->get_terrain_branch();
    group->removeChildren(0, group->getNumChildren());
    // clear OSG cache
    osgDB::Registry::instance()->clearObjectCache();
}


// Initialize the Tile Manager subsystem
void FGTileMgr::init() {
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing Tile Manager subsystem." );

    _options = new simgear::SGReaderWriterOptions;
    _options->setMaterialLib(globals->get_matlib());
    _options->setPropertyNode(globals->get_props());

    osgDB::FilePathList &fp = _options->getDatabasePathList();
    const string_list &sc = globals->get_fg_scenery();
    fp.clear();
    std::copy(sc.begin(), sc.end(), back_inserter(fp));
    _options->setPluginStringData("SimGear::FG_ROOT", globals->get_fg_root());
    if (!_disableNasalHooks->getBoolValue())
        _options->setModelData(new FGNasalModelDataProxy);

    reinit();
}

void FGTileMgr::refresh_tile(void* tileMgr, long tileIndex)
{
    ((FGTileMgr*) tileMgr)->tile_cache.refresh_tile(tileIndex);
}

void FGTileMgr::reinit()
{
    // protect against multiple scenery reloads and properly reset flags,
    // otherwise aircraft fall through the ground while reloading scenery
    if (!fgGetBool("/sim/sceneryloaded",true))
        return;
    fgSetBool("/sim/sceneryloaded",false);
    fgSetDouble("/sim/startup/splash-alpha", 1.0);

    // remove all old scenery nodes from scenegraph and clear cache
    osg::Group* group = globals->get_scenery()->get_terrain_branch();
    group->removeChildren(0, group->getNumChildren());
    tile_cache.init();
    
    // clear OSG cache, except on initial start-up
    if (state != Start)
    {
        osgDB::Registry::instance()->clearObjectCache();
    }
    
    state = Inited;
    
    previous_bucket.make_bad();
    current_bucket.make_bad();
    longitude = latitude = -1000.0;
    scheduled_visibility = 100.0;

    _terra_sync = (simgear::SGTerraSync*) globals->get_subsystem("terrasync");
    if (_terra_sync)
        _terra_sync->setTileRefreshCb(&refresh_tile, this);

    // force an update now
    update(0.0);
}

/* schedule a tile for loading, keep request for given amount of time.
 * Returns true if tile is already loaded. */
bool FGTileMgr::sched_tile( const SGBucket& b, double priority, bool current_view, double duration)
{
    // see if tile already exists in the cache
    TileEntry *t = tile_cache.get_tile( b );
    if (!t)
    {
        // create a new entry
        t = new TileEntry( b );
        // insert the tile into the cache, update will generate load request
        if ( tile_cache.insert_tile( t ) )
        {
            // Attach to scene graph

            t->addToSceneGraph(globals->get_scenery()->get_terrain_branch());
        } else
        {
            // insert failed (cache full with no available entries to
            // delete.)  Try again later
            delete t;
            return false;
        }

        SG_LOG( SG_TERRAIN, SG_DEBUG, "  New tile cache size " << (int)tile_cache.get_size() );
    }

    // update tile's properties
    tile_cache.request_tile(t,priority,current_view,duration);

    return t->is_loaded();
}

/* schedule needed buckets for the current view position for loading,
 * keep request for given amount of time */
void FGTileMgr::schedule_needed(const SGBucket& curr_bucket, double vis)
{
    // sanity check (unfortunately needed!)
    if ( longitude < -180.0 || longitude > 180.0 
         || latitude < -90.0 || latitude > 90.0 )
    {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Attempting to schedule tiles for bogus lon and lat  = ("
                << longitude << "," << latitude << ")" );
        return;
    }

    SG_LOG( SG_TERRAIN, SG_INFO,
            "scheduling needed tiles for " << longitude << " " << latitude );

    double tile_width = curr_bucket.get_width_m();
    double tile_height = curr_bucket.get_height_m();
    // cout << "tile width = " << tile_width << "  tile_height = "
    //      << tile_height << endl;

    double tileRangeM = std::min(vis,_maxTileRangeM->getDoubleValue());
    int xrange = (int)(tileRangeM / tile_width) + 1;
    int yrange = (int)(tileRangeM / tile_height) + 1;
    if ( xrange < 1 ) { xrange = 1; }
    if ( yrange < 1 ) { yrange = 1; }

    // make the cache twice as large to avoid losing terrain when switching
    // between aircraft and tower views
    tile_cache.set_max_cache_size( (2*xrange + 2) * (2*yrange + 2) * 2 );
    // cout << "xrange = " << xrange << "  yrange = " << yrange << endl;
    // cout << "max cache size = " << tile_cache.get_max_cache_size()
    //      << " current cache size = " << tile_cache.get_size() << endl;

    // clear flags of all tiles belonging to the previous view set 
    tile_cache.clear_current_view();

    // update timestamps, so all tiles scheduled now are *newer* than any tile previously loaded
    osg::FrameStamp* framestamp
            = globals->get_renderer()->getViewer()->getFrameStamp();
    tile_cache.set_current_time(framestamp->getReferenceTime());

    SGBucket b;

    int x, y;

    /* schedule all tiles, use distance-based loading priority,
     * so tiles are loaded in innermost-to-outermost sequence. */
    for ( x = -xrange; x <= xrange; ++x )
    {
        for ( y = -yrange; y <= yrange; ++y )
        {
            SGBucket b = sgBucketOffset( longitude, latitude, x, y );
            float priority = (-1.0) * (x*x+y*y);
            sched_tile( b, priority, true, 0.0 );
        }
    }
}

/**
 * Update the various queues maintained by the tilemagr (private
 * internal function, do not call directly.)
 */
void FGTileMgr::update_queues()
{
    osg::FrameStamp* framestamp
        = globals->get_renderer()->getViewer()->getFrameStamp();
    double current_time = framestamp->getReferenceTime();
    double vis = _visibilityMeters->getDoubleValue();
    TileEntry *e;
    int loading=0;
    int sz=0;

    tile_cache.set_current_time( current_time );
    tile_cache.reset_traversal();

    while ( ! tile_cache.at_end() )
    {
        e = tile_cache.get_current();
        // cout << "processing a tile" << endl;
        if ( e )
        {
            // Prepare the ssg nodes corresponding to each tile.
            // Set the ssg transform and update it's range selector
            // based on current visibilty
            e->prep_ssg_node(vis);

            if (( !e->is_loaded() )&&
                ((!e->is_expired(current_time))||
                  e->is_current_view() ))
            {
                // schedule tile for loading with osg pager
                _pager->queueRequest(e->tileFileName,
                                     e->getNode(),
                                     e->get_priority(),
                                     framestamp,
                                     e->getDatabaseRequest(),
                                     _options.get());
                loading++;
            }
        } else
        {
            SG_LOG(SG_TERRAIN, SG_ALERT, "Warning: empty tile in cache!");
        }
        tile_cache.next();
        sz++;
    }

    int drop_count = sz - tile_cache.get_max_cache_size();
    if (( drop_count > 0 )&&
         ((loading==0)||(drop_count > 10)))
    {
        long drop_index = tile_cache.get_drop_tile();
        while ( drop_index > -1 )
        {
            // schedule tile for deletion with osg pager
            TileEntry* old = tile_cache.get_tile(drop_index);
            tile_cache.clear_entry(drop_index);
            
            osg::ref_ptr<osg::Object> subgraph = old->getNode();
            old->removeFromSceneGraph();
            delete old;
            // zeros out subgraph ref_ptr, so subgraph is owned by
            // the pager and will be deleted in the pager thread.
            _pager->queueDeleteRequest(subgraph);
            
            if (--drop_count > 0)
                drop_index = tile_cache.get_drop_tile();
            else
                drop_index = -1;
        }
    }
}

// given the current lon/lat (in degrees), fill in the array of local
// chunks.  If the chunk isn't already in the cache, then read it from
// disk.
void FGTileMgr::update(double)
{
    SGVec3d viewPos = globals->get_current_view()->get_view_pos();
    double vis = _visibilityMeters->getDoubleValue();
    schedule_tiles_at(SGGeod::fromCart(viewPos), vis);

    update_queues();
}

// schedule tiles for the viewer bucket (FDM/AI/groundcache/... use
// "schedule_scenery" instead
int FGTileMgr::schedule_tiles_at(const SGGeod& location, double range_m)
{
    longitude = location.getLongitudeDeg();
    latitude = location.getLatitudeDeg();

    // SG_LOG( SG_TERRAIN, SG_DEBUG, "FGTileMgr::update() for "
    //         << longitude << " " << latatitude );

    current_bucket.set_bucket( location );

    // schedule more tiles when visibility increased considerably
    // TODO Calculate tile size - instead of using fixed value (5000m)
    if (range_m - scheduled_visibility > 5000.0)
        previous_bucket.make_bad();

    // SG_LOG( SG_TERRAIN, SG_DEBUG, "Updating tile list for "
    //         << current_bucket );
    fgSetInt( "/environment/current-tile-id", current_bucket.gen_index() );

    // do tile load scheduling.
    // Note that we need keep track of both viewer buckets and fdm buckets.
    if ( state == Running ) {
        if (last_state != state)
        {
            SG_LOG( SG_TERRAIN, SG_DEBUG, "State == Running" );
        }
        if (current_bucket != previous_bucket) {
            // We've moved to a new bucket, we need to schedule any
            // needed tiles for loading.
            SG_LOG( SG_TERRAIN, SG_INFO, "FGTileMgr::update()" );
            scheduled_visibility = range_m;
            schedule_needed(current_bucket, range_m);
            if (_terra_sync)
                _terra_sync->schedulePosition(latitude,longitude);
        }
        // save bucket
        previous_bucket = current_bucket;
    } else if ( state == Start || state == Inited ) {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "State == Start || Inited" );
        // do not update bucket yet (position not valid in initial loop)
        state = Running;
        previous_bucket.make_bad();
    }
    last_state = state;

    return 1;
}

/** Schedules scenery for given position. Load request remains valid for given duration
 * (duration=0.0 => nothing is loaded).
 * Used for FDM/AI/groundcache/... requests. Viewer uses "schedule_tiles_at" instead.
 * Returns true when all tiles for the given position are already loaded, false otherwise.
 */
bool FGTileMgr::schedule_scenery(const SGGeod& position, double range_m, double duration)
{
    const float priority = 0.0;
    double current_longitude = position.getLongitudeDeg();
    double current_latitude = position.getLatitudeDeg();
    bool available = true;
    
    // sanity check (unfortunately needed!)
    if (current_longitude < -180 || current_longitude > 180 ||
        current_latitude < -90 || current_latitude > 90)
        return false;
  
    SGBucket bucket(position);
    available = sched_tile( bucket, priority, false, duration );
  
    if ((!available)&&(duration==0.0))
        return false;

    SGVec3d cartPos = SGVec3d::fromGeod(position);

    // Traverse all tiles required to be there for the given visibility.
    double tile_width = bucket.get_width_m();
    double tile_height = bucket.get_height_m();
    double tile_r = 0.5*sqrt(tile_width*tile_width + tile_height*tile_height);
    double max_dist = tile_r + range_m;
    double max_dist2 = max_dist*max_dist;
    
    int xrange = (int)fabs(range_m / tile_width) + 1;
    int yrange = (int)fabs(range_m / tile_height) + 1;

    for ( int x = -xrange; x <= xrange; ++x )
    {
        for ( int y = -yrange; y <= yrange; ++y )
        {
            // We have already checked for the center tile.
            if ( x != 0 || y != 0 )
            {
                SGBucket b = sgBucketOffset( current_longitude,
                                             current_latitude, x, y );
                double distance2 = distSqr(cartPos, SGVec3d::fromGeod(b.get_center()));
                // Do not ask if it is just the next tile but way out of range.
                if (distance2 <= max_dist2)
                {
                    available &= sched_tile( b, priority, false, duration );
                    if ((!available)&&(duration==0.0))
                        return false;
                }
            }
        }
    }

    return available;
}

// Returns true if tiles around current view position have been loaded
bool FGTileMgr::isSceneryLoaded()
{
    double range_m = 100.0;
    if (scheduled_visibility < range_m)
        range_m = scheduled_visibility;

    return schedule_scenery(SGGeod::fromDeg(longitude, latitude), range_m, 0.0);
}
