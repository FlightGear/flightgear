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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include <simgear/scene/model/placementtrans.hxx>
#include <simgear/structure/ssgSharedPtr.hxx>

SG_USING_STD(list);

class ssgRoot;
class ssgBranch;
class SGMaterial;


// Define a structure containing global scenery parameters
class FGScenery : public SGSubsystem {
    // center of current scenery chunk
    SGVec3d center;

    // FIXME this should be a views property
    // angle of sun relative to current local horizontal
    double sun_angle;

    // SSG scene graph
    ssgSharedPtr<ssgRoot> scene_graph;
    ssgSharedPtr<ssgBranch> terrain_branch;
    ssgSharedPtr<ssgRoot> gnd_lights_root;
    ssgSharedPtr<ssgRoot> vasi_lights_root;
    ssgSharedPtr<ssgRoot> rwy_lights_root;
    ssgSharedPtr<ssgRoot> taxi_lights_root;
    ssgSharedPtr<ssgBranch> models_branch;
    ssgSharedPtr<ssgBranch> aircraft_branch;

    // list of all placement transform, used to move the scenery center on the fly.
    typedef list<ssgSharedPtr<ssgPlacementTransform> > placement_list_type;
    placement_list_type _placement_list;

public:

    FGScenery();
    ~FGScenery();

    // Implementation of SGSubsystem.
    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    /// Compute the elevation of the scenery at geodetic latitude lat,
    /// geodetic longitude lon and not higher than max_alt.
    /// If the exact flag is set to true, the scenery center is moved to
    /// gain a higher accuracy of that query. The center is restored past
    /// that to the original value.
    /// The altitude hit is returned in the alt argument.
    /// The method returns true if the scenery is available for the given
    /// lat/lon pair. If there is no scenery for that point, the altitude
    /// value is undefined. 
    /// All values are meant to be in meters or degrees.
    bool get_elevation_m(double lat, double lon, double max_alt,
                         double& alt, const SGMaterial** material,
                         bool exact = false);

    /// Compute the elevation of the scenery beow the cartesian point pos.
    /// you the returned scenery altitude is not higher than the position
    /// pos plus an ofset given with max_altoff.
    /// If the exact flag is set to true, the scenery center is moved to
    /// gain a higher accuracy of that query. The center is restored past
    /// that to the original value.
    /// The altitude hit is returned in the alt argument.
    /// The method returns true if the scenery is available for the given
    /// lat/lon pair. If there is no scenery for that point, the altitude
    /// value is undefined.
    /// All values are meant to be in meters.
    bool get_cart_elevation_m(const SGVec3d& pos, double max_altoff,
                              double& radius, const SGMaterial** material,
                              bool exact = false);

    /// Compute the nearest intersection point of the line starting from 
    /// start going in direction dir with the terrain.
    /// The input and output values should be in cartesian coordinates in the
    /// usual earth centered wgs84 coordiante system. Units are meters.
    /// On success, true is returned.
    bool get_cart_ground_intersection(const SGVec3d& start, const SGVec3d& dir,
                                      SGVec3d& nearestHit, bool exact = false);

    const SGVec3d& get_center() const { return center; }
    void set_center( const SGVec3d& p );

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


