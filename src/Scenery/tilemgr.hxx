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

#include <list>

#include <plib/ssg.h>

#include <simgear/bucket/newbucket.hxx>

#include "hitlist.hxx"

FG_USING_STD(list);


#define FG_LOCAL_X_Y         81  // max(o->tile_diameter) ** 2

#define FG_SQUARE( X ) ( (X) * (X) )

#if defined(USE_MEM) || defined(WIN32)
#  define FG_MEM_COPY(to,from,n)        memcpy(to, from, n)
#else
#  define FG_MEM_COPY(to,from,n)        bcopy(from, to, n)
#endif


// forward declaration
class FGTileEntry;


class FGLoadRec {

public:

    FGBucket b;
    int cache_index;
};


class FGTileMgr {

private:

    // Tile loading state
    enum load_state {
	Start = 0,
	Inited = 1,
	Running = 2
    };

    load_state state;

    enum SCROLL_DIRECTION {
	SCROLL_INIT = -1,
	SCROLL_NONE = 0,
	SCROLL_NORTH,
	SCROLL_EAST,
	SCROLL_SOUTH,
	SCROLL_WEST,
    };

    SCROLL_DIRECTION scroll_direction;

    // pending tile load queue
    list < FGLoadRec > load_queue;

    // initialize the cache
    void initialize_queue();

    // forced emptying of the queue.  This is necessay to keep
    // bookeeping straight for the tile_cache -- which actually
    // handles all the (de)allocations
    void destroy_queue();

    FGBucket BucketOffset( int dx, int dy );

    // schedule a tile for loading
    int sched_tile( const FGBucket& b );

    // load a tile
    void load_tile( const FGBucket& b, int cache_index );

    // schedule a tile row(column) for loading
    void scroll( void );

    // see comment at prep_ssg_nodes()
    void prep_ssg_node( int idx );
	
    // int hitcount;
    // sgdVec3 hit_pts [ MAX_HITS ] ;

    // ssgEntity *last_hit;
    FGHitList hit_list;

    FGBucket previous_bucket;
    FGBucket current_bucket;
    FGBucket pending;
	
    FGTileEntry *current_tile;
	
    // index of current tile in tile cache;
    long int tile_index;
    int tile_diameter;
	
    // current longitude latitude
    double longitude;
    double latitude;
    double last_longitude;
    double last_latitude;

public:

    // Constructor
    FGTileMgr ( void );

    // Destructor
    ~FGTileMgr ( void );

    // Initialize the Tile Manager subsystem
    int init( void );

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

    bool current_elev_ssg( const Point3D& abs_view_pos, 
			   const Point3D& view_pos );
	
    // Prepare the ssg nodes ... for each tile, set it's proper
    // transform and update it's range selector based on current
    // visibilty
    void prep_ssg_nodes( void );

    inline int queue_size() const { return load_queue.size(); }
};


// the tile manager
extern FGTileMgr global_tile_mgr;


#endif // _TILEMGR_HXX
