// fragment.hxx -- routines to handle "atomic" display objects
//
// Written by Curtis Olson, started August 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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


#ifndef _FRAGMENT_HXX
#define _FRAGMENT_HXX


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
#include <simgear/xgl.h>

#include <simgear/compiler.h>

#include <vector>

#include <simgear/constants.h>
#include <simgear/point3d.hxx>

FG_USING_STD(vector);


struct fgFACE {
    int n1, n2, n3;

    fgFACE( int a = 0, int b =0, int c =0 )
	: n1(a), n2(b), n3(c) {}

    fgFACE( const fgFACE & image )
	: n1(image.n1), n2(image.n2), n3(image.n3) {}

    fgFACE& operator= ( const fgFACE & image ) {
	n1 = image.n1; n2 = image.n2; n3 = image.n3; return *this;
    }

    ~fgFACE() {}
};

inline bool
operator== ( const fgFACE& lhs, const fgFACE& rhs )
{
    return (lhs.n1 == rhs.n1) && (lhs.n2 == rhs.n2) && (lhs.n3 == rhs.n3);
}

// Forward declarations
class FGTileEntry;
class FGMaterialSlot;

// Object fragment data class
class fgFRAGMENT {

private:

public:
    // culling data for this object fragment (fine grain culling)
    Point3D center;
    double bounding_radius;

    // variable offset data for this object fragment for this frame
    // fgCartesianPoint3d tile_offset;

    // saved transformation matrix for this fragment (used by renderer)
    // GLfloat matrix[16];
    
    // tile_ptr & material_ptr are set so that when we traverse the
    // list of fragments we can quickly reference back the tile or
    // material property this fragment is assigned to.

    // material property pointer
    FGMaterialSlot *material_ptr;

    // tile pointer
    FGTileEntry *tile_ptr;

    // OpenGL display list for fragment data
    // GLint display_list;

    // face list (this indexes into the master tile vertex list)
    typedef vector < fgFACE > container;
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

    container faces;

public:

    // number of faces in this fragment
    int num_faces() {
	return faces.size();
    }

    // Add a face to the face list
    void add_face(int n1, int n2, int n3) {
	faces.push_back( fgFACE(n1,n2,n3) );
    }

    // test if line intesects with this fragment.  p0 and p1 are the
    // two line end points of the line.  If side_flag is true, check
    // to see that end points are on opposite sides of face.  Returns
    // 1 if it intersection found, 0 otherwise.  If it intesects,
    // result is the point of intersection
    int intersect( const Point3D& end0,
		   const Point3D& end1,
		   int side_flag,
		   Point3D& result) const;

    // Constructors
    fgFRAGMENT () { /*faces.reserve(512);*/}
    fgFRAGMENT ( const fgFRAGMENT &image );

    // Destructor
    ~fgFRAGMENT() { faces.erase( faces.begin(), faces.end() ); }

    // operators
    fgFRAGMENT & operator = ( const fgFRAGMENT & rhs );

    bool operator <  ( const fgFRAGMENT & rhs ) const {
	// This is completely arbitrary. It satisfies RW's STL implementation
	return bounding_radius < rhs.bounding_radius;
    }

    void init() {
	faces.clear();
    }

    // int deleteDisplayList() {
    //    xglDeleteLists( display_list, 1 ); return 0;
    // }

    friend bool operator== ( const fgFRAGMENT & lhs, const fgFRAGMENT & rhs );
};

inline bool
operator == ( const fgFRAGMENT & lhs, const fgFRAGMENT & rhs ) {
    return lhs.center == rhs.center;
}


#endif // _FRAGMENT_HXX 


