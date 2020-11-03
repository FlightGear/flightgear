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
#include <simgear/misc/strutils.hxx>
#include <simgear/scene/material/matlib.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/splash.hxx>
#include <Scripting/NasalSys.hxx>
#include <Scripting/NasalModelData.hxx>

#include "scenery.hxx"
#include "SceneryPager.hxx"
#include "tilemgr.hxx"

using flightgear::SceneryPager;

class FGTileMgr::TileManagerListener : public SGPropertyChangeListener
{
public:
    TileManagerListener(FGTileMgr* manager) :
        _manager(manager),
        _useVBOsProp(fgGetNode("/sim/rendering/use-vbos", true)),
        _enableCacheProp(fgGetNode("/sim/tile-cache/enable", true)),
        _pagedLODMaximumProp(fgGetNode("/sim/rendering/max-paged-lod", true)),
        _lodDetailed(fgGetNode("/sim/rendering/static-lod/detailed", true)),
        _lodRoughDelta(fgGetNode("/sim/rendering/static-lod/rough-delta", true)),
        _lodBareDelta(fgGetNode("/sim/rendering/static-lod/bare-delta", true)),
        _lodRough(fgGetNode("/sim/rendering/static-lod/rough", true)),
        _lodBare(fgGetNode("/sim/rendering/static-lod/bare", true))
    {
        _useVBOsProp->addChangeListener(this, true);

        _enableCacheProp->addChangeListener(this, true);
        if (_enableCacheProp->getType() == simgear::props::NONE) {
            _enableCacheProp->setBoolValue(true);
        }

        if (_pagedLODMaximumProp->getType() == simgear::props::NONE) {
            // not set, use OSG default / environment value variable
            osg::ref_ptr<osgViewer::Viewer> viewer(globals->get_renderer()->getViewer());
            int current = viewer->getDatabasePager()->getTargetMaximumNumberOfPageLOD();
            _pagedLODMaximumProp->setIntValue(current);
        }
        _pagedLODMaximumProp->addChangeListener(this, true);
        _lodDetailed->addChangeListener(this, true);
        _lodBareDelta->addChangeListener(this, true);
        _lodRoughDelta->addChangeListener(this, true);
    }

    ~TileManagerListener()
    {
        _useVBOsProp->removeChangeListener(this);
        _enableCacheProp->removeChangeListener(this);
        _pagedLODMaximumProp->removeChangeListener(this);
        _lodDetailed->removeChangeListener(this);
        _lodBareDelta->removeChangeListener(this);
        _lodRoughDelta->removeChangeListener(this);
    }

    virtual void valueChanged(SGPropertyNode* prop)
    {
        if (prop == _useVBOsProp) {
            bool useVBOs = prop->getBoolValue();
            _manager->_options->setPluginStringData("SimGear::USE_VBOS",
                                                    useVBOs ? "ON" : "OFF");
        } else if (prop == _enableCacheProp) {
            _manager->_enableCache = prop->getBoolValue();
        } else if (prop == _pagedLODMaximumProp) {
            int                             v = prop->getIntValue();
            osg::ref_ptr<osgViewer::Viewer> viewer(globals->get_renderer()->getViewer());
            if (viewer) {
              osgDB::DatabasePager* pager = viewer->getDatabasePager();
              if (pager) pager->setTargetMaximumNumberOfPageLOD(v);
            }
        } else if (prop == _lodDetailed || prop == _lodBareDelta || prop == _lodRoughDelta) {
            // compatibility with earlier versions; set the static lod ranges appropriately as otherwise (bad) self managed
            // LOD on scenery with range animations doesn't work.
            // see also /sim/rendering/enable-range-lod-animations - which is false by default in > 2019.2 which also fixes
            // the scenery but in a more efficient way.
            _lodRough->setDoubleValue(_lodDetailed->getDoubleValue() + _lodRoughDelta->getDoubleValue());
            _lodBare->setDoubleValue(_lodRough->getDoubleValue() + _lodBareDelta->getDoubleValue());
        }
    }

private:
    FGTileMgr* _manager;
    SGPropertyNode_ptr _useVBOsProp,
      _enableCacheProp,
      _pagedLODMaximumProp,
      _lodDetailed,
      _lodRoughDelta,
      _lodBareDelta,
      _lodRough,
      _lodBare
        ;
};

FGTileMgr::FGTileMgr():
    state( Start ),
    last_state( Running ),
    scheduled_visibility(100.0),
    _visibilityMeters(fgGetNode("/environment/visibility-m", true)),
    _lodDetailed(fgGetNode("/sim/rendering/static-lod/detailed", true)),
    _lodRoughDelta(fgGetNode("/sim/rendering/static-lod/rough-delta", true)),
    _lodBareDelta(fgGetNode("/sim/rendering/static-lod/bare-delta", true)),
    _disableNasalHooks(fgGetNode("/sim/temp/disable-scenery-nasal", true)),
    _scenery_loaded(fgGetNode("/sim/sceneryloaded", true)),
    _scenery_override(fgGetNode("/sim/sceneryloaded-override", true)),
    _pager(FGScenery::getPagerSingleton()),
    _enableCache(true)
{
}


FGTileMgr::~FGTileMgr()
{
}

// Initialize the Tile Manager subsystem
void FGTileMgr::init()
{
    reinit();
}

void FGTileMgr::shutdown()
{
    _listener.reset();

    FGScenery* scenery = globals->get_scenery();
    if (scenery && scenery->get_terrain_branch()) {
        osg::Group* group = scenery->get_terrain_branch();
        group->removeChildren(0, group->getNumChildren());
    }
    // clear OSG cache
    osgDB::Registry::instance()->clearObjectCache();
    state = Start; // need to init again
}

void FGTileMgr::reinit()
{
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing Tile Manager subsystem." );
    auto terraSync = globals->get_subsystem<simgear::SGTerraSync>();

    // drops the previous options reference
    _options = new simgear::SGReaderWriterOptions;
    _listener.reset(new TileManagerListener(this));

    materialLibChanged();
    _options->setPropertyNode(globals->get_props());

    osgDB::FilePathList &fp = _options->getDatabasePathList();
    const PathList &sc = globals->get_fg_scenery();
    fp.clear();
    for (auto it = sc.begin(); it != sc.end(); ++it) {
        fp.push_back(it->utf8Str());
    }
    _options->setPluginStringData("SimGear::FG_ROOT", globals->get_fg_root().utf8Str());

    if (terraSync) {
        _options->setPluginStringData("SimGear::TERRASYNC_ROOT", globals->get_terrasync_dir().utf8Str());
    }

    if (!_disableNasalHooks->getBoolValue())
      _options->setModelData(new FGNasalModelDataProxy);

    double detailed = fgGetDouble("/sim/rendering/static-lod/detailed", SG_OBJECT_RANGE_DETAILED);
    double rough    = fgGetDouble("/sim/rendering/static-lod/rough-delta", SG_OBJECT_RANGE_ROUGH) + detailed;
    double bare     = fgGetDouble("/sim/rendering/static-lod/bare", SG_OBJECT_RANGE_BARE) + rough;

    _options->setPluginStringData("SimGear::LOD_RANGE_BARE", std::to_string(bare));
    _options->setPluginStringData("SimGear::LOD_RANGE_ROUGH", std::to_string(rough));
    _options->setPluginStringData("SimGear::LOD_RANGE_DETAILED", std::to_string(detailed));

    string_list scenerySuffixes;
    for (auto node : fgGetNode("/sim/rendering/", true)->getChildren("scenery-path-suffix")) {
        if (node->getBoolValue("enabled", true)) {
            scenerySuffixes.push_back(node->getStringValue("name"));
        }
    }

    if (scenerySuffixes.empty()) {
        // if preferences didn't load, use some default
        scenerySuffixes = {"Objects", "Terrain"}; // defaut values
    }

    _options->setSceneryPathSuffixes(scenerySuffixes);

    if (state != Start)
    {
      // protect against multiple scenery reloads and properly reset flags,
      // otherwise aircraft fall through the ground while reloading scenery
      if (_scenery_loaded->getBoolValue() == false) {
        SG_LOG( SG_TERRAIN, SG_INFO, "/sim/sceneryloaded already false, avoiding duplicate re-init of tile manager" );
        return;
      }
    }

    _scenery_loaded->setBoolValue(false);
    fgSetDouble("/sim/startup/splash-alpha", 1.0);

    materialLibChanged();

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
    scheduled_visibility = 100.0;

    // force an update now
    update(0.0);
}

void FGTileMgr::materialLibChanged()
{
    _options->setMaterialLib(globals->get_matlib());
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
        SG_LOG( SG_TERRAIN, SG_INFO, "sched_tile: new tile entry for:" << b );


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
    if (!curr_bucket.isValid() )
    {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Attempting to schedule tiles for invalid bucket" );
        return;
    }

    double tile_width = curr_bucket.get_width_m();
    double tile_height = curr_bucket.get_height_m();
    SG_LOG( SG_TERRAIN, SG_INFO,
            "scheduling needed tiles for " << curr_bucket
           << ", tile-width-m:" << tile_width << ", tile-height-m:" << tile_height);


    // cout << "tile width = " << tile_width << "  tile_height = "
    //      << tile_height << endl;
    // starting with 2018.3 we will use deltas rather than absolutes as it is more intuitive for the user
    // and somewhat easier to visualise
    double maxTileRange = _lodDetailed->getDoubleValue() + _lodRoughDelta->getDoubleValue() + _lodBareDelta->getDoubleValue();

    double tileRangeM = std::min(vis, maxTileRange);
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
    auto terraSync = globals->get_subsystem<simgear::SGTerraSync>();

    /* schedule all tiles, use distance-based loading priority,
     * so tiles are loaded in innermost-to-outermost sequence. */
    for ( x = -xrange; x <= xrange; ++x )
    {
        for ( y = -yrange; y <= yrange; ++y )
        {
            SGBucket b = curr_bucket.sibling(x, y);
            if (!b.isValid()) {
                continue;
            }

            float priority = (-1.0) * (x*x+y*y);
            sched_tile( b, priority, true, 0.0 );

            if (terraSync) {
                terraSync->scheduleTile(b);
            }
        }
    }
}

/**
 * Update the various queues maintained by the tilemgr (private
 * internal function, do not call directly.)
 */
void FGTileMgr::update_queues(bool& isDownloadingScenery)
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
        if ( e )
        {
            // Prepare the ssg nodes corresponding to each tile.
            // Set the ssg transform and update it's range selector
            // based on current visibilty
            e->prep_ssg_node(vis);

            if (!e->is_loaded()) {
                bool nonExpiredOrCurrent = !e->is_expired(current_time) || e->is_current_view();
                bool downloading = isTileDirSyncing(e->tileFileName);
                isDownloadingScenery |= downloading;
                if ( !downloading && nonExpiredOrCurrent) {
                    // schedule tile for loading with osg pager
                    _pager->queueRequest(e->tileFileName,
                                         e->getNode(),
                                         e->get_priority(),
                                         framestamp,
                                         e->getDatabaseRequest(),
                                         _options.get());
                    loading++;
                }
            } // of tile not loaded case
        } else {
            SG_LOG(SG_TERRAIN, SG_ALERT, "Warning: empty tile in cache!");
        }
        tile_cache.next();
        sz++;
    }

    int drop_count = sz - tile_cache.get_max_cache_size();
    bool dropTiles = false;
    if (_enableCache) {
      dropTiles = ( drop_count > 0 ) && ((loading==0)||(drop_count > 10));
    } else {
      dropTiles = true;
      drop_count = sz; // no limit on tiles to drop
    }

    if (dropTiles)
    {
        long drop_index = _enableCache ? tile_cache.get_drop_tile() :
                                         tile_cache.get_first_expired_tile();
        while ( drop_index > -1 )
        {
            // schedule tile for deletion with osg pager
            TileEntry* old = tile_cache.get_tile(drop_index);
            SG_LOG(SG_TERRAIN, SG_DEBUG, "Dropping:" << old->get_tile_bucket());

            tile_cache.clear_entry(drop_index);

            osg::ref_ptr<osg::Object> subgraph = old->getNode();
            old->removeFromSceneGraph();
            delete old;
            // zeros out subgraph ref_ptr, so subgraph is owned by
            // the pager and will be deleted in the pager thread.
            _pager->queueDeleteRequest(subgraph);

            if (!_enableCache)
                drop_index = tile_cache.get_first_expired_tile();
            // limit tiles dropped to drop_count
            else if (--drop_count > 0)
                drop_index = tile_cache.get_drop_tile();
            else
               drop_index = -1;
        }
    } // of dropping tiles loop
}

// given the current lon/lat (in degrees), fill in the array of local
// chunks.  If the chunk isn't already in the cache, then read it from
// disk.
void FGTileMgr::update(double)
{
    double vis = _visibilityMeters->getDoubleValue();
    schedule_tiles_at(globals->get_view_position(), vis);

    bool waitingOnTerrasync = false;
    update_queues(waitingOnTerrasync);

    // scenery loading check, triggers after each sim (tile manager) reinit
    if (!_scenery_loaded->getBoolValue())
    {
        bool fdmInited = fgGetBool("sim/fdm-initialized");
        bool positionFinalized = fgGetBool("sim/position-finalized");
        bool sceneryOverride = _scenery_override->getBoolValue();


    // we are done if final position is set and the scenery & FDM are done.
    // scenery-override can ignore the last two, but not position finalization.
        if (positionFinalized && (sceneryOverride || (isSceneryLoaded() && fdmInited)))
        {
            _scenery_loaded->setBoolValue(true);
            fgSplashProgress("");
        }
        else
        {
            if (!positionFinalized) {
                fgSplashProgress("finalize-position");
            } else if (waitingOnTerrasync) {
                fgSplashProgress("downloading-scenery");
            } else {
                fgSplashProgress("loading-scenery");
            }

            // be nice to loader threads while waiting for initial scenery, reduce to 20fps
            SGTimeStamp::sleepForMSec(50);
        }
    }
}

// schedule tiles for the viewer bucket
// (FDM/AI/groundcache/... should use "schedule_scenery" instead)
void FGTileMgr::schedule_tiles_at(const SGGeod& location, double range_m)
{
    // SG_LOG( SG_TERRAIN, SG_DEBUG, "FGTileMgr::update() for "
    //         << longitude << " " << latitude );

    current_bucket = SGBucket( location );

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
            SG_LOG( SG_TERRAIN, SG_INFO, "FGTileMgr: at " << location << ", scheduling needed for:" << current_bucket
                   << ", visibility=" << range_m);
            scheduled_visibility = range_m;
            schedule_needed(current_bucket, range_m);
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
}

/** Schedules scenery for given position. Load request remains valid for given duration
 * (duration=0.0 => nothing is loaded).
 * Used for FDM/AI/groundcache/... requests. Viewer uses "schedule_tiles_at" instead.
 * Returns true when all tiles for the given position are already loaded, false otherwise.
 */
bool FGTileMgr::schedule_scenery(const SGGeod& position, double range_m, double duration)
{
    // sanity check (unfortunately needed!)
    if (!position.isValid())
        return false;
    const float priority = 0.0;
    bool available = true;

    SGBucket bucket(position);
    available = sched_tile( bucket, priority, false, duration );

    if ((!available)&&(duration==0.0)) {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "schedule_scenery: Scheduling tile at bucket:" << bucket << " return false" );
        return false;
    }

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
                SGBucket b = bucket.sibling(x, y );
                if (!b.isValid()) {
                    continue;
                }

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

    return schedule_scenery(globals->get_view_position(), range_m, 0.0);
}

bool FGTileMgr::isTileDirSyncing(const std::string& tileFileName) const
{
    auto terraSync = globals->get_subsystem<simgear::SGTerraSync>();
    if (!terraSync) {
        return false;
    }

    // if Models is syncing, also wait for it, since otherwise
    // we get load errors
    if (terraSync->isDataDirPending("Models")) {
        return true;
    }

    std::string nameWithoutExtension = tileFileName.substr(0, tileFileName.size() - 4);
    long int bucketIndex = simgear::strutils::to_int(nameWithoutExtension);
    SGBucket bucket(bucketIndex);

    return terraSync->isTileDirPending(bucket.gen_base_path());
}
