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


#include <plib/sg.h>
#include <simgear/math/point3d.hxx>

#include <Main/fgfs.hxx>


// Define a structure containing global scenery parameters
class FGScenery : public FGSubsystem {
    // center of current scenery chunk
    Point3D center;

    // next center of current scenery chunk
    Point3D next_center;

    // angle of sun relative to current local horizontal
    double sun_angle;

    // elevation of terrain at our current lat/lon (based on the
    // actual drawn polygons)
    double cur_elev;

    // the distance (radius) from the center of the earth to the
    // current scenery elevation point
    double cur_radius;

    // unit normal at point used to determine current elevation
    sgdVec3 cur_normal;

public:

    FGScenery();
    ~FGScenery();

    // Implementation of FGSubsystem.
    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    inline double get_cur_elev() const { return cur_elev; }
    inline void set_cur_elev( double e ) { cur_elev = e; }

    inline Point3D get_center() const { return center; }
    inline void set_center( Point3D p ) { center = p; }

    inline Point3D get_next_center() const { return next_center; }
    inline void set_next_center( Point3D p ) { next_center = p; }

    inline void set_cur_radius( double r ) { cur_radius = r; }
    inline void set_cur_normal( sgdVec3 n ) { sgdCopyVec3( cur_normal, n ); }
};


extern FGScenery scenery;


// Initialize the Scenery Management system
int fgSceneryInit( void );


#endif // _SCENERY_HXX


