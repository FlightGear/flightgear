// tileentry.hxx -- routines to handle an individual scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _TILEENTRY_HXX
#define _TILEENTRY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <vector>
#include STL_STRING

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/misc/sg_path.hxx>

#if defined( sgi )
#include <strings.h>
#endif

SG_USING_STD(string);
SG_USING_STD(vector);


typedef vector < Point3D > point_list;
typedef point_list::iterator point_list_iterator;
typedef point_list::const_iterator const_point_list_iterator;


class ssgLeaf;
class ssgBranch;
class ssgTransform;
class ssgPlacementTransform;
class ssgSelector;
class ssgRangeSelector;
class ssgVertexArray;
class FGTileEntry;


/**
 * A class to hold deferred model loading info
 */
class FGDeferredModel {

private:

    string model_path;
    string texture_path;
    FGTileEntry *tile;
    ssgTransform *obj_trans;
    SGBucket bucket;


public:

    inline FGDeferredModel() { }
    inline FGDeferredModel( const string& mp, const string& tp, SGBucket b,
		     FGTileEntry *t, ssgTransform *ot )
    {
	model_path = mp;
	texture_path = tp;
        bucket = b;
	tile = t;
	obj_trans = ot;
    }
    inline ~FGDeferredModel() { }
    inline const string& get_model_path() const { return model_path; }
    inline const string& get_texture_path() const { return texture_path; }
    inline const SGBucket& get_bucket() const { return bucket; }
    inline FGTileEntry *get_tile() const { return tile; }
    inline ssgTransform *get_obj_trans() const { return obj_trans; }
};


/**
 * A class to encapsulate everything we need to know about a scenery tile.
 */
class FGTileEntry {

public:

    // global tile culling data
    Point3D center;
    double bounding_radius;

    // this tile's official location in the world
    SGBucket tile_bucket;

private:

    // ssg tree structure for this tile is as follows:
    // ssgRoot(scene)
    //     - ssgBranch(terrain)
    //        - ssgTransform(tile)
    //           - ssgRangeSelector(tile)
    //              - ssgEntity(tile)
    //                 - kid1(fan)
    //                 - kid2(fan)
    //                   ...
    //                 - kidn(fan)

    // pointer to ssg transform for this tile
    ssgPlacementTransform *terra_transform;
    ssgPlacementTransform *vasi_lights_transform;
    ssgPlacementTransform *rwy_lights_transform;
    ssgPlacementTransform *taxi_lights_transform;
    ssgPlacementTransform *gnd_lights_transform;

    // pointer to ssg range selector for this tile
    ssgRangeSelector *terra_range;
    ssgRangeSelector *gnd_lights_range;

    // we create several preset brightness and can choose which one we
    // want based on lighting conditions.
    ssgSelector *gnd_lights_brightness;

    // we need to be able to turn runway lights on or off (doing this
    // via a call back would be nifty, but then the call back needs to
    // know about the higher level application's global state which is
    // a problem if we move the code into simgear.)
    ssgSelector *vasi_lights_selector;
    ssgSelector *rwy_lights_selector;
    ssgSelector *taxi_lights_selector;

    /**
     * Indicates this tile has been loaded from a file and connected
     * into the scene graph.  Note that this may be set asynchronously
     * by another thread.
     */
    volatile bool loaded;

    /**
     * Count of pending models to load for this tile.  This tile
     * cannot be removed until this number reaches zero (i.e. no
     * pending models to load for this tile.)
     */
    volatile int pending_models;

    bool obj_load( const string& path,
                   ssgBranch* geometry,
                   ssgBranch* vasi_lights,
                   ssgBranch* rwy_lights,
                   ssgBranch* taxi_lights,
                   ssgVertexArray* gound_lights,
                   bool is_base );

    ssgLeaf* gen_lights( SGMaterialLib *matlib, ssgVertexArray *lights,
                         int inc, float bright );

    double timestamp;

    /**
     * this value is used by the tile scheduler/loader to mark which
     * tiles are in the primary ring (i.e. the current tile or the
     * surrounding eight.)  Other routines then can use this as an
     * optimization and not do some operation to tiles outside of this
     * inner ring.  (For instance vasi color updating)
     */
    bool is_inner_ring;

    /**
     * this variable tracks the status of the incremental memory
     * freeing.
     */
    enum {
        NODES = 0x01,
        VEC_PTRS = 0x02,
        TERRA_NODE = 0x04,
        GROUND_LIGHTS = 0x08,
        VASI_LIGHTS = 0x10,
        RWY_LIGHTS = 0x20,
        TAXI_LIGHTS = 0x40,
        LIGHTMAPS = 0x80
    };
    int free_tracker;

public:

    // Constructor
    FGTileEntry( const SGBucket& b );

    // Destructor
    ~FGTileEntry();

    // Clean up the memory used by this tile and delete the arrays
    // used by ssg as well as the whole ssg branch.  This does a
    // partial clean up and exits so we can spread the load across
    // multiple frames.  Returns false if work remaining to be done,
    // true if dynamically allocated memory used by this tile is
    // completely freed.
    bool free_tile();

    // Update the ssg transform node for this tile so it can be
    // properly drawn relative to our (0,0,0) point
    void prep_ssg_node( const Point3D& p, sgVec3 up, float vis);

    /**
     * Load tile data from a file.
     * @param base name of directory containing tile data file.
     * @param is_base is this a base terrain object for which we should generate
     *        random ground light points */
    void load( const string_list &base_path, bool is_base );

    /**
     * Return true if the tile entry is loaded, otherwise return false
     * indicating that the loading thread is still working on this.
     */
    inline bool is_loaded() const { return loaded; }

    /**
     * decrement the pending models count
     */
    inline void dec_pending_models() { pending_models--; }

    /**
     * return the number of remaining pending models for this tile
     */
    inline int get_pending_models() const { return pending_models; }

    /**
     * Return the "bucket" for this tile
     */
    inline const SGBucket& get_tile_bucket() const { return tile_bucket; }

    /**
     * Apply ssgLeaf::makeDList to all leaf of a branch
     */
    void makeDList( ssgBranch *b );

    /**
     * Add terrain mesh and ground lighting to scene graph.
     */
    void add_ssg_nodes( ssgBranch *terrain_branch,
			ssgBranch *gnd_lights_branch,
                        ssgBranch *vasi_lights_branch,
			ssgBranch *rwy_lights_branch,
			ssgBranch *taxi_lights_branch );

    /**
     * disconnect terrain mesh and ground lighting nodes from scene
     * graph for this tile.
     */
    void disconnect_ssg_nodes();

	
    /**
     * return the SSG Transform node for the terrain
     */
    inline ssgPlacementTransform *get_terra_transform() const { return terra_transform; }

    inline double get_timestamp() const { return timestamp; }
    inline void set_timestamp( double time_ms ) { timestamp = time_ms; }

    inline bool get_inner_ring() const { return is_inner_ring; }
    inline void set_inner_ring( bool val ) { is_inner_ring = val; }
};


#endif // _TILEENTRY_HXX 
