//
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


#include <config.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Include/fg_types.h>


// Define a structure containing the global lighting parameters
typedef struct {

    ///////////////////////////////////////////////////////////
    // position of the sun in various forms

    // in geocentric coordinates
    double sun_lon, sun_gc_lat;

    // in cartesian coordiantes
    struct fgCartesianPoint fg_sunpos;

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
} fgLIGHT;


extern fgLIGHT cur_light_params;


// initialize lighting tables
void fgLightInit( void );


// update lighting parameters based on current sun position
void fgLightUpdate( void);


#endif // _LIGHT_HXX


// $Log$
// Revision 1.1  1998/04/22 13:24:06  curt
// C++ - ifiing the code a bit.
// Starting to reorginize some of the lighting calcs to use a table lookup.
//
//
