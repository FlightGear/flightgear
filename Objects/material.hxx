// material.hxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
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


#ifndef _MATERIAL_HXX
#define _MATERIAL_HXX


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

#include <string>        // Standard C++ string library
#include <map>           // STL associative "array"
#include <vector>        // STL "array"

#include "Include/compiler.h"
FG_USING_STD(string);
FG_USING_STD(map);
FG_USING_STD(vector);
FG_USING_STD(less);

// forward decl.
class fgFRAGMENT;


// convenience types
typedef vector < fgFRAGMENT * > frag_list_type;
typedef frag_list_type::iterator frag_list_iterator;
typedef frag_list_type::const_iterator frag_list_const_iterator;


// #define FG_MAX_MATERIAL_FRAGS 800

// MSVC++ 6.0 kuldge - Need forward declaration of friends.
class fgMATERIAL;
istream& operator >> ( istream& in, fgMATERIAL& m );

// Material property class
class fgMATERIAL {

private:
    // OpenGL texture name
    GLuint texture_id;

    // file name of texture
    string texture_name;

    // alpha texture?
    int alpha;

    // material properties
    GLfloat ambient[4], diffuse[4], specular[4], emissive[4];
    GLint texture_ptr;

    // transient list of objects with this material type (used for sorting
    // by material to reduce GL state changes when rendering the scene
    frag_list_type list;
    // size_t list_size;

public:

    // Constructor
    fgMATERIAL ( void );

    int size() const { return list.size(); }
    bool empty() const { return list.size() == 0; }

    // Sorting routines
    void init_sort_list( void ) {
	list.erase( list.begin(), list.end() );
    }

    bool append_sort_list( fgFRAGMENT *object ) {
	list.push_back( object );
	return true;
    }

    void render_fragments();

    void load_texture();

    // Destructor
    ~fgMATERIAL ( void );

    friend istream& operator >> ( istream& in, fgMATERIAL& m );
};


// Material management class
class fgMATERIAL_MGR {

public:

    // associative array of materials
    typedef map < string, fgMATERIAL, less<string> > container;
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

    iterator begin() { return material_map.begin(); }
    const_iterator begin() const { return material_map.begin(); }

    iterator end() { return material_map.end(); }
    const_iterator end() const { return material_map.end(); }

    // Constructor
    fgMATERIAL_MGR ( void );

    // Load a library of material properties
    int load_lib ( void );

    bool get_textures_loaded() { return textures_loaded; }

    // Initialize the transient list of fragments for each material property
    void init_transient_material_lists( void );

    bool find( const string& material, fgMATERIAL*& mtl_ptr );

    void render_fragments();

    // Destructor
    ~fgMATERIAL_MGR ( void );

private:

    // Have textures been loaded
    bool textures_loaded;

    container material_map;
};


// global material management class
extern fgMATERIAL_MGR material_mgr;


#endif // _MATERIAL_HXX 


// $Log$
// Revision 1.6  1999/02/02 20:13:39  curt
// MSVC++ portability changes by Bernie Bright:
//
// Lib/Serial/serial.[ch]xx: Initial Windows support - incomplete.
// Simulator/Astro/stars.cxx: typo? included <stdio> instead of <cstdio>
// Simulator/Cockpit/hud.cxx: Added Standard headers
// Simulator/Cockpit/panel.cxx: Redefinition of default parameter
// Simulator/Flight/flight.cxx: Replaced cout with FG_LOG.  Deleted <stdio.h>
// Simulator/Main/fg_init.cxx:
// Simulator/Main/GLUTmain.cxx:
// Simulator/Main/options.hxx: Shuffled <fg_serial.hxx> dependency
// Simulator/Objects/material.hxx:
// Simulator/Time/timestamp.hxx: VC++ friend kludge
// Simulator/Scenery/tile.[ch]xx: Fixed using std::X declarations
// Simulator/Main/views.hxx: Added a constant
//
// Revision 1.5  1998/10/12 23:49:18  curt
// Changes from NHV to make the code more dynamic with fewer hard coded limits.
//
// Revision 1.4  1998/09/17 18:35:53  curt
// Tweaks and optimizations by Norman Vine.
//
// Revision 1.3  1998/09/10 19:07:12  curt
// /Simulator/Objects/fragment.hxx
//   Nested fgFACE inside fgFRAGMENT since its not used anywhere else.
//
// ./Simulator/Objects/material.cxx
// ./Simulator/Objects/material.hxx
//   Made fgMATERIAL and fgMATERIAL_MGR bona fide classes with private
//   data members - that should keep the rabble happy :)
//
// ./Simulator/Scenery/tilemgr.cxx
//   In viewable() delay evaluation of eye[0] and eye[1] in until they're
//   actually needed.
//   Change to fgTileMgrRender() to call fgMATERIAL_MGR::render_fragments()
//   method.
//
// ./Include/fg_stl_config.h
// ./Include/auto_ptr.hxx
//   Added support for g++ 2.7.
//   Further changes to other files are forthcoming.
//
// Brief summary of changes required for g++ 2.7.
//   operator->() not supported by iterators: use (*i).x instead of i->x
//   default template arguments not supported,
//   <functional> doesn't have mem_fun_ref() needed by callbacks.
//   some std include files have different names.
//   template member functions not supported.
//
// Revision 1.2  1998/09/01 19:03:09  curt
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
// Revision 1.1  1998/08/25 16:51:24  curt
// Moved from ../Scenery
//
// Revision 1.10  1998/07/24 21:42:06  curt
// material.cxx: whups, double method declaration with no definition.
// obj.cxx: tweaks to avoid errors in SGI's CC.
// tile.cxx: optimizations by Norman Vine.
// tilemgr.cxx: optimizations by Norman Vine.
//
// Revision 1.9  1998/07/06 21:34:33  curt
// Added using namespace std for compilers that support this.
//
// Revision 1.8  1998/06/17 21:36:39  curt
// Load and manage multiple textures defined in the Materials library.
// Boost max material fagments for each material property to 800.
// Multiple texture support when rendering.
//
// Revision 1.7  1998/06/12 00:58:04  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
//
// Revision 1.6  1998/06/06 01:09:31  curt
// I goofed on the log message in the last commit ... now fixed.
//
// Revision 1.5  1998/06/06 01:07:17  curt
// Increased per material fragment list size from 100 to 400.
// Now correctly draw viewable fragments in per material order.
//
// Revision 1.4  1998/06/05 22:39:53  curt
// Working on sorting by, and rendering by material properties.
//
// Revision 1.3  1998/06/03 00:47:50  curt
// No .h for STL includes.
// Minor view culling optimizations.
//
// Revision 1.2  1998/06/01 17:56:20  curt
// Incremental additions to material.cxx (not fully functional)
// Tweaked vfc_ratio math to avoid divide by zero.
//
// Revision 1.1  1998/05/30 01:56:45  curt
// Added material.cxx material.hxx
//
