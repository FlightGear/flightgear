// groundcache.cxx -- carries a small subset of the scenegraph near the vehicle
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <float.h>

#include <utility>

#include <osg/CullFace>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <osg/TriangleFunctor>

#include <osgUtil/PolytopeIntersector>

#include <simgear/sg_inlines.h>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/PrimitiveUtils.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <AIModel/AICarrier.hxx>

#include "flight.hxx"
#include "groundcache.hxx"

using namespace osg;
using namespace osgUtil;
using namespace simgear;

void makePolytopeShaft(Polytope& polyt, const Vec3d& refPoint,
                       const Vec3d& direction, double radius)
{
    polyt.clear();
    // Choose best principal axis to start making orthogonal axis.
    Vec3d majorAxis;
    if (fabs(direction.x()) <= fabs(direction.y())) {
        if (fabs(direction.z()) <= fabs(direction.x()))
            majorAxis = Vec3d(0.0, 0.0, 1.0);
        else
            majorAxis = Vec3d(1.0, 0.0, 0.0);
    } else {
        if (fabs(direction.z()) <= fabs(direction.y()))
            majorAxis = Vec3d(0.0, 0.0, 1.0);
        else
            majorAxis = Vec3d(0.0, 1.0, 0.0);
    }
    Vec3d axis1 = majorAxis ^ direction;
    axis1.normalize();
    Vec3d axis2 = direction ^ axis1;

    polyt.add(Plane(-axis1, refPoint + axis1 * radius));
    polyt.add(Plane(axis1, refPoint - axis1 * radius));
    polyt.add(Plane(-axis2, refPoint + axis2 * radius));
    polyt.add(Plane(axis2 , refPoint - axis2 * radius));
}

void makePolytopeBox(Polytope& polyt, const Vec3d& center,
                     const Vec3d& direction, double radius)
{
    makePolytopeShaft(polyt, center, direction, radius);
    polyt.add(Plane(-direction, center + direction * radius));
    polyt.add(Plane(direction, center - direction * radius));
}

// Intersector for finding catapults and wires

class WireIntersector : public PolytopeIntersector
{
public:
    WireIntersector(const Polytope& polytope)
        : PolytopeIntersector(polytope), depth(0)
    {
        setDimensionMask(DimOne);
    }

    bool enter(const osg::Node& node)
    {
        if (!PolytopeIntersector::enter(node))
            return false;
        const Referenced* base = node.getUserData();
        if (base) {
            const FGAICarrierHardware *ud
                = dynamic_cast<const FGAICarrierHardware*>(base);
            if (ud)
                carriers.push_back(depth);
        }
        depth++;
        return true;
    }

    void leave()
    {
        depth--;
        if (!carriers.empty() && depth == carriers.back())
            carriers.pop_back();
    }

    void intersect(IntersectionVisitor& iv, Drawable* drawable)
    {
        if (carriers.empty())
            return;
        PolytopeIntersector::intersect(iv, drawable);
    }

    void reset()
    {
        carriers.clear();
    }
    std::vector<int> carriers;
    int depth;
};

/// Ok, variant that uses a infinite line istead of the ray.
/// also not that this only works if the ray direction is normalized.
static inline bool
intersectsInf(const SGRayd& ray, const SGSphered& sphere)
{
  SGVec3d r = sphere.getCenter() - ray.getOrigin();
  double projectedDistance = dot(r, ray.getDirection());
  double dist = dot(r, r) - projectedDistance * projectedDistance;
  return dist < sphere.getRadius2();
}

FGGroundCache::FGGroundCache() :
  ground_radius(0.0),
  _type(0),
  _material(0),
  cache_ref_time(0.0),
  wire_id(0),
  reference_wgs84_point(SGVec3d(0, 0, 0)),
  reference_vehicle_radius(0.0),
  down(0.0, 0.0, 0.0),
  found_ground(false)
{
}

FGGroundCache::~FGGroundCache()
{
}

inline void
FGGroundCache::velocityTransformTriangle(double dt,
                                         SGTriangled& dst, SGSphered& sdst,
                                         const FGGroundCache::Triangle& src)
{
  dst = src.triangle;
  sdst = src.sphere;

  if (dt*dt*dot(src.gp.vel, src.gp.vel) < SGLimitsd::epsilon())
    return;

  SGVec3d baseVert = dst.getBaseVertex();
  SGVec3d pivotoff = baseVert - src.gp.pivot;
  baseVert += dt*(src.gp.vel + cross(src.gp.rot, pivotoff));
  dst.setBaseVertex(baseVert);
  dst.setEdge(0, dst.getEdge(0) + dt*cross(src.gp.rot, dst.getEdge(0)));
  dst.setEdge(1, dst.getEdge(1) + dt*cross(src.gp.rot, dst.getEdge(1)));
}

void FGGroundCache::getGroundProperty(Drawable* drawable,
                                      const NodePath& nodePath,
                                      FGGroundCache::GroundProperty& gp,
                                      bool& backfaceCulling)
{
    gp.type = FGInterface::Unknown;
    gp.wire_id = 0;
    gp.vel = SGVec3d(0.0, 0.0, 0.0);
    gp.rot = SGVec3d(0.0, 0.0, 0.0);
    gp.pivot = SGVec3d(0.0, 0.0, 0.0);
    gp.material = 0;
    backfaceCulling = false;
    // XXX state set might be higher up in scene graph
    gp.material = SGMaterialLib::findMaterial(drawable->getStateSet());
    if (gp.material)
        gp.type = (gp.material->get_solid() ? FGInterface::Solid
                   : FGInterface::Water);
    for (NodePath::const_iterator iter = nodePath.begin(), e = nodePath.end();
         iter != e;
         ++iter) {
        Node* node = *iter;
        StateSet* stateSet = node->getStateSet();
        StateAttribute* stateAttribute = 0;        
        if (stateSet && (stateAttribute
                         = stateSet->getAttribute(StateAttribute::CULLFACE))) {
            backfaceCulling
                = (static_cast<osg::CullFace*>(stateAttribute)->getMode()
                   == CullFace::BACK);
        }
        
        // get some material information for use in the gear model
        Referenced* base = node->getUserData();
        if (!base)
            continue;
        FGAICarrierHardware *ud = dynamic_cast<FGAICarrierHardware*>(base);
        if (!ud)
            continue;
        switch (ud->type) {
        case FGAICarrierHardware::Wire:
            gp.type = FGInterface::Wire;
            gp.wire_id = ud->id;
            break;
        case FGAICarrierHardware::Catapult:
            gp.type = FGInterface::Catapult;
            break;
        default:
            gp.type = FGInterface::Solid;
            break;
        }
        // Copy the velocity from the carrier class.
        ud->carrier->getVelocityWrtEarth(gp.vel, gp.rot, gp.pivot);
        break;
    }
}

void FGGroundCache::getTriIntersectorResults(PolytopeIntersector* triInt)
{
    const PolytopeIntersector::Intersections& intersections
        = triInt->getIntersections();
    Drawable* lastDrawable = 0;
    RefMatrix* lastMatrix = 0;
    Matrix worldToLocal;
    GroundProperty gp;
    bool backfaceCulling = false;
    for (PolytopeIntersector::Intersections::const_iterator
             itr = intersections.begin(), e = intersections.end();
         itr != e;
         ++itr) {
        const PolytopeIntersector::Intersection& intr = *itr;
        if (intr.drawable.get() != lastDrawable) {
            getGroundProperty(intr.drawable.get(), intr.nodePath, gp,
                              backfaceCulling);
            lastDrawable = intr.drawable.get();
        }
        Primitive triPrim = getPrimitive(intr.drawable, intr.primitiveIndex);
        if (triPrim.numVerts != 3)
            continue;
        SGVec3d v[3] = { SGVec3d(triPrim.vertices[0]),
                         SGVec3d(triPrim.vertices[1]),
                         SGVec3d(triPrim.vertices[2])
        };
        RefMatrix* mat = intr.matrix.get();
        // If the drawable is the same then the intersection model
        // matrix should be the same, because it is only set by nodes
        // in the scene graph. However, do an extra test in case
        // something funny is going on with the drawable.
        if (mat != lastMatrix) {
            lastMatrix = mat;
            worldToLocal = Matrix::inverse(*mat);
        }
        SGVec3d localCacheReference;
        localCacheReference.osg() = reference_wgs84_point.osg() * worldToLocal;
        SGVec3d localDown;
        localDown.osg() = Matrixd::transform3x3(down.osg(), worldToLocal);
        // a bounding sphere in the node local system
        SGVec3d boundCenter = (1.0/3)*(v[0] + v[1] + v[2]);
        double boundRadius = std::max(distSqr(v[0], boundCenter),
                                      distSqr(v[1], boundCenter));
        boundRadius = std::max(boundRadius, distSqr(v[2], boundCenter));
        boundRadius = sqrt(boundRadius);
        SGRayd ray(localCacheReference, localDown);
        SGTriangled triangle(v);
        // The normal and plane in the node local coordinate system
        SGVec3d n = cross(triangle.getEdge(0), triangle.getEdge(1));
        if (0 < dot(localDown, n)) {
            if (backfaceCulling) {
                // Surface points downwards, ignore for altitude computations.
                continue;
            } else {
                triangle.flip();
            }
        }
    
        // Only check if the triangle is in the cache sphere if the plane
        // containing the triangle is near enough
        double d = dot(n, v[0] - localCacheReference);
        if (d*d < reference_vehicle_radius*dot(n, n)) {
            // Check if the sphere around the vehicle intersects the sphere
            // around that triangle. If so, put that triangle into the cache.
            double r2 = boundRadius + reference_vehicle_radius;
            if (distSqr(boundCenter, localCacheReference) < r2*r2) {
                FGGroundCache::Triangle t;
                t.triangle.setBaseVertex(SGVec3d(v[0].osg() * *mat));
                t.triangle.setEdge(0, SGVec3d(Matrixd::
                                              transform3x3(triangle
                                                           .getEdge(0).osg(),
                                                           *mat)));
                t.triangle.setEdge(1, SGVec3d(Matrixd::
                                              transform3x3(triangle
                                                           .getEdge(1).osg(),
                                                           *mat)));
                t.sphere.setCenter(SGVec3d(boundCenter.osg()* *mat));
                t.sphere.setRadius(boundRadius);
                t.gp = gp;
                triangles.push_back(t);
            }
        }
        // In case the cache is empty, we still provide agl computations.
        // But then we use the old way of having a fixed elevation value for
        // the whole lifetime of this cache.
        SGVec3d isectpoint;
        if (intersects(isectpoint, triangle, ray, 1e-4)) {
            found_ground = true;
            isectpoint.osg() = isectpoint.osg() * *mat;
            double this_radius = length(isectpoint);
            if (ground_radius < this_radius) {
                ground_radius = this_radius;
                _type = gp.type;
                _material = gp.material;
            }
        }        
    }
}

void FGGroundCache::getWireIntersectorResults(WireIntersector* wireInt,
                                              double wireCacheRadius)
{
    const WireIntersector::Intersections& intersections
        = wireInt->getIntersections();
    Drawable* lastDrawable = 0;
    GroundProperty gp;
    bool backfaceCulling = false;
    for (PolytopeIntersector::Intersections::const_iterator
             itr = intersections.begin(), e = intersections.end();
         itr != e;
         ++itr) {
        if (itr->drawable.get() != lastDrawable) {
            getGroundProperty(itr->drawable.get(), itr->nodePath, gp,
                              backfaceCulling);
            lastDrawable = itr->drawable.get();
        }
        Primitive linePrim = getPrimitive(itr->drawable, itr->primitiveIndex);
        if (linePrim.numVerts != 2)
            continue;
        RefMatrix* mat = itr->matrix.get();
        SGVec3d gv1(osg::Vec3d(linePrim.vertices[0]) * *mat);
        SGVec3d gv2(osg::Vec3d(linePrim.vertices[1]) * *mat);

        SGVec3d boundCenter = 0.5*(gv1 + gv2);
        double boundRadius = length(gv1 - boundCenter);

        if (distSqr(boundCenter, reference_wgs84_point)
            < (boundRadius + wireCacheRadius)*(boundRadius + wireCacheRadius)) {
            if (gp.type == FGInterface::Wire) {
                FGGroundCache::Wire wire;
                wire.ends[0] = gv1;
                wire.ends[1] = gv2;
                wire.gp = gp;
                wires.push_back(wire);
            } else if (gp.type == FGInterface::Catapult) {
                FGGroundCache::Catapult cat;
                // Trick to get the ends in the right order.
                // Use the x axis in the original coordinate system. Choose the
                // most negative x-axis as the one pointing forward
                if (linePrim.vertices[0][0] > linePrim.vertices[1][0]) {
                    cat.start = gv1;
                    cat.end = gv2;
                } else {
                    cat.start = gv2;
                    cat.end = gv1;
                }
                cat.gp = gp;
                catapults.push_back(cat);
            }
        }
        
    }
}

bool
FGGroundCache::prepare_ground_cache(double ref_time, const SGVec3d& pt,
                                    double rad)
{
  // Empty cache.
  found_ground = false;
  SGGeod geodPt = SGGeod::fromCart(pt);
  // Don't blow away the cache ground_radius and stuff if there's no
  // scenery
  if (!globals->get_tile_mgr()->scenery_available(geodPt.getLatitudeDeg(),
                                                  geodPt.getLongitudeDeg(),
                                                  rad))
    return false;
  ground_radius = 0.0;
  triangles.resize(0);
  catapults.resize(0);
  wires.resize(0);

  // Store the parameters we used to build up that cache.
  reference_wgs84_point = pt;
  reference_vehicle_radius = rad;
  // Store the time reference used to compute movements of moving triangles.
  cache_ref_time = ref_time;

  // Get a normalized down vector valid for the whole cache
  SGQuatd hlToEc = SGQuatd::fromLonLat(geodPt);
  down = hlToEc.rotate(SGVec3d(0, 0, 1));

  // Prepare sphere around the aircraft.
  double cacheRadius = rad;

  // Prepare bigger sphere around the aircraft.
  // This one is required for reliably finding wires we have caught but
  // have already left the hopefully smaller sphere for the ground reactions.
  const double max_wire_dist = 300.0;
  double wireCacheRadius = max_wire_dist < rad ? rad : max_wire_dist;

  Polytope triPolytope;
  makePolytopeShaft(triPolytope, pt.osg(), down.osg(), cacheRadius);
  ref_ptr<PolytopeIntersector> triIntersector
      = new PolytopeIntersector(triPolytope);
  triIntersector->setDimensionMask(PolytopeIntersector::DimTwo);
  Polytope wirePolytope;
  makePolytopeBox(wirePolytope, pt.osg(), down.osg(), wireCacheRadius);
  ref_ptr<WireIntersector> wireIntersector = new WireIntersector(wirePolytope);
  wireIntersector->setDimensionMask(PolytopeIntersector::DimOne);
  ref_ptr<IntersectorGroup> intersectors = new IntersectorGroup;
  intersectors->addIntersector(triIntersector.get());
  intersectors->addIntersector(wireIntersector.get());

  // Walk the scene graph and extract solid ground triangles and
  // carrier data.
  IntersectionVisitor iv(intersectors);
  iv.setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
  globals->get_scenery()->get_scene_graph()->accept(iv);
  getTriIntersectorResults(triIntersector.get());
  getWireIntersectorResults(wireIntersector.get(), wireCacheRadius);
  
  // some stats
  SG_LOG(SG_FLIGHT,SG_DEBUG, "prepare_ground_cache(): ac radius = " << rad
         << ", # triangles = " << triangles.size()
         << ", # wires = " << wires.size()
         << ", # catapults = " << catapults.size()
         << ", ground_radius = " << ground_radius );

  // If the ground radius is still below 5e6 meters, then we do not yet have
  // any scenery.
  found_ground = found_ground && 5e6 < ground_radius;
  if (!found_ground)
    SG_LOG(SG_FLIGHT, SG_WARN, "prepare_ground_cache(): trying to build cache "
           "without any scenery below the aircraft" );

  return found_ground;
}

bool
FGGroundCache::is_valid(double& ref_time, SGVec3d& pt, double& rad)
{
  pt = reference_wgs84_point;
  rad = reference_vehicle_radius;
  ref_time = cache_ref_time;
  return found_ground;
}

double
FGGroundCache::get_cat(double t, const SGVec3d& dpt,
                       SGVec3d end[2], SGVec3d vel[2])
{
  // start with a distance of 1e10 meters...
  double dist = 1e10;

  // Time difference to the reference time.
  t -= cache_ref_time;

  size_t sz = catapults.size();
  for (size_t i = 0; i < sz; ++i) {
    SGVec3d pivotoff, rvel[2];
    pivotoff = catapults[i].start - catapults[i].gp.pivot;
    rvel[0] = catapults[i].gp.vel + cross(catapults[i].gp.rot, pivotoff);
    pivotoff = catapults[i].end - catapults[i].gp.pivot;
    rvel[1] = catapults[i].gp.vel + cross(catapults[i].gp.rot, pivotoff);

    SGVec3d thisEnd[2];
    thisEnd[0] = catapults[i].start + t*rvel[0];
    thisEnd[1] = catapults[i].end + t*rvel[1];

    double this_dist = distSqr(SGLineSegmentd(thisEnd[0], thisEnd[1]), dpt);
    if (this_dist < dist) {
      SG_LOG(SG_FLIGHT,SG_INFO, "Found catapult "
             << this_dist << " meters away");
      dist = this_dist;
        
      end[0] = thisEnd[0];
      end[1] = thisEnd[1];
      vel[0] = rvel[0];
      vel[1] = rvel[1];
    }
  }

  // At the end take the root, we only computed squared distances ...
  return sqrt(dist);
}

bool
FGGroundCache::get_agl(double t, const SGVec3d& dpt, double max_altoff,
                       SGVec3d& contact, SGVec3d& normal, SGVec3d& vel,
                       int *type, const SGMaterial** material, double *agl)
{
  bool ret = false;

  *type = FGInterface::Unknown;
//   *agl = 0.0;
  if (material)
    *material = 0;
  vel = SGVec3d(0, 0, 0);
  contact = SGVec3d(0, 0, 0);
  normal = SGVec3d(0, 0, 0);

  // Time difference to th reference time.
  t -= cache_ref_time;

  // The double valued point we start to search for intersection.
  SGVec3d pt = dpt;
  // shift the start of our ray by maxaltoff upwards
  SGRayd ray(pt - max_altoff*down, down);

  // Initialize to something sensible
  double current_radius = 0.0;

  size_t sz = triangles.size();
  for (size_t i = 0; i < sz; ++i) {
    SGSphered sphere;
    SGTriangled triangle;
    velocityTransformTriangle(t, triangle, sphere, triangles[i]);
    if (!intersectsInf(ray, sphere))
      continue;

    // Check for intersection.
    SGVec3d isecpoint;
    if (intersects(isecpoint, triangle, ray, 1e-4)) {
      // Compute the vector from pt to the intersection point ...
      SGVec3d off = isecpoint - pt;
      // ... and check if it is too high or not

      // compute the radius, good enough approximation to take the geocentric radius
      double radius = dot(isecpoint, isecpoint);
      if (current_radius < radius) {
        current_radius = radius;
        ret = true;
        // Save the new potential intersection point.
        contact = isecpoint;
        // The first three values in the vector are the plane normal.
        normal = triangle.getNormal();
        // The velocity wrt earth.
        SGVec3d pivotoff = pt - triangles[i].gp.pivot;
        vel = triangles[i].gp.vel + cross(triangles[i].gp.rot, pivotoff);
        // Save the ground type.
        *type = triangles[i].gp.type;
        *agl = dot(down, contact - dpt);
        if (material)
          *material = triangles[i].gp.material;
      }
    }
  }

  if (ret)
    return true;

  // Whenever we did not have a ground triangle for the requested point,
  // take the ground level we found during the current cache build.
  // This is as good as what we had before for agl.
  double r = length(dpt);
  contact = dpt;
  contact *= ground_radius/r;
  normal = -down;
  vel = SGVec3d(0, 0, 0);
  
  // The altitude is the distance of the requested point from the
  // contact point.
  *agl = dot(down, contact - dpt);
  *type = _type;
  if (material)
    *material = _material;

  return ret;
}

bool FGGroundCache::caught_wire(double t, const SGVec3d pt[4])
{
  size_t sz = wires.size();
  if (sz == 0)
    return false;

  // Time difference to the reference time.
  t -= cache_ref_time;

  // Build the two triangles spanning the area where the hook has moved
  // during the past step.
  SGTriangled triangle[2];
  triangle[0].set(pt[0], pt[1], pt[2]);
  triangle[1].set(pt[0], pt[2], pt[3]);

  // Intersect the wire lines with each of these triangles.
  // You have caught a wire if they intersect.
  for (size_t i = 0; i < sz; ++i) {
    SGVec3d le[2];
    for (int k = 0; k < 2; ++k) {
      le[k] = wires[i].ends[k];
      SGVec3d pivotoff = le[k] - wires[i].gp.pivot;
      SGVec3d vel = wires[i].gp.vel + cross(wires[i].gp.rot, pivotoff);
      le[k] += t*vel;
    }
    SGLineSegmentd lineSegment(le[0], le[1]);
    
    for (int k=0; k<2; ++k) {
      if (intersects(triangle[k], lineSegment)) {
        SG_LOG(SG_FLIGHT,SG_INFO, "Caught wire");
        // Store the wire id.
        wire_id = wires[i].gp.wire_id;
        return true;
      }
    }
  }

  return false;
}

bool FGGroundCache::get_wire_ends(double t, SGVec3d end[2], SGVec3d vel[2])
{
  // Fast return if we do not have an active wire.
  if (wire_id < 0)
    return false;

  // Time difference to the reference time.
  t -= cache_ref_time;

  // Search for the wire with the matching wire id.
  size_t sz = wires.size();
  for (size_t i = 0; i < sz; ++i) {
    if (wires[i].gp.wire_id == wire_id) {
      for (size_t k = 0; k < 2; ++k) {
        SGVec3d pivotoff = wires[i].ends[k] - wires[i].gp.pivot;
        vel[k] = wires[i].gp.vel + cross(wires[i].gp.rot, pivotoff);
        end[k] = wires[i].ends[k] + t*vel[k];
      }
      return true;
    }
  }

  return false;
}

void FGGroundCache::release_wire(void)
{
  wire_id = -1;
}
