// tileentry.hxx -- routines to handle an individual scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998, 1999  Curtis L. Olson  - curt@flightgear.org
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
#include <simgear/xgl/xgl.h>

#include <simgear/compiler.h>

#include <vector>
#include STL_STRING

#include <plib/ssg.h>		// plib includes

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/point3d.hxx>

#if defined( sgi )
#include <strings.h>
#endif

SG_USING_STD(string);
SG_USING_STD(vector);


typedef vector < Point3D > point_list;
typedef point_list::iterator point_list_iterator;
typedef point_list::const_iterator const_point_list_iterator;


/**
 * A class to encapsulate everything we need to know about a scenery tile.
 */
class FGTileEntry {

public:

    typedef vector < sgVec3 * > free_vec3_list;
    typedef vector < sgVec2 * > free_vec2_list;
    typedef vector < unsigned short * > free_index_list;

    // node list
    point_list nodes;
    int ncount;

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
    ssgTransform *lights_transform;

    // pointer to ssg range selector for this tile
    ssgRangeSelector *terra_range;
    ssgRangeSelector *lights_range;

    // we create several preset brightness and can choose which one we
    // want based on lighting conditions.
    ssgSelector *lights_brightness;

    /**
     * Indicates this tile has been loaded from a file.
     * Note that this may be set asynchronously by another thread.
     */
    volatile bool loaded;

    ssgBranch* obj_load( const std::string& path,
			 ssgVertexArray* lights, bool is_base );

    ssgLeaf* gen_lights( ssgVertexArray *lights, int inc, float bright );

public:

    // Constructor
    FGTileEntry( const SGBucket& b );

    // Destructor
    ~FGTileEntry();

    // Clean up the memory used by this tile and delete the arrays
    // used by ssg as well as the whole ssg branch
    void free_tile();

    // Calculate this tile's offset
    void SetOffset( const Point3D& p)
    {
	offset = center - p;
    }

    // Return this tile's offset
    inline Point3D get_offset() const { return offset; }

    // Update the ssg transform node for this tile so it can be
    // properly drawn relative to our (0,0,0) point
    void prep_ssg_node( const Point3D& p, float vis);

    /**
     * Load tile data from a file.
     * @param base name of directory containing tile data file.
     * @param is_base is this a base terrain object for which we should generate
     *        random ground light points
     */
    void load( const SGPath& base, bool is_base );

    /**
     * Return true if the tile entry is loaded, otherwise return false
     * indicating that the loading thread is still working on this.
     */
    inline bool is_loaded() const { return loaded; }

    /**
     * Return the "bucket" for this tile
     */
    inline SGBucket get_tile_bucket() const { return tile_bucket; }

};


#endif // _TILEENTRY_HXX 
