// groundcache.hxx -- carries a small subset of the scenegraph near the vehicle
//
// Written by Mathias Froehlich, started Nov 2004.
//
// Copyright (C) 2004  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef _GROUNDCACHE_HXX
#define _GROUNDCACHE_HXX

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/SGMath.hxx>

class SGMaterial;
class GroundCacheFillVisitor;

class FGGroundCache {
public:
    FGGroundCache();
    ~FGGroundCache();

    //////////////////////////////////////////////////////////////////////////
    // Ground handling routines
    //////////////////////////////////////////////////////////////////////////

    // Prepare the ground cache for the wgs84 position pt_*.
    // That is take all vertices in the ball with radius rad around the
    // position given by the pt_* and store them in a local scene graph.
    bool prepare_ground_cache(double ref_time, const SGVec3d& pt,
                              double rad);

    // Returns true if the cache is valid.
    // Also the reference time, point and radius values where the cache
    // is valid for are returned.
    bool is_valid(double& ref_time, SGVec3d& pt, double& rad);

    // Return the nearest catapult to the given point
    // pt in wgs84 coordinates.
    double get_cat(double t, const SGVec3d& pt,
                   SGVec3d end[2], SGVec3d vel[2]);
  

    // Return the altitude above ground below the wgs84 point pt
    // Search for highest triangle not higher than pt + max_altoff.
    // Return ground properties like the ground type, the maximum load
    // this kind kind of ground can carry, the friction factor between
    // 0 and 1 which can be used to model lower friction with wet runways
    // and finally the altitude above ground.
    bool get_agl(double t, const SGVec3d& pt, double max_altoff,
                 SGVec3d& contact, SGVec3d& normal, SGVec3d& vel,
                 int *type, const SGMaterial** material, double *agl);

    // Return 1 if the hook intersects with a wire.
    // That test is done by checking if the quad spanned by the points pt*
    // intersects with the line representing the wire.
    // If the wire is caught, the cache will trace this wires endpoints until
    // the FDM calls release_wire().
    bool caught_wire(double t, const SGVec3d pt[4]);
  
    // Return the location and speed of the wire endpoints.
    bool get_wire_ends(double t, SGVec3d end[2], SGVec3d vel[2]);

    // Tell the cache code that it does no longer need to care for
    // the wire end position.
    void release_wire(void);

private:
    friend class GroundCacheFillVisitor;

    struct Triangle {
      Triangle() : material(0) {}
      // The edge vertices.
      SGVec3d vertices[3];
      // The surface normal.
      SGVec4d plane;
      // The bounding shpere.
      SGVec3d boundCenter;
      double boundRadius;
      // The linear and angular velocity.
      SGVec3d velocity;
      SGVec3d rotation;
      SGVec3d rotation_pivot;
      // Ground type
      int type;
      // the simgear material reference, contains friction coeficients ...
      const SGMaterial* material;
    };
    struct Catapult {
      SGVec3d start;
      SGVec3d end;
      SGVec3d velocity;
      SGVec3d rotation;
      SGVec3d rotation_pivot;
    };
    struct Wire {
      SGVec3d ends[2];
      SGVec3d velocity;
      SGVec3d rotation;
      SGVec3d rotation_pivot;
      int wire_id;
    };


    // The center of the cache.
    SGVec3d cache_center;
    // Approximate ground radius.
    // In case the aircraft is too high above ground.
    double ground_radius;
    // Ground type
    int _type;
    // the simgear material reference, contains friction coeficients ...
    const SGMaterial* _material;
    // The time reference for later call to intersection test routines.
    // Is required since we will have moving triangles in carriers.
    double cache_ref_time;
    // The wire identifier to track.
    int wire_id;

    // Containers which hold all the essential information about this cache.
    std::vector<Triangle> triangles;
    std::vector<Catapult> catapults;
    std::vector<Wire> wires;

    // The point and radius where the cache is built around.
    // That are the arguments that were given to prepare_ground_cache.
    SGVec3d reference_wgs84_point;
    double reference_vehicle_radius;
    SGVec3d down;
    bool found_ground;


    // Helper class to hold some properties of the ground triangle.
    struct GroundProperty {
      GroundProperty() : type(0), material(0) {}
      int type;
      int wire_id;
      SGVec3d vel;
      SGVec3d rot;
      SGVec3d pivot;
      const SGMaterial* material;
    };

    static void velocityTransformTriangle(double dt, Triangle& dst,
                                          const Triangle& src);
};

#endif
