// newmat.hxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _NEWMAT_HXX
#define _NEWMAT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/compiler.h>

#include <GL/glut.h>

#include STL_STRING      // Standard C++ string library

SG_USING_STD(string);


// MSVC++ 6.0 kuldge - Need forward declaration of friends.
class FGNewMat;
istream& operator >> ( istream& in, FGNewMat& m );

// Material property class
class FGNewMat {

private:

    // names
    string material_name;
    string texture_name;

    // pointers to ssg states
    ssgStateSelector *state;
    ssgSimpleState *textured;
    ssgSimpleState *nontextured;

    // alpha texture?
    int alpha;

    // texture size
    double xsize, ysize;

    // wrap texture?
    int wrapu, wrapv;

    // use mipmapping?
    int mipmap;

    // coverage of night lighting.  This number is specifically the
    // amount of area coverage we give a single light.  The size of a
    // triangle is divided by this number and that is the number of
    // lights assigned to that triangle.  Lower numbers mean more
    // dense light ocverage.
    double light_coverage;

    // material properties
    sgVec4 ambient, diffuse, specular, emission;

    // true if texture loading deferred, and not yet loaded
    bool texture_loaded;

    // ref count so we can properly delete if we have multiple
    // pointers to this record
    int refcount;

public:

    // Constructor
    FGNewMat ( void );
    FGNewMat ( const string& name );
    FGNewMat ( const string &mat_name, const string &tex_name );

    // Destructor
    ~FGNewMat ( void );

    friend istream& operator >> ( istream& in, FGNewMat& m );

    // void load_texture( const string& root );
    void build_ssg_state( GLenum shade_model, bool texture_default,
			  bool defer_tex_load = false );
    void set_ssg_state( ssgSimpleState *s );

    inline string get_material_name() const { return material_name; }
    inline void set_material_name( const string& n ) { material_name = n; }

    inline string get_texture_name() const { return texture_name; }
    inline const char *get_texture_name_c_str() const {
        return texture_name.c_str();
    }
    inline void set_texture_name( const string& n ) { texture_name = n; }

    inline ssgSimpleState *get_textured() { return textured; }

    inline double get_xsize() const { return xsize; }
    inline double get_ysize() const { return ysize; }
    inline void set_xsize( double x ) { xsize = x; }
    inline void set_ysize( double y ) { ysize = y; }

    inline float *get_ambient() { return ambient; }
    inline float *get_diffuse() { return diffuse; }
    inline float *get_specular() { return specular; }
    inline float *get_emission() { return emission; }
    inline void set_ambient( sgVec4 a ) { sgCopyVec4( ambient, a ); }
    inline void set_diffuse( sgVec4 d ) { sgCopyVec4( diffuse, d ); }
    inline void set_specular( sgVec4 s ) { sgCopyVec4( specular, s ); }
    inline void set_emission( sgVec4 e ) { sgCopyVec4( emission, e ); }

    inline bool get_texture_loaded() const { return texture_loaded; }
    inline void set_texture_loaded( bool val ) { texture_loaded = val; }

    inline double get_light_coverage () const { return light_coverage; }
    inline void set_light_coverage (double coverage) {
	light_coverage = coverage;
    }

    inline ssgStateSelector *get_state() const { return state; }

    inline void ref() { refcount++; }
    inline void deRef() { refcount--; }
    inline int getRef() const { return refcount; }

    void dump_info();
};


#endif // _NEWMAT_HXX 


