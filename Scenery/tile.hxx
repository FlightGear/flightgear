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
// (Log is kept at end of this file)


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
    // 1 if it does, 0 otherwise.  If it intesects, result is the
    // point of intersection
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


// Scenery tile class
class fgTILE {

public:

    // node list (the per fragment face lists reference this node list)
    double (*nodes)[3];
    int ncount;

    // culling data for whole tile (course grain culling)
    fgPoint3d center;
    double bounding_radius;
    fgPoint3d offset;
    GLdouble model_view[16];

    // this tile's official location in the world
    fgBUCKET tile_bucket;

    // the tile cache will mark here if the tile is being used
    int used;

    list < fgFRAGMENT > fragment_list;

    // Constructor
    fgTILE ( void );

    // Destructor
    ~fgTILE ( void );
};


#endif // _TILE_HXX 


// $Log$
// Revision 1.16  1998/08/22 02:01:34  curt
// increased fragment list size.
//
// Revision 1.15  1998/08/20 15:12:06  curt
// Used a forward declaration of classes fgTILE and fgMATERIAL to eliminate
// the need for "void" pointers and casts.
// Quick hack to count the number of scenery polygons that are being drawn.
//
// Revision 1.14  1998/08/12 21:13:06  curt
// material.cxx: don't load textures if they are disabled
// obj.cxx: optimizations from Norman Vine
// tile.cxx: minor tweaks
// tile.hxx: addition of num_faces
// tilemgr.cxx: minor tweaks
//
// Revision 1.13  1998/07/24 21:42:08  curt
// material.cxx: whups, double method declaration with no definition.
// obj.cxx: tweaks to avoid errors in SGI's CC.
// tile.cxx: optimizations by Norman Vine.
// tilemgr.cxx: optimizations by Norman Vine.
//
// Revision 1.12  1998/07/22 21:41:42  curt
// Add basic fgFACE methods contributed by Charlie Hotchkiss.
// intersect optimization from Norman Vine.
//
// Revision 1.11  1998/07/12 03:18:28  curt
// Added ground collision detection.  This involved:
// - saving the entire vertex list for each tile with the tile records.
// - saving the face list for each fragment with the fragment records.
// - code to intersect the current vertical line with the proper face in
//   an efficient manner as possible.
// Fixed a bug where the tiles weren't being shifted to "near" (0,0,0)
//
// Revision 1.10  1998/07/08 14:47:22  curt
// Fix GL_MODULATE vs. GL_DECAL problem introduced by splash screen.
// polare3d.h renamed to polar3d.hxx
// fg{Cartesian,Polar}Point3d consolodated.
// Added some initial support for calculating local current ground elevation.
//
// Revision 1.9  1998/07/06 21:34:34  curt
// Added using namespace std for compilers that support this.
//
// Revision 1.8  1998/07/04 00:54:30  curt
// Added automatic mipmap generation.
//
// When rendering fragments, use saved model view matrix from associated tile
// rather than recalculating it with push() translate() pop().
//
// Revision 1.7  1998/06/12 00:58:05  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
//
// Revision 1.6  1998/06/08 17:57:54  curt
// Working first pass at material proporty sorting.
//
// Revision 1.5  1998/06/06 01:09:32  curt
// I goofed on the log message in the last commit ... now fixed.
//
// Revision 1.4  1998/06/06 01:07:18  curt
// Increased per material fragment list size from 100 to 400.
// Now correctly draw viewable fragments in per material order.
//
// Revision 1.3  1998/06/05 22:39:54  curt
// Working on sorting by, and rendering by material properties.
//
// Revision 1.2  1998/06/03 00:47:50  curt
// No .h for STL includes.
// Minor view culling optimizations.
//
// Revision 1.1  1998/05/23 14:09:21  curt
// Added tile.cxx and tile.hxx.
// Working on rewriting the tile management system so a tile is just a list
// fragments, and the fragment record contains the display list for that fragment.
//
