// matlib.hxx -- class to handle material properties
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


#ifndef _MATLIB_HXX
#define _MATLIB_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#include STL_STRING		// Standard C++ string library
#include <map>			// STL associative "array"
#include <vector>		// STL "array"

#include <plib/ssg.h>		// plib include

#include "newmat.hxx"


SG_USING_STD(string);
SG_USING_STD(map);
SG_USING_STD(vector);
SG_USING_STD(less);


// Material management class
class FGMaterialLib {

private:

    // associative array of materials
    typedef map < string, FGNewMat, less<string> > material_map;
    typedef material_map::iterator material_map_iterator;
    typedef material_map::const_iterator const_material_map_iterator;

    material_map matlib;

public:

    // Constructor
    FGMaterialLib ( void );

    // Load a library of material properties
    bool load( const string& mpath );

    // Add the named texture with default properties
    bool add_item( const string &tex_path );
    bool add_item( const string &mat_name, const string &tex_path );
    bool add_item( const string &mat_name, ssgSimpleState *state );

    // find a material record by material name
    FGNewMat *find( const string& material );

    void set_step (int step);

    material_map_iterator begin() { return matlib.begin(); }
    const_material_map_iterator begin() const { return matlib.begin(); }

    material_map_iterator end() { return matlib.end(); }
    const_material_map_iterator end() const { return matlib.end(); }

    // Destructor
    ~FGMaterialLib ( void );
};


// global material management class
extern FGMaterialLib material_lib;


#endif // _MATLIB_HXX 
