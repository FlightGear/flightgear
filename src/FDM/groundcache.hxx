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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifndef _GROUNDCACHE_HXX
#define _GROUNDCACHE_HXX

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/compiler.h>
#include <simgear/constants.h>

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
    bool prepare_ground_cache(double ref_time, const double pt[3],
                              double rad);

    // Returns true if the cache is valid.
    // Also the reference time, point and radius values where the cache
    // is valid for are returned.
    bool is_valid(double *ref_time, double pt[3], double *rad);
      

    // Return the nearest catapult to the given point
    // pt in wgs84 coordinates.
    double get_cat(double t, const double pt[3],
                   double end[2][3], double vel[2][3]);
  

    // Return the altitude above ground below the wgs84 point pt
    // Search for highest triangle not higher than pt + max_altoff.
    // Return ground properties like the ground type, the maximum load
    // this kind kind of ground can carry, the friction factor between
    // 0 and 1 which can be used to model lower friction with wet runways
    // and finally the altitude above ground.
    bool get_agl(double t, const double pt[3], double max_altoff,
                 double contact[3], double normal[3], double vel[3],
                 int *type, double *loadCapacity,
                 double *frictionFactor, double *agl);

    // Return 1 if the hook intersects with a wire.
    // That test is done by checking if the quad spanned by the points pt*
    // intersects with the line representing the wire.
    // If the wire is caught, the cache will trace this wires endpoints until
    // the FDM calls release_wire().
    bool caught_wire(double t, const double pt[4][3]);
  
    // Return the location and speed of the wire endpoints.
    bool get_wire_ends(double t, double end[2][3], double vel[2][3]);

    // Tell the cache code that it does no longer need to care for
    // the wire end position.
    void release_wire(void);

private:
    struct Triangle {
      // The edge vertices.
      sgdVec3 vertices[3];
      // The surface normal.
      sgdVec4 plane;
      // The bounding shpere.
      sgdSphere sphere;
      // The linear velocity.
      sgdVec3 velocity;
      // Ground type
      int type;
    };
    struct Catapult {
      sgdVec3 start;
      sgdVec3 end;
      sgdVec3 velocity;
    };
    struct Wire {
      sgdVec3 ends[2];
      sgdVec3 velocity;
      int wire_id;
    };


    // The center of the cache.
    sgdVec3 cache_center;
    // Approximate ground radius.
    // In case the aircraft is too high above ground.
    double ground_radius;
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
    sgdVec3 reference_wgs84_point;
    double reference_vehicle_radius;
    bool found_ground;


    // Fills the environment cache with everything inside the sphere sp.
    void cache_fill(ssgBranch *branch, sgdMat4 xform,
                    sgdSphere* sp, sgdVec3 down, sgdSphere* wsp);

    // compute the ground property of this leaf.
    void putSurfaceLeafIntoCache(const sgdSphere *sp, const sgdMat4 xform,
                                 bool sphIsec, sgdVec3 down, ssgLeaf *l);

    void putLineLeafIntoCache(const sgdSphere *wsp, const sgdMat4 xform,
                              ssgLeaf *l);

    // Helper class to hold some properties of the ground triangle.
    struct GroundProperty {
      GroundProperty() : type(0) {}
      int type;
      int wire_id;
      sgVec3 vel;
      // not yet implemented ...
//       double loadCapacity;
    };

    // compute the ground property of this leaf.
    static GroundProperty extractGroundProperty( ssgLeaf* leaf );


    static void velocityTransformTriangle(double dt, Triangle& dst,
                                          const Triangle& src);
};

#endif
