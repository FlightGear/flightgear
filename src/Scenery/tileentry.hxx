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
#include <XGL/xgl.h>

#include <Include/compiler.h>

#include <vector>
#include STL_STRING

#include <ssg.h>		// plib includes

#include <Bucket/newbucket.hxx>
#include <Math/point3d.hxx>
#include <Objects/fragment.hxx>

#ifdef FG_HAVE_NATIVE_SGI_COMPILERS
#include <strings.h>
#endif

FG_USING_STD(string);
FG_USING_STD(vector);


typedef vector < Point3D > point_list;
typedef point_list::iterator point_list_iterator;
typedef point_list::const_iterator const_point_list_iterator;


// Scenery tile class
class FGTileEntry {

private:

    // Tile state
    enum tile_state {
	Unused = 0,
	Scheduled_for_use = 1,
	Scheduled_for_cache = 2,
	Loaded = 3,
	Cached = 4
    };

public:

    typedef vector < fgFRAGMENT > container;
    typedef container::iterator FragmentIterator;
    typedef container::const_iterator FragmentConstIterator;

    typedef vector < unsigned short * > free_list;

public:
    // node list (the per fragment face lists reference this node list)
    point_list nodes;
    int ncount;

    // global tile culling data
    Point3D center;
    double bounding_radius;
    Point3D offset;

    // model view matrix for this tile
    GLfloat model_view[16];

    // this tile's official location in the world
    FGBucket tile_bucket;

    // the tile cache will keep track here if the tile is being used
    tile_state state;

    container fragment_list;

    // ssg related structures
    sgVec3 *vtlist;
    sgVec3 *vnlist;
    sgVec2 *tclist;
    free_list free_ptrs; // list of pointers to free when tile
                                 // entry goes away

    // ssg tree structure for this tile is as follows:
    // ssgRoot(scene)
    //     - ssgBranch(terrain)
    //      - ssgSelector(tile)
    //        - ssgTransform(tile)
    //           - ssgRangeSelector(tile)
    //              - ssgEntity(tile)
    //                 - kid1(fan)
    //                 - kid2(fan)
    //                   ...
    //                 - kidn(fan)

    // selector (turn tile on/off)
    ssgSelector *select_ptr;

    // pointer to ssg transform for this tile
    ssgTransform *transform_ptr;

    // pointer to ssg range selector for this tile
    ssgRangeSelector *range_ptr;

public:

    // Constructor
    FGTileEntry ( void );

    // Destructor
    ~FGTileEntry ( void );

    FragmentIterator begin() { return fragment_list.begin(); }
    FragmentConstIterator begin() const { return fragment_list.begin(); }

    FragmentIterator end() { return fragment_list.end(); }
    FragmentConstIterator end() const { return fragment_list.end(); }

    void add_fragment( fgFRAGMENT& frag ) {
 	frag.tile_ptr = this;
	fragment_list.push_back( frag );
    }

    size_t num_fragments() const {
	return fragment_list.size();
    }

    // Step through the fragment list, deleting the display list, then
    // the fragment, until the list is empty.  Also delete the arrays
    // used by ssg as well as the whole ssg branch
    void free_tile();

    // Calculate this tile's offset
    void SetOffset( const Point3D& p)
    {
	offset = center - p;
    }

    // Return this tile's offset
    inline Point3D get_offset( void ) const { return offset; }

    // Calculate the model_view transformation matrix for this tile
    inline void update_view_matrix(GLfloat *MODEL_VIEW)
    {

#if defined( USE_MEM ) || defined( WIN32 )
	memcpy( model_view, MODEL_VIEW, 16*sizeof(GLfloat) );
#else 
	bcopy( MODEL_VIEW, model_view, 16*sizeof(GLfloat) );
#endif
	
	// This is equivalent to doing a glTranslatef(x, y, z);
	model_view[12] += (model_view[0]*offset.x() +
			   model_view[4]*offset.y() +
			   model_view[8]*offset.z());
	model_view[13] += (model_view[1]*offset.x() +
			   model_view[5]*offset.y() +
			   model_view[9]*offset.z());
	model_view[14] += (model_view[2]*offset.x() +
			   model_view[6]*offset.y() +
			   model_view[10]*offset.z() );
	// m[15] += (m[3]*x + m[7]*y + m[11]*z);
	// m[3] m7[] m[11] are 0.0 see LookAt() in views.cxx
	// so m[15] is unchanged
    }

    inline bool is_unused() const { return state == Unused; }
    inline bool is_scheduled_for_use() const { 
	return state == Scheduled_for_use;
    }
    inline bool is_scheduled_for_cache() const {
	return state == Scheduled_for_cache;
    }
    inline bool is_loaded() const { return state == Loaded; }
    inline bool is_cached() const { return state == Cached; }

    inline void mark_unused() { state = Unused; }
    inline void mark_scheduled_for_use() { state = Scheduled_for_use; }
    inline void mark_scheduled_for_cache() { state = Scheduled_for_use; }
    inline void mark_loaded() { state = Loaded; }


    // when a tile is still in the cache, but not in the immediate
    // draw l ist, it can still remain in the scene graph, but we use
    // a range selector to disable it from ever being drawn.
    void ssg_disable();
};


typedef vector < FGTileEntry > tile_list;
typedef tile_list::iterator tile_list_iterator;
typedef tile_list::const_iterator const_tile_list_iterator;


#endif // _TILEENTRY_HXX 
