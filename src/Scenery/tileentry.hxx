// tileentry.hxx -- routines to handle an individual scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2001  Curtis L. Olson  - curt@flightgear.org
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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <GL/gl.h>

#include <simgear/compiler.h>

#include <vector>
#include STL_STRING

#include <plib/ssg.h>		// plib includes

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

public:

    inline FGDeferredModel() { }
    inline FGDeferredModel( const string mp, const string tp,
		     FGTileEntry *t, ssgTransform *ot )
    {
	model_path = mp;
	texture_path = tp;
	tile = t;
	obj_trans = ot;
    }
    inline ~FGDeferredModel() { }
    inline string get_model_path() const { return model_path; }
    inline string get_texture_path() const { return texture_path; }
    inline FGTileEntry *get_tile() const { return tile; }
    inline ssgTransform *get_obj_trans() const { return obj_trans; }
};


/**
 * A class to encapsulate everything we need to know about a scenery tile.
 */
class FGTileEntry {

public:

    /* CLO123 FROM HERE TO THE CORRESPONDING MARKER ARE THINGS THAT
       CAN BE DELETED AFTER WE DROP THE ASCII SCENERY FORMAT */
    typedef vector < sgVec3 * > free_vec3_list;
    typedef vector < sgVec2 * > free_vec2_list;
    typedef vector < unsigned short * > free_index_list;

    // node list
    point_list nodes;
    int ncount;
    /* CLO123 MARKER */

    // global tile culling data
    Point3D center;
    double bounding_radius;
    Point3D offset;

    // this tile's official location in the world
    SGBucket tile_bucket;

    // list of pointers to memory chunks that need to be freed when
    // tile entry goes away
    free_vec3_list vec3_ptrs;
    free_vec2_list vec2_ptrs;
    free_index_list index_ptrs;

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
    ssgTransform *terra_transform;
    ssgTransform *rwy_lights_transform;
    ssgTransform *gnd_lights_transform;

    // pointer to ssg range selector for this tile
    ssgRangeSelector *terra_range;
    ssgRangeSelector *gnd_lights_range;

    // we create several preset brightness and can choose which one we
    // want based on lighting conditions.
    ssgSelector *gnd_lights_brightness;

    // ADA --->
    // Runway centre coords.
    Point3D rway_center; // Reqd. for switching as function of distance from runway center
    //pointers for Runway lights    
    ssgTransform *lightmaps_transform; // branch for runway lights
    ssgRangeSelector *lightmaps_range; // tried to maintain same structure as Curt's ground
    ssgSelector *lightmaps_brightness; // selector node for points/ lightmaps

    ssgSimpleState *lightmap_state; // OpenGL state of the polygons that make up runway lights
    ssgVertexArray *light_points; // array of runway light coords.
    // <--- ADA

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

    bool obj_load( const std::string& path,
			 ssgBranch* geometry,
			 ssgBranch* rwy_lights,
			 ssgVertexArray* gound_lights,
			 bool is_base );

    ssgLeaf* gen_lights( ssgVertexArray *lights, int inc, float bright );

    double timestamp;


    // this variable tracks the status of the incremental memory freeing.
    enum {
        NODES = 0x01,
        VEC_PTRS = 0x02,
        TERRA_NODE = 0x04,
        GROUND_LIGHTS = 0x08,
        RWY_LIGHTS = 0x10,
        LIGHTMAPS = 0x20
    };
    int free_tracker;

public:

    // ADA --->
    ssgTransform *ols_transform; // transform node for flols simulation
    ssgSelector *lightmaps_sequence; // selector node for points/ lightmaps
    ssgBranch* gen_runway_lights(ssgVertexArray *points,ssgVertexArray *normal, ssgVertexArray *dir, int *type);
    // <--- ADA

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

    // Calculate this tile's offset
    void SetOffset( const Point3D& p)
    {
	offset = center - p;
    }

    // Return this tile's offset
    inline Point3D get_offset() const { return offset; }

    // Update the ssg transform node for this tile so it can be
    // properly drawn relative to our (0,0,0) point
    void prep_ssg_node( const Point3D& p, sgVec3 up, float vis);

    /**
     * Load tile data from a file.
     * @param base name of directory containing tile data file.
     * @param is_base is this a base terrain object for which we should generate
     *        random ground light points */
    void load( const SGPath& base, bool is_base );

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
    inline SGBucket get_tile_bucket() const { return tile_bucket; }

    /**
     * Add terrain mesh and ground lighting to scene graph.
     */
    void add_ssg_nodes( ssgBranch* terrain_branch,
			ssgBranch* gnd_lights_branch,
			ssgBranch* rwy_lights_branch );

    /**
     * disconnect terrain mesh and ground lighting nodes from scene
     * graph for this tile.
     */
    void disconnect_ssg_nodes();

	
    /**
     * return the SSG Transform node for the terrain
     */
    inline ssgTransform *get_terra_transform() { return terra_transform; }

    void set_timestamp(double time_ms) { timestamp = time_ms; }

    inline double get_timestamp() const { return timestamp; }

};


#endif // _TILEENTRY_HXX 
