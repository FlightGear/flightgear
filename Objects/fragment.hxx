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

#include <vector>

#include <Bucket/bucketutils.h>
#include <Include/fg_types.h>
#include "Include/fg_constants.h"
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

    explicit fgFACE( int a = 0, int b =0, int c =0 )
	: n1(a), n2(b), n3(c) {}

    fgFACE( const fgFACE & image )
	: n1(image.n1), n2(image.n2), n3(image.n3) {}

    ~fgFACE() {}

    bool operator < ( const fgFACE & rhs ) { return n1 < rhs.n1; }
};

inline bool
operator == ( const fgFACE& lhs, const fgFACE & rhs )
{
    return (lhs.n1 == rhs.n1) && (lhs.n2 == rhs.n2) && (lhs.n3 == rhs.n3);
}

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
    typedef vector < fgFACE > container;
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

    container faces;

    // number of faces in this fragment
    int num_faces;

    // Add a face to the face list
    void add_face(int n1, int n2, int n3) {
	faces.push_back( fgFACE(n1,n2,n3) );
	num_faces++;
    }

    // test if line intesects with this fragment.  p0 and p1 are the
    // two line end points of the line.  If side_flag is true, check
    // to see that end points are on opposite sides of face.  Returns
    // 1 if it intersection found, 0 otherwise.  If it intesects,
    // result is the point of intersection
    int intersect( const fgPoint3d *end0,
		   const fgPoint3d *end1,
		   int side_flag,
		   fgPoint3d *result) const;

    // Constructors
    fgFRAGMENT () : num_faces(0) { /*faces.reserve(512);*/}
    fgFRAGMENT ( const fgFRAGMENT &image );

    // Destructor
    ~fgFRAGMENT() { faces.erase( faces.begin(), faces.end() ); }

    // operators
    fgFRAGMENT & operator = ( const fgFRAGMENT & rhs );

    bool operator <  ( const fgFRAGMENT & rhs ) {
	// This is completely arbitrary. It satisfies RW's STL implementation
	return bounding_radius < rhs.bounding_radius;
    }

    void init() {
	faces.erase( faces.begin(), faces.end() );
	num_faces = 0;
    }

    int deleteDisplayList() {
	xglDeleteLists( display_list, 1 ); return 0;
    }
};

inline bool
operator == ( const fgFRAGMENT & lhs, const fgFRAGMENT & rhs ) {
    return (( lhs.center.x - rhs.center.x ) < FG_EPSILON &&
	    ( lhs.center.y - rhs.center.y ) < FG_EPSILON &&
	    ( lhs.center.z - rhs.center.z ) < FG_EPSILON    );
}


#endif // _FRAGMENT_HXX 


// $Log$
// Revision 1.3  1998/09/08 21:40:44  curt
// Updates from Bernie Bright.
//
// Revision 1.2  1998/09/01 19:03:08  curt
// Changes contributed by Bernie Bright <bbright@c031.aone.net.au>
//  - The new classes in libmisc.tgz define a stream interface into zlib.
//    I've put these in a new directory, Lib/Misc.  Feel free to rename it
//    to something more appropriate.  However you'll have to change the
//    include directives in all the other files.  Additionally you'll have
//    add the library to Lib/Makefile.am and Simulator/Main/Makefile.am.
//
//    The StopWatch class in Lib/Misc requires a HAVE_GETRUSAGE autoconf
//    test so I've included the required changes in config.tgz.
//
//    There are a fair few changes to Simulator/Objects as I've moved
//    things around.  Loading tiles is quicker but thats not where the delay
//    is.  Tile loading takes a few tenths of a second per file on a P200
//    but it seems to be the post-processing that leads to a noticeable
//    blip in framerate.  I suppose its time to start profiling to see where
//    the delays are.
//
//    I've included a brief description of each archives contents.
//
// Lib/Misc/
//   zfstream.cxx
//   zfstream.hxx
//     C++ stream interface into zlib.
//     Taken from zlib-1.1.3/contrib/iostream/.
//     Minor mods for STL compatibility.
//     There's no copyright associated with these so I assume they're
//     covered by zlib's.
//
//   fgstream.cxx
//   fgstream.hxx
//     FlightGear input stream using gz_ifstream.  Tries to open the
//     given filename.  If that fails then filename is examined and a
//     ".gz" suffix is removed or appended and that file is opened.
//
//   stopwatch.hxx
//     A simple timer for benchmarking.  Not used in production code.
//     Taken from the Blitz++ project.  Covered by GPL.
//
//   strutils.cxx
//   strutils.hxx
//     Some simple string manipulation routines.
//
// Simulator/Airports/
//   Load airports database using fgstream.
//   Changed fgAIRPORTS to use set<> instead of map<>.
//   Added bool fgAIRPORTS::search() as a neater way doing the lookup.
//   Returns true if found.
//
// Simulator/Astro/
//   Modified fgStarsInit() to load stars database using fgstream.
//
// Simulator/Objects/
//   Modified fgObjLoad() to use fgstream.
//   Modified fgMATERIAL_MGR::load_lib() to use fgstream.
//   Many changes to fgMATERIAL.
//   Some changes to fgFRAGMENT but I forget what!
//
// Revision 1.1  1998/08/25 16:51:23  curt
// Moved from ../Scenery
//
//
