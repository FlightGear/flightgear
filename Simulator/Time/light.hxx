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

// #include <Include/fg_types.h>
#include <Math/interpolater.hxx>
#include <Math/point3d.hxx>


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
    GLfloat sun_vec[4];

    // inverse (in view coordinates)
    GLfloat sun_vec_inv[4];

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


// $Log$
// Revision 1.1  1999/04/05 21:32:47  curt
// Initial revision
//
// Revision 1.9  1999/03/22 02:08:17  curt
// Changes contributed by Durk Talsma:
//
// Here's a few changes I made to fg-0.58 this weekend. Included are the
// following features:
// - Sun and moon have a halo
// - The moon has a light vector, moon_angle, etc. etc. so that we can have
//   some moonlight during the night.
// - Lot's of small changes tweakes, including some stuff Norman Vine sent
//   me earlier.
//
// Revision 1.8  1998/10/16 00:56:10  curt
// Converted to Point3D class.
//
// Revision 1.7  1998/08/29 13:11:33  curt
// Bernie Bright writes:
//   I've created some new classes to enable pointers-to-functions and
//   pointers-to-class-methods to be treated like objects.  These objects
//   can be registered with fgEVENT_MGR.
//
//   File "Include/fg_callback.hxx" contains the callback class defns.
//
//   Modified fgEVENT and fgEVENT_MGR to use the callback classes.  Also
//   some minor tweaks to STL usage.
//
//   Added file "Include/fg_stl_config.h" to deal with STL portability
//   issues.  I've added an initial config for egcs (and probably gcc-2.8.x).
//   I don't have access to Visual C++ so I've left that for someone else.
//   This file is influenced by the stl_config.h file delivered with egcs.
//
//   Added "Include/auto_ptr.hxx" which contains an implementation of the
//   STL auto_ptr class which is not provided in all STL implementations
//   and is needed to use the callback classes.
//
//   Deleted fgLightUpdate() which was just a wrapper to call
//   fgLIGHT::Update().
//
//   Modified fg_init.cxx to register two method callbacks in place of the
//   old wrapper functions.
//
// Revision 1.6  1998/07/22 21:45:39  curt
// fg_time.cxx: Removed call to ctime() in a printf() which should be harmless
//   but seems to be triggering a bug.
// light.cxx: Added code to adjust fog color based on sunrise/sunset effects
//   and view orientation.  This is an attempt to match the fog color to the
//   sky color in the center of the screen.  You see discrepancies at the
//   edges, but what else can be done?
// sunpos.cxx: Caculate local direction to sun here.  (what compass direction
//   do we need to face to point directly at sun)
//
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
