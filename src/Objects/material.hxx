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

#include <Include/compiler.h>

#include <GL/glut.h>
#include <XGL/xgl.h>

#include STL_STRING      // Standard C++ string library

FG_USING_STD(string);


// MSVC++ 6.0 kuldge - Need forward declaration of friends.
class fgMATERIAL;
istream& operator >> ( istream& in, fgMATERIAL& m );

// Material property class
class FGMaterial {

private:
    // texture loaded
    bool loaded;

    // OpenGL texture name
    GLuint texture_id;

    // file name of texture
    string texture_name;

    // alpha texture?
    int alpha;

    // texture size
    double xsize, ysize;

    // material properties
    GLfloat ambient[4], diffuse[4], specular[4], emissive[4];
    GLint texture_ptr;

public:

    // Constructor
    FGMaterial ( void );

    // Destructor
    ~FGMaterial ( void );

    void load_texture( const string& root );

    friend istream& operator >> ( istream& in, FGMaterial& m );

    inline bool is_loaded() const { return loaded; }
    inline GLuint get_texture_id() const { return texture_id; }
    inline double get_xsize() const { return xsize; }
    inline double get_ysize() const { return ysize; }
    inline GLfloat *get_ambient() { return ambient; }
    inline GLfloat *get_diffuse() { return diffuse; }
    inline GLfloat *get_specular() { return specular; }
    inline GLfloat *get_emissive() { return emissive; }
};


#endif // _MATERIAL_HXX 


