// light.hxx -- lighting routines
//
// Written by Curtis Olson, started April 1998.
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


#ifndef _LIGHT_HXX
#define _LIGHT_HXX


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

#include <Include/fg_types.h>
#include <Math/interpolater.hxx>


// Define a structure containing the global lighting parameters
class fgLIGHT {

    // Lighting look up tables (based on sun angle with local horizon)
    fgINTERPTABLE *ambient_tbl;
    fgINTERPTABLE *diffuse_tbl;
    fgINTERPTABLE *sky_tbl;

public:

    ///////////////////////////////////////////////////////////
    // position of the sun in various forms

    // in geocentric coordinates
    double sun_lon, sun_gc_lat;

    // in cartesian coordiantes
    fgPoint3d fg_sunpos;

    // (in view coordinates)
    GLfloat sun_vec[4];

    // inverse (in view coordinates)
    GLfloat sun_vec_inv[4];

    // the angle between the sun and the local horizontal
    double sun_angle;

    ///////////////////////////////////////////////////////////
    // Derived lighting values

    // ambient component
    GLfloat scene_ambient[3];

    // diffuse component
    GLfloat scene_diffuse[3];

    // fog color
    GLfloat fog_color[4];

    // clear screen color
    GLfloat sky_color[4];

    // Constructor
    fgLIGHT( void );

    // initialize lighting tables
    void Init( void );

    // update lighting parameters based on current sun position
    void Update( void);

    // Destructor
    ~fgLIGHT( void );
};


// Global shared light parameter structure
extern fgLIGHT cur_light_params;


// wrapper function for updating light parameters via the event scheduler
void fgLightUpdate ( void );


#endif // _LIGHT_HXX


// $Log$
// Revision 1.5  1998/07/08 14:48:39  curt
// polar3d.h renamed to polar3d.hxx
//
// Revision 1.4  1998/05/20 20:54:17  curt
// Converted fgLIGHT to a C++ class.
//
// Revision 1.3  1998/05/02 01:53:18  curt
// Fine tuning mktime() support because of varying behavior on different
// platforms.
//
// Revision 1.2  1998/04/24 00:52:31  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Fog color fixes.
// Separated out lighting calcs into their own file.
//
// Revision 1.1  1998/04/22 13:24:06  curt
// C++ - ifiing the code a bit.
// Starting to reorginize some of the lighting calcs to use a table lookup.
//
//
