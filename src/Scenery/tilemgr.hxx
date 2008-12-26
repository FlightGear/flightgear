// tilemgr.hxx -- routines to handle dynamic management of scenery tiles
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


#ifndef _TILEMGR_HXX
#define _TILEMGR_HXX

#include <simgear/compiler.h>
#include <simgear/scene/model/location.hxx>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/scene/tgdb/TileEntry.hxx>
#include <simgear/scene/tgdb/TileCache.hxx>

class SGReaderWriterBTGOptions;

namespace osg
{
class Node;
}

class FGTileMgr : public simgear::ModelLoadHelper {

private:

    // Tile loading state
    enum load_state {
	Start = 0,
	Inited = 1,
	Running = 2
    };

    load_state state;

    // initialize the cache
    void initialize_queue();

    // schedule a tile for loading
    void sched_tile( const SGBucket& b, const bool is_inner_ring );

    // schedule a needed buckets for loading
    void schedule_needed(double visibility_meters, const SGBucket& curr_bucket);

    SGBucket previous_bucket;
    SGBucket current_bucket;
    SGBucket pending;
    osg::ref_ptr<SGReaderWriterBTGOptions> _options;

    // x and y distance of tiles to load/draw
    float vis;
    int xrange, yrange;

    // current longitude latitude
    double longitude;
    double latitude;
    double altitude_m;

    /**
     * tile cache
     */
    simgear::TileCache tile_cache;

public:
    FGTileMgr();

    ~FGTileMgr();

    // Initialize the Tile Manager
    int init();

    // Update the various queues maintained by the tilemagr (private
    // internal function, do not call directly.)
    void update_queues();

    // given the current lon/lat (in degrees), fill in the array of
    // local chunks.  If the chunk isn't already in the cache, then
    // read it from disk.
    int update( double visibility_meters );
    int update( SGLocation *location, double visibility_meters);

    // Prepare the ssg nodes corresponding to each tile.  For each
    // tile, set the ssg transform and update it's range selector
    // based on current visibilty void prep_ssg_nodes( float
    // visibility_meters );
    void prep_ssg_nodes(float visibility_meters );

    const SGBucket& get_current_bucket () const { return current_bucket; }

    /// Returns true if scenery is avaliable for the given lat, lon position
    /// within a range of range_m.
    /// lat and lon are expected to be in degrees.
    bool scenery_available(double lat, double lon, double range_m);

    // Load a model for a tile
    osg::Node* loadTileModel(const string& modelPath, bool cacheModel);

    // Returns true if all the tiles in the tile cache have been loaded
    bool isSceneryLoaded();
};


#endif // _TILEMGR_HXX
