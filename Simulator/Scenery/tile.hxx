// tile.hxx -- routines to handle a scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@infoplane.com
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


#ifndef _TILE_HXX
#define _TILE_HXX


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

#include <Bucket/newbucket.hxx>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Objects/fragment.hxx>

#ifdef FG_HAVE_NATIVE_SGI_COMPILERS
#include <strings.h>
#endif

FG_USING_STD(string);
FG_USING_STD(vector);


// Scenery tile class
class fgTILE {

public:

    typedef vector < fgFRAGMENT > container;
    typedef container::iterator FragmentIterator;
    typedef container::const_iterator FragmentConstIterator;

public:
    // node list (the per fragment face lists reference this node list)
    double (*nodes)[3];
    int ncount;

    // culling data for whole tile (course grain culling)
    Point3D center;
    double bounding_radius;
    Point3D offset;
    GLfloat model_view[16];

    // this tile's official location in the world
    FGBucket tile_bucket;

    // the tile cache will mark here if the tile is being used
    bool used;

    container fragment_list;

public:

    FragmentIterator begin() { return fragment_list.begin(); }
    FragmentConstIterator begin() const { return fragment_list.begin(); }

    FragmentIterator end() { return fragment_list.end(); }
    FragmentConstIterator end() const { return fragment_list.end(); }

    void add_fragment( fgFRAGMENT& frag ) {
 	frag.tile_ptr = this;
	fragment_list.push_back( frag );
    }

    //
    size_t num_fragments() const {
	return fragment_list.size();
    }

    // Step through the fragment list, deleting the display list, then
    // the fragment, until the list is empty.
    void release_fragments();

//     int ObjLoad( const string& path, const fgBUCKET& p );

    // Constructor
    fgTILE ( void );

    // Destructor
    ~fgTILE ( void );

    // Calculate this tile's offset
    void SetOffset( const Point3D& off)
    {
	offset = center - off;
    }

    // Return this tile's offset
    inline Point3D GetOffset( void ) const { return offset; }

    // Calculate the model_view transformation matrix for this tile
    inline void
    UpdateViewMatrix(GLfloat *MODEL_VIEW)
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

private:

    // not defined
    fgTILE( const fgTILE& );
    fgTILE& operator = ( const fgTILE& );
};


#endif // _TILE_HXX 
