// materialmgr.hxx -- class to handle material properties
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


#ifndef _MATERIALMGR_HXX
#define _MATERIALMGR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include "Include/compiler.h"

#include <GL/glut.h>
#include <XGL/xgl.h>

#include STL_STRING      // Standard C++ string library
#include <map>           // STL associative "array"
#include <vector>        // STL "array"

#include <Misc/material.hxx>

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
// class FGMaterialSlot;
// istream& operator >> ( istream& in, FGMaterialSlot& m );

// Material property class
class FGMaterialSlot {

private:
    FGMaterial m;

    // OpenGL texture name
    // GLuint texture_id;

    // file name of texture
    // string texture_name;

    // alpha texture?
    // int alpha;

    // texture size
    // double xsize, ysize;

    // material properties
    // GLfloat ambient[4], diffuse[4], specular[4], emissive[4];
    // GLint texture_ptr;

    // transient list of objects with this material type (used for sorting
    // by material to reduce GL state changes when rendering the scene
    frag_list_type list;
    // size_t list_size;

public:

    // Constructor
    FGMaterialSlot ( void );

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

    // void load_texture();

    // Destructor
    ~FGMaterialSlot ( void );

    // friend istream& operator >> ( istream& in, FGMaterialSlot& m );

    inline void set_m( FGMaterial new_m ) { m = new_m; }
};


// Material management class
class fgMATERIAL_MGR {

public:

    // associative array of materials
    typedef map < string, FGMaterialSlot, less<string> > container;
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

    inline bool loaded() const { return textures_loaded; }

    // Initialize the transient list of fragments for each material property
    void init_transient_material_lists( void );

    bool find( const string& material, FGMaterialSlot*& mtl_ptr );

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


#endif // _MATERIALMGR_HXX 


