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

#ifdef __sun__
extern "C" void *memmove(void *, const void *, size_t);
extern "C" void *memset(void *, int, size_t);
#endif

#include <map>             // STL associative "array"
#include <string>          // Standard C++ string library

#include "tile.hxx"


#define FG_MAX_MATERIAL_FRAGS 400


// Material property class
class fgMATERIAL {

public:
    // file name of texture
    char texture_name[256];

    // material properties
    GLfloat ambient[4], diffuse[4], specular[4], emissive[4];
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
    map < string, fgMATERIAL, less<string> > material_map;

    // Constructor
    fgMATERIAL_MGR ( void );

    // Load a library of material properties
    int load_lib ( void );

    // Initialize the transient list of fragments for each material property
    void init_transient_material_lists( void );

    // Destructor
    ~fgMATERIAL_MGR ( void );
};


// global material management class
extern fgMATERIAL_MGR material_mgr;


#endif // _MATERIAL_HXX 


// $Log$
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
