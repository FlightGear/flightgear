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
#include <simgear/xgl/xgl.h>

#include <plib/sg.h>			// plib include

#include <simgear/math/interpolater.hxx>
#include <simgear/math/point3d.hxx>


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
    Point3D fg_sunpos;

    // (in view coordinates)
    sgVec4 sun_vec;

    // inverse (in view coordinates)
    sgVec4 sun_vec_inv;

    // the angle between the sun and the local horizontal (in radians)
    double sun_angle;

    // the rotation around our vertical axis of the sun (relative to
    // due south with positive numbers going in the counter clockwise
    // direction.)  This is the direction we'd need to face if we
    // wanted to travel towards the sun.
    double sun_rotation;

    ///////////////////////////////////////////////////////////
    // Have the same for the moon. Useful for having some light at night
    // and stuff. I (Durk) also want to use this for adding similar 
    // coloring effects to the moon as I did to the sun. 
    ///////////////////////////////////////////////////////////
    // position of the moon in various forms

    // in geocentric coordinates
    double moon_lon, moon_gc_lat;

    // in cartesian coordiantes
    Point3D fg_moonpos;

    // (in view coordinates)
    GLfloat moon_vec[4];

    // inverse (in view coordinates)
    GLfloat moon_vec_inv[4];

    // the angle between the moon and the local horizontal (in radians)
    double moon_angle;

    // the rotation around our vertical axis of the moon (relative to
    // due south with positive numbers going in the counter clockwise
    // direction.)  This is the direction we'd need to face if we
    // wanted to travel towards the sun.
    double moon_rotation;

    ///////////////////////////////////////////////////////////
    // Derived lighting values

    // ambient component
    GLfloat scene_ambient[3];

    // diffuse component
    GLfloat scene_diffuse[3];

    // fog color
    GLfloat fog_color[4];

    // fog color adjusted for sunset effects
    GLfloat adj_fog_color[4];

    // clear screen color
    GLfloat sky_color[4];

    // Constructor
    fgLIGHT( void );

    // initialize lighting tables
    void Init( void );

    // update lighting parameters based on current sun position
    void Update( void);

    // calculate fog color adjusted for sunrise/sunset effects
    void UpdateAdjFog( void );

    // Destructor
    ~fgLIGHT( void );
};


// Global shared light parameter structure
extern fgLIGHT cur_light_params;


#endif // _LIGHT_HXX
