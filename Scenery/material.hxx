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

#include <map.h>           // STL associative "array"

#if defined(__CYGWIN32__)
#  include <string>        // Standard C++ string library
#elif defined(WIN32)
#  include <string.h>      // Standard C++ string library
#else
#  include <std/string.h>  // Standard C++ string library
#endif

#include "tile.hxx"


#define FG_MAX_MATERIAL_FRAGS 100


// Material property class
class fgMATERIAL {

public:

    // material properties
    GLfloat ambient[4], diffuse[4], specular[4];
    GLint texture_ptr;

    // transient list of objects with this material type (used for sorting
    // by material to reduce GL state changes when rendering the scene
    fgFRAGMENT * list[FG_MAX_MATERIAL_FRAGS];
    int list_size;

    // Constructor
    fgMATERIAL ( void );

    // Sorting routines
    void init_sort_list( void );
    int append_sort_list( fgFRAGMENT *object );

    // Destructor
    ~fgMATERIAL ( void );
};


// Material management class
class fgMATERIAL_MGR {

public:

    // associative array of materials
    map < string, fgMATERIAL, less<string> > materials;

    // Constructor
    fgMATERIAL_MGR ( void );

    // Load a library of material properties
    int load_lib ( char *file );

    // Destructor
    ~fgMATERIAL_MGR ( void );
};


#endif // _MATERIAL_HXX 


// $Log$
// Revision 1.1  1998/05/30 01:56:45  curt
// Added material.cxx material.hxx
//
