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

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/tgdb/SGReaderWriterBTGOptions.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/renderer.hxx>
#include <Main/viewer.hxx>
#include <Scripting/NasalSys.hxx>

#include "scenery.hxx"
#include "SceneryPager.hxx"
#include "tilemgr.hxx"

using std::for_each;
using flightgear::SceneryPager;
using simgear::SGModelLib;
using simgear::TileEntry;
using simgear::TileCache;

FGTileMgr::FGTileMgr():
    state( Start ),
    vis( 16000 )
{
}


FGTileMgr::~FGTileMgr() {
    // remove all nodes we might have left behind
    osg::Group* group = globals->get_scenery()->get_terrain_branch();
    group->removeChildren(0, group->getNumChildren());
}


// Initialize the Tile Manager subsystem
int FGTileMgr::init() {
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing Tile Manager subsystem." );

    _options = new SGReaderWriterBTGOptions;
    _options->setMatlib(globals->get_matlib());
    _options->setUseRandomObjects(fgGetBool("/sim/rendering/random-objects", true));
    _options->setUseRandomVegetation(fgGetBool("/sim/rendering/random-vegetation", true));
    osgDB::FilePathList &fp = _options->getDatabasePathList();
    const string_list &sc = globals->get_fg_scenery();
    fp.clear();
    std::copy(sc.begin(), sc.end(), back_inserter(fp));

    TileEntry::setModelLoadHelper(this);

    tile_cache.init();

    state = Inited;

    previous_bucket.make_bad();
    current_bucket.make_bad();

    longitude = latitude = -1000.0;

    return 1;
}

// schedule a tile for loading
void FGTileMgr::sched_tile( const SGBucket& b, const bool is_inner_ring ) {
    // see if tile already exists in the cache
    TileEntry *t = tile_cache.get_tile( b );

    if ( !t ) {
        // make space in the cache
        SceneryPager* pager = FGScenery::getPagerSingleton();
        while ( (int)tile_cache.get_size() > tile_cache.get_max_cache_size() ) {
            long index = tile_cache.get_oldest_tile();
            if ( index >= 0 ) {
                TileEntry *old = tile_cache.get_tile( index );
                tile_cache.clear_entry( index );
                osg::ref_ptr<osg::Object> subgraph = old->getNode();
                old->removeFromSceneGraph();
                delete old;
                // zeros out subgraph ref_ptr, so subgraph is owned by
                // the pager and will be deleted in the pager thread.
                pager->queueDeleteRequest(subgraph);
            } else {
                // nothing to free ?!? forge ahead
                break;
            }
        }

        // create a new entry
        TileEntry *e = new TileEntry( b );

        // insert the tile into the cache
        if ( tile_cache.insert_tile( e ) ) {
            // update_queues will generate load request
        } else {
            // insert failed (cache full with no available entries to
            // delete.)  Try again later
            delete e;
        }
        // Attach to scene graph
        e->addToSceneGraph(globals->get_scenery()->get_terrain_branch());
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
        return;		// FIXME
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

    // make the cache twice as large to avoid losing terrain when switching
    // between aircraft and tower views
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
}

osg::Node*
FGTileMgr::loadTileModel(const string& modelPath, bool cacheModel)
{
    SGPath fullPath;
    if (fgGetBool("/sim/paths/use-custom-scenery-data") == true) {
        string_list sc = globals->get_fg_scenery();

        for (string_list_iterator it = sc.begin(); it != sc.end(); ++it) {
            SGPath tmpPath(*it);
            tmpPath.append(modelPath);
            if (tmpPath.exists()) {
                fullPath = tmpPath;
                break;
            } 
        }
    } else {
         fullPath.append(modelPath);
    }
    osg::Node* result = 0;
    try {
        if(cacheModel)
            result =
                SGModelLib::loadModel(fullPath.str(), globals->get_props(),
                                      new FGNasalModelData);
        else
            result=
                SGModelLib::loadPagedModel(modelPath, globals->get_props(),
                                           new FGNasalModelData);
    } catch (const sg_io_exception& exc) {
        string m(exc.getMessage());
        m += " ";
        m += exc.getLocation().asString();
        SG_LOG( SG_ALL, SG_ALERT, m );
    } catch (const sg_exception& exc) { // XXX may be redundant
        SG_LOG( SG_ALL, SG_ALERT, exc.getMessage());
    }
    return result;
}

// Helper class for STL fun
class TileLoad : public std::unary_function<TileCache::tile_map::value_type,
                                            void>
{
public:
    TileLoad(SceneryPager *pager, osg::FrameStamp* framestamp,
             osg::Group* terrainBranch, osgDB::ReaderWriter::Options* options) :
        _pager(pager), _framestamp(framestamp), _options(options) {}

    TileLoad(const TileLoad& rhs) :
        _pager(rhs._pager), _framestamp(rhs._framestamp),
        _options(rhs._options) {}

    void operator()(TileCache::tile_map::value_type& tilePair)
    {
        TileEntry* entry = tilePair.second;
        if (entry->getNode()->getNumChildren() == 0) {
            _pager->queueRequest(entry->tileFileName,
                                 entry->getNode(),
                                 entry->get_inner_ring() ? 10.0f : 1.0f,
                                 _framestamp,
                                 entry->getDatabaseRequest(),
                                 _options);
        }
    }
private:
    SceneryPager* _pager;
    osg::FrameStamp* _framestamp;
    osgDB::ReaderWriter::Options* _options;
};

/**
 * Update the various queues maintained by the tilemagr (private
 * internal function, do not call directly.)
 */
void FGTileMgr::update_queues()
{
    SceneryPager* pager = FGScenery::getPagerSingleton();
    osg::FrameStamp* framestamp
        = globals->get_renderer()->getViewer()->getFrameStamp();
    tile_cache.set_current_time(framestamp->getReferenceTime());
    for_each(tile_cache.begin(), tile_cache.end(),
             TileLoad(pager,
                      framestamp,
                      globals->get_scenery()->get_terrain_branch(), _options.get()));
}


// given the current lon/lat (in degrees), fill in the array of local
// chunks.  If the chunk isn't already in the cache, then read it from
// disk.
int FGTileMgr::update( double visibility_meters )
{
    SGVec3d viewPos = globals->get_current_view()->get_view_pos();
    return update(SGGeod::fromCart(viewPos), visibility_meters);
}

int FGTileMgr::update( const SGGeod& location, double visibility_meters)
{
    SG_LOG( SG_TERRAIN, SG_DEBUG, "FGTileMgr::update()" );

    longitude = location.getLongitudeDeg();
    latitude = location.getLatitudeDeg();

    // SG_LOG( SG_TERRAIN, SG_DEBUG, "FGTileMgr::update() for "
    //         << longitude << " " << latatitude );

    current_bucket.set_bucket( location );
    // SG_LOG( SG_TERRAIN, SG_DEBUG, "Updating tile list for "
    //         << current_bucket );
    fgSetInt( "/environment/current-tile-id", current_bucket.gen_index() );

    // do tile load scheduling. 
    // Note that we need keep track of both viewer buckets and fdm buckets.
    if ( state == Running ) {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "State == Running" );
        if (current_bucket != previous_bucket) {
            // We've moved to a new bucket, we need to schedule any
            // needed tiles for loading.
            SG_LOG( SG_TERRAIN, SG_INFO, "FGTileMgr::update()" );
            schedule_needed(visibility_meters, current_bucket);
        }
    } else if ( state == Start || state == Inited ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "State == Start || Inited" );
//        initialize_queue();
        state = Running;
        if (current_bucket != previous_bucket
            && current_bucket.get_chunk_lon() != -1000) {
               SG_LOG( SG_TERRAIN, SG_INFO, "FGTileMgr::update()" );
               schedule_needed(visibility_meters, current_bucket);
        }
    }

    update_queues();

    // save bucket...
    previous_bucket = current_bucket;

    return 1;
}

void FGTileMgr::prep_ssg_nodes(float vis) {

    // traverse the potentially viewable tile list and update range
    // selector and transform

    TileEntry *e;
    tile_cache.reset_traversal();

    while ( ! tile_cache.at_end() ) {
        // cout << "processing a tile" << endl;
        if ( (e = tile_cache.get_current()) ) {
            e->prep_ssg_node(vis);
        } else {
            SG_LOG(SG_INPUT, SG_ALERT, "warning ... empty tile in cache");
        }
        tile_cache.next();
    }
}

bool FGTileMgr::scenery_available(const SGGeod& position, double range_m)
{
  // sanity check (unfortunately needed!)
  if (position.getLongitudeDeg() < -180 || position.getLongitudeDeg() > 180 ||
      position.getLatitudeDeg() < -90 || position.getLatitudeDeg() > 90)
    return false;
  
  SGBucket bucket(position);
  TileEntry *te = tile_cache.get_tile(bucket);
  if (!te || !te->is_loaded())
    return false;

  SGVec3d cartPos = SGVec3d::fromGeod(position);

  // Traverse all tiles required to be there for the given visibility.
  // This uses exactly the same algorithm like the tile scheduler.
  double tile_width = bucket.get_width_m();
  double tile_height = bucket.get_height_m();
  double tile_r = 0.5*sqrt(tile_width*tile_width + tile_height*tile_height);
  double max_dist = tile_r + range_m;
  double max_dist2 = max_dist*max_dist;
  
  int xrange = (int)fabs(range_m / tile_width) + 1;
  int yrange = (int)fabs(range_m / tile_height) + 1;
  
  for ( int x = -xrange; x <= xrange; ++x ) {
    for ( int y = -yrange; y <= yrange; ++y ) {
      // We have already checked for the center tile.
      if ( x != 0 || y != 0 ) {
        SGBucket b = sgBucketOffset( position.getLongitudeDeg(),
                                     position.getLatitudeDeg(), x, y );
        // Do not ask if it is just the next tile but way out of range.
        if (max_dist2 < distSqr(cartPos, SGVec3d::fromGeod(b.get_center())))
          continue;
        TileEntry *te = tile_cache.get_tile(b);
        if (!te || !te->is_loaded())
          return false;
      }
    }
  }

  // Survived all tests.
  return true;
}

namespace
{
struct IsTileLoaded :
        public std::unary_function<TileCache::tile_map::value_type, bool>
{
    bool operator()(const TileCache::tile_map::value_type& tilePair) const
    {
        return tilePair.second->is_loaded();
    }
};
}

bool FGTileMgr::isSceneryLoaded()
{
    return (std::find_if(tile_cache.begin(), tile_cache.end(),
                         std::not1(IsTileLoaded()))
            == tile_cache.end());
}
