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
// (Log is kept at end of this file)


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
#include <XGL/xgl.h>

#if defined ( __sun__ )
extern "C" void *memmove(void *, const void *, size_t);
extern "C" void *memset(void *, int, size_t);
#endif

#include <list>         // STL list

#include <Bucket/bucketutils.h>
#include <Include/fg_types.h>
#include <Math/mat3.h>

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif


// Maximum nodes per tile
#define MAX_NODES 2000


// Forward declarations
class fgTILE;
class fgMATERIAL;


class fgFACE {
public:
    int n1, n2, n3;

    fgFACE();
    ~fgFACE();
    fgFACE( const fgFACE & image );
    bool operator < ( const fgFACE & rhs );
    bool operator == ( const fgFACE & rhs );
};


// Object fragment data class
class fgFRAGMENT {

public:
    // culling data for this object fragment (fine grain culling)
    fgPoint3d center;
    double bounding_radius;

    // variable offset data for this object fragment for this frame
    // fgCartesianPoint3d tile_offset;

    // saved transformation matrix for this fragment (used by renderer)
    // GLfloat matrix[16];
    
    // tile_ptr & material_ptr are set so that when we traverse the
    // list of fragments we can quickly reference back the tile or
    // material property this fragment is assigned to.

    // material property pointer
    fgMATERIAL *material_ptr;

    // tile pointer
    fgTILE *tile_ptr;

    // OpenGL display list for fragment data
    GLint display_list;

    // face list (this indexes into the master tile vertex list)
    list < fgFACE > faces;

    // number of faces in this fragment
    int num_faces;

    // Add a face to the face list
    void add_face(int n1, int n2, int n3);

    // test if line intesects with this fragment.  p0 and p1 are the
    // two line end points of the line.  If side_flag is true, check
    // to see that end points are on opposite sides of face.  Returns
    // 1 if it intersection found, 0 otherwise.  If it intesects,
    // result is the point of intersection
    int intersect( fgPoint3d *end0, fgPoint3d *end1, int side_flag,
		   fgPoint3d *result);

    // Constructors
    fgFRAGMENT ();
    fgFRAGMENT ( const fgFRAGMENT &image );

    // Destructor
    ~fgFRAGMENT ( );

    // operators
    fgFRAGMENT & operator = ( const fgFRAGMENT & rhs );
    bool operator == ( const fgFRAGMENT & rhs );
    bool operator <  ( const fgFRAGMENT & rhs );
};


#endif // _FRAGMENT_HXX 


// $Log$
// Revision 1.1  1998/08/25 16:51:23  curt
// Moved from ../Scenery
//
//
