// pt_lights.hxx -- build a 'directional' light on the fly
//
// Written by Curtis Olson, started March 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _PT_LIGHTS_HXX
#define _PT_LIGHTS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <vector>		// STL
#include STL_STRING

#include <plib/sg.h>
#include <plib/ssg.h>		// plib include

#include <simgear/math/sg_types.hxx>

SG_USING_STD(string);
SG_USING_STD(vector);


typedef vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator int_point_list_iterator;


// Define the various supported light types
typedef enum {
    FG_RWYLIGHT_TAXI = 0,
    FG_RWYLIGHT_VASI,
    FG_RWYLIGHT_EDGE,
    FG_RWYLIGHT_TOUCHDOWN,
    FG_RWYLIGHT_THRESHOLD,
    FG_RWYLIGHT_WHITE,
    FG_RWYLIGHT_RED,
    FG_RWYLIGHT_GREEN,
    FG_RWYLIGHT_YELLOW
} fgRunwayLightType;


// Generate a directional light.  This routines creates a
// 'directional' light that can only be viewed from within 90 degrees
// of the specified dir vector.

// To accomplish this, he routine creates a triangle with the 1st
// point coincident with the specified pt and the 2nd and 3rd points
// extending upward.  The 1st point is created with an 'alpha' value
// of 1 while the 2nd and 3rd points are created with an 'alpha' of
// 0.0.

// If the triangle is drawn in glPolygonMode(GL_FRONT, GL_POINT) mode,
// then two important things happen:

// 1) Only the 3 vertex points are drawn, the 2nd two with an alpha of
// 0 so actually only the desired point is rendered.

// 2) since we are drawing a triangle, back face culling takes care of
// eliminating this poing when the view angle relative to dir is
// greater than 90 degrees.

// The final piece of this puzzle is that if we now use environment
// mapping on this point, via an appropriate texture we can then
// control the intensity and color of the point based on the view
// angle relative to 'dir' the optimal view direction of the light
// (i.e. the direction the light is pointing.)

// Yes this get's to be long and convoluted.  If you can suggest a
// simpler way, please do! :-)

ssgLeaf *gen_directional_light( sgVec3 pt, sgVec3 dir, sgVec3 up );


ssgBranch *gen_directional_lights( const point_list &nodes,
                                   const point_list &normals,
                                   const int_list &pnt_i,
                                   const int_list &nml_i,
                                   sgVec3 up );


#endif // _PT_LIGHTS_HXX
