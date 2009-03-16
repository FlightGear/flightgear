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
#include <simgear/math/SGGeometry.hxx>
#include <simgear/scene/bvh/BVHNode.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

// #define GROUNDCACHE_DEBUG
#ifdef GROUNDCACHE_DEBUG
#include <osg/Group>
#include <osg/ref_ptr>
#include <simgear/timing/timestamp.hxx>
#endif

class SGMaterial;
namespace simgear {
class BVHLineGeometry;
}

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
    bool prepare_ground_cache(double startSimTime, double endSimTime,
                              const SGVec3d& pt, double rad);

    // Returns true if the cache is valid.
    // Also the reference time, point and radius values where the cache
    // is valid for are returned.
    bool is_valid(double& ref_time, SGVec3d& pt, double& rad);

    // Returns the unit down vector at the ground cache
    const SGVec3d& get_down() const
    { return down; }

    // The time offset that originates from a simtime different than zero
    // at initialization time of the fdm.
    double get_cache_time_offset() const
    { return cache_time_offset; }
    void set_cache_time_offset(double time_offset)
    { cache_time_offset = time_offset; }

    bool get_body(double t, SGMatrixd& bodyToWorld, SGVec3d& linearVel,
                  SGVec3d& angularVel, simgear::BVHNode::Id id);

    // Return the nearest catapult to the given point
    // pt in wgs84 coordinates.
    double get_cat(double t, const SGVec3d& pt,
                   SGVec3d end[2], SGVec3d vel[2]);
  

    // Return the altitude above ground below the wgs84 point pt
    // Search for highest triangle not higher than pt + max_altoff.
    // Return ground properties like the maximum load
    // this kind kind of ground can carry, the friction factor between
    // 0 and 1 which can be used to model lower friction with wet runways.
    bool get_agl(double t, const SGVec3d& pt, SGVec3d& contact,
                 SGVec3d& normal, SGVec3d& linearVel, SGVec3d& angularVel,
                 simgear::BVHNode::Id& id, const SGMaterial*& material);

    bool get_nearest(double t, const SGVec3d& pt, double maxDist,
                     SGVec3d& contact, SGVec3d& linearVel, SGVec3d& angularVel,
                     simgear::BVHNode::Id& id, const SGMaterial*& material);

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
    class CacheFill;
    class BodyFinder;
    class CatapultFinder;
    class WireIntersector;
    class WireFinder;

    // Approximate ground radius.
    // In case the aircraft is too high above ground.
    double _altitude;
    // the simgear material reference, contains friction coeficients ...
    const SGMaterial* _material;
    // The time reference for later call to intersection test routines.
    // Is required since we will have moving triangles in carriers.
    double cache_ref_time;
    // The time the cache was initialized.
    double cache_time_offset;
    // The wire to track.
    const simgear::BVHLineGeometry* _wire;

    // The point and radius where the cache is built around.
    // That are the arguments that were given to prepare_ground_cache.
    SGVec3d reference_wgs84_point;
    double reference_vehicle_radius;
    SGVec3d down;
    bool found_ground;

    SGSharedPtr<simgear::BVHNode> _localBvhTree;

#ifdef GROUNDCACHE_DEBUG
    SGTimeStamp _lookupTime;
    unsigned _lookupCount;
    SGTimeStamp _buildTime;
    unsigned _buildCount;

    osg::ref_ptr<osg::Group> _group;
#endif
};

#endif
