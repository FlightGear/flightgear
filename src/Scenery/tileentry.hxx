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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include <simgear/scene/model/placementtrans.hxx>

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/Group>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/Switch>

#if defined( sgi )
#include <strings.h>
#endif

SG_USING_STD(string);
SG_USING_STD(vector);


typedef vector < Point3D > point_list;
typedef point_list::iterator point_list_iterator;
typedef point_list::const_iterator const_point_list_iterator;


class FGTileEntry;

#if 0
/**
 * A class to hold deferred model loading info
 */
class FGDeferredModel {

private:

    string model_path;
    string texture_path;
    FGTileEntry *tile;
    osg::ref_ptr<osg::MatrixTransform> obj_trans;
    SGBucket bucket;
    bool cache_obj;


public:

    inline FGDeferredModel() { }
    inline FGDeferredModel( const string& mp, const string& tp, SGBucket b,
                            FGTileEntry *t, osg::MatrixTransform *ot, bool co )
    {
	model_path = mp;
	texture_path = tp;
        bucket = b;
	tile = t;
	obj_trans = ot;
	cache_obj = co;
    }
    inline ~FGDeferredModel() { }
    inline const string& get_model_path() const { return model_path; }
    inline const string& get_texture_path() const { return texture_path; }
    inline const SGBucket& get_bucket() const { return bucket; }
    inline const bool get_cache_state() const { return cache_obj; }
    inline FGTileEntry *get_tile() const { return tile; }
    inline osg::MatrixTransform *get_obj_trans() const { return obj_trans.get(); }
};
#endif

/**
 * A class to encapsulate everything we need to know about a scenery tile.
 */
class FGTileEntry {

public:
    // this tile's official location in the world
    SGBucket tile_bucket;
    std::string tileFileName;

private:

    // pointer to ssg range selector for this tile
    osg::ref_ptr<osg::LOD> _node;

    static bool obj_load( const std::string& path,
                          osg::Group* geometry,
                          bool is_base );

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
    void prep_ssg_node(float vis);

    /**
     * Transition to OSG database pager
     */
    static osg::Node* loadTileByName(const std::string& index_str,
                                     const string_list &path_list);
    /**
     * Return true if the tile entry is loaded, otherwise return false
     * indicating that the loading thread is still working on this.
     */
    inline bool is_loaded() const
    {
        return _node->getNumChildren() > 0;
    }

    /**
     * Return the "bucket" for this tile
     */
    inline const SGBucket& get_tile_bucket() const { return tile_bucket; }

    /**
     * Add terrain mesh and ground lighting to scene graph.
     */
    void add_ssg_nodes( osg::Group *terrain_branch);

    /**
     * disconnect terrain mesh and ground lighting nodes from scene
     * graph for this tile.
     */
    void disconnect_ssg_nodes();

	
    /**
     * return the scenegraph node for the terrain
     */
    osg::LOD *getNode() const { return _node.get(); }

    double get_timestamp() const;
    void set_timestamp( double time_ms );

    inline bool get_inner_ring() const { return is_inner_ring; }
    inline void set_inner_ring( bool val ) { is_inner_ring = val; }
};


#endif // _TILEENTRY_HXX 
