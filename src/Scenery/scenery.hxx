// scenery.hxx -- data structures and routines for managing scenery.
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#ifndef _SCENERY_HXX
#define _SCENERY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/math/point3d.hxx>


// Define a structure containing global scenery parameters
struct fgSCENERY {
    // center of current scenery chunk
    Point3D center;

    // next center of current scenery chunk
    Point3D next_center;

    // angle of sun relative to current local horizontal
    double sun_angle;

    // elevation of terrain at our current lat/lon (based on the
    // actual drawn polygons)
    double cur_elev;
};

extern struct fgSCENERY scenery;


// Initialize the Scenery Management system
int fgSceneryInit( void );


// Tell the scenery manager where we are so it can load the proper
// data, and build the proper structures.
void fgSceneryUpdate(double lon, double lat, double elev);


// Render out the current scene
void fgSceneryRender( void );


#endif // _SCENERY_HXX


