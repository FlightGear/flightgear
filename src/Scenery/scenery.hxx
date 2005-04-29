// scenery.hxx -- data structures and routines for managing scenery.
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <list>

#include <plib/sg.h>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/point3d.hxx>

SG_USING_STD(list);

class ssgRoot;
class ssgBranch;
class ssgPlacementTransform;


// Define a structure containing global scenery parameters
class FGScenery : public SGSubsystem {
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

    // SSG scene graph
    ssgRoot *scene_graph;
    ssgBranch *terrain_branch;
    ssgRoot *gnd_lights_root;
    ssgRoot *vasi_lights_root;
    ssgRoot *rwy_lights_root;
    ssgRoot *taxi_lights_root;
    ssgBranch *models_branch;
    ssgBranch *aircraft_branch;

    // list of all placement transform, used to move the scenery center on the fly.
    typedef list<ssgPlacementTransform*> placement_list_type;
    placement_list_type _placement_list;

public:

    FGScenery();
    ~FGScenery();

    // Implementation of SGSubsystem.
    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    inline double get_cur_elev() const { return cur_elev; }
    inline void set_cur_elev( double e ) { cur_elev = e; }

    inline Point3D get_center() const { return center; }
    void set_center( Point3D p );

    inline Point3D get_next_center() const { return next_center; }
    inline void set_next_center( Point3D p ) { next_center = p; }

    inline void set_cur_radius( double r ) { cur_radius = r; }
    inline void set_cur_normal( sgdVec3 n ) { sgdCopyVec3( cur_normal, n ); }

    inline ssgRoot *get_scene_graph () const { return scene_graph; }
    inline void set_scene_graph (ssgRoot * s) { scene_graph = s; }

    inline ssgBranch *get_terrain_branch () const { return terrain_branch; }
    inline void set_terrain_branch (ssgBranch * t) { terrain_branch = t; }

    inline ssgRoot *get_gnd_lights_root () const {
        return gnd_lights_root;
    }
    inline void set_gnd_lights_root (ssgRoot *r) {
        gnd_lights_root = r;
    }

    inline ssgRoot *get_vasi_lights_root () const {
        return vasi_lights_root;
    }
    inline void set_vasi_lights_root (ssgRoot *r) {
        vasi_lights_root = r;
    }

    inline ssgRoot *get_rwy_lights_root () const {
        return rwy_lights_root;
    }
    inline void set_rwy_lights_root (ssgRoot *r) {
        rwy_lights_root = r;
    }

    inline ssgRoot *get_taxi_lights_root () const {
        return taxi_lights_root;
    }
    inline void set_taxi_lights_root (ssgRoot *r) {
        taxi_lights_root = r;
    }

    inline ssgBranch *get_models_branch () const {
        return models_branch;
    }
    inline void set_models_branch (ssgBranch *t) {
        models_branch = t;
    }

    inline ssgBranch *get_aircraft_branch () const {
        return aircraft_branch;
    }
    inline void set_aircraft_branch (ssgBranch *t) {
        aircraft_branch = t;
    }

    void register_placement_transform(ssgPlacementTransform *trans);
    void unregister_placement_transform(ssgPlacementTransform *trans);
};


#endif // _SCENERY_HXX


