// tilemgr.hxx -- routines to handle dynamic management of scenery tiles
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


#ifndef _TILEMGR_HXX
#define _TILEMGR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>

#include <plib/ssg.h>

#include <simgear/bucket/newbucket.hxx>

#include "FGTileLoader.hxx"
#include "hitlist.hxx"
#include "newcache.hxx"

#if defined(USE_MEM) || defined(WIN32)
#  define FG_MEM_COPY(to,from,n)        memcpy(to, from, n)
#else
#  define FG_MEM_COPY(to,from,n)        bcopy(from, to, n)
#endif


// forward declaration
class FGTileEntry;


class FGTileMgr {

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

    // forced emptying of the queue.  This is necessay to keep
    // bookeeping straight for the tile_cache -- which actually
    // handles all the (de)allocations
    void destroy_queue();

    // schedule a tile for loading
    void sched_tile( const SGBucket& b );

    // schedule a needed buckets for loading
    void schedule_needed();

    // see comment at prep_ssg_nodes()
    void prep_ssg_node( int idx );
	
    // int hitcount;
    // sgdVec3 hit_pts [ MAX_HITS ] ;

    // ssgEntity *last_hit;
    FGHitList hit_list;

    SGBucket previous_bucket;
    SGBucket current_bucket;
    SGBucket pending;
	
    FGTileEntry *current_tile;
	
    // x and y distance of tiles to load/draw
    float vis;
    int xrange, yrange;
	
    // current longitude latitude
    double longitude;
    double latitude;
    double last_longitude;
    double last_latitude;

    /**
     * tile cache
     */
    FGNewCache tile_cache;

    /**
     * Queue tiles for loading.
     */
    FGTileLoader loader;
    int counter_hack;

public:

    // Constructor
    FGTileMgr();

    // Destructor
    ~FGTileMgr();

    // Initialize the Tile Manager subsystem
    int init();

    // given the current lon/lat (in degrees), fill in the array of
    // local chunks.  If the chunk isn't already in the cache, then
    // read it from disk.
    int update( double lon, double lat );

    // Determine scenery altitude.  Normally this just happens when we
    // render the scene, but we'd also like to be able to do this
    // explicitely.  lat & lon are in radians.  abs_view_pos in
    // meters.  Returns result in meters.
    void my_ssg_los( string s, ssgBranch *branch, sgdMat4 m, 
		     const sgdVec3 p, const sgdVec3 dir, sgdVec3 normal );
	
    void my_ssg_los( ssgBranch *branch, sgdMat4 m, 
		     const sgdVec3 p, const sgdVec3 dir,
		     FGHitList *list );

    bool current_elev_ssg( sgdVec3 abs_view_pos, double *terrain_elev );
	
    // Prepare the ssg nodes ... for each tile, set it's proper
    // transform and update it's range selector based on current
    // visibilty
    void prep_ssg_nodes();
};


// the tile manager
extern FGTileMgr global_tile_mgr;


#endif // _TILEMGR_HXX
