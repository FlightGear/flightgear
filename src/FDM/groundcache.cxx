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

#include <osg/CullFace>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/TriangleFunctor>

#include <simgear/sg_inlines.h>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <AIModel/AICarrier.hxx>

#include "flight.hxx"
#include "groundcache.hxx"

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

template<typename T>
class SGExtendedTriangleFunctor : public osg::TriangleFunctor<T> {
public:
  // Ok, to be complete we should also implement the indexed variants
  // For now this one appears to be enough ...
  void drawArrays(GLenum mode, GLint first, GLsizei count)
  {
    if (_vertexArrayPtr==0 || count==0) return;

    const osg::Vec3* vlast;
    const osg::Vec3* vptr;
    switch(mode) {
    case(GL_LINES):
      vlast = &_vertexArrayPtr[first+count];
      for(vptr=&_vertexArrayPtr[first];vptr<vlast;vptr+=2)
        this->operator()(*(vptr),*(vptr+1),_treatVertexDataAsTemporary);
      break;
    case(GL_LINE_STRIP):
      vlast = &_vertexArrayPtr[first+count-1];
      for(vptr=&_vertexArrayPtr[first];vptr<vlast;++vptr)
        this->operator()(*(vptr),*(vptr+1),_treatVertexDataAsTemporary);
      break;
    case(GL_LINE_LOOP):
      vlast = &_vertexArrayPtr[first+count-1];
      for(vptr=&_vertexArrayPtr[first];vptr<vlast;++vptr)
        this->operator()(*(vptr),*(vptr+1),_treatVertexDataAsTemporary);
      this->operator()(_vertexArrayPtr[first+count-1],
                       _vertexArrayPtr[first],_treatVertexDataAsTemporary);
      break;
    default:
      osg::TriangleFunctor<T>::drawArrays(mode, first, count);
      break;
    }
  }
protected:
  using osg::TriangleFunctor<T>::_vertexArrayPtr;
  using osg::TriangleFunctor<T>::_treatVertexDataAsTemporary;
};

class GroundCacheFillVisitor : public osg::NodeVisitor {
public:
  
  /// class to just redirect triangles to the GroundCacheFillVisitor
  class GroundCacheFill {
  public:
    void setGroundCacheFillVisitor(GroundCacheFillVisitor* gcfv)
    { mGroundCacheFillVisitor = gcfv; }
    
    void operator () (const osg::Vec3& v1, const osg::Vec3& v2,
                      const osg::Vec3& v3, bool)
    { mGroundCacheFillVisitor->addTriangle(v1, v2, v3); }

    void operator () (const osg::Vec3& v1, const osg::Vec3& v2, bool)
    { mGroundCacheFillVisitor->addLine(v1, v2); }
    
  private:
    GroundCacheFillVisitor* mGroundCacheFillVisitor;
  };


  GroundCacheFillVisitor(FGGroundCache* groundCache,
                         const SGVec3d& down, 
                         const SGVec3d& cacheReference,
                         double cacheRadius,
                         double wireCacheRadius) :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
    mGroundCache(groundCache)
  {
    setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
    mDown = down;
    mLocalDown = down;
    sphIsec = true;
    mBackfaceCulling = false;
    mCacheReference = cacheReference;
    mLocalCacheReference = cacheReference;
    mCacheRadius = cacheRadius;
    mWireCacheRadius = wireCacheRadius;

    mTriangleFunctor.setGroundCacheFillVisitor(this);

    mGroundProperty.wire_id = -1;
    mGroundProperty.vel = SGVec3d(0, 0, 0);
    mGroundProperty.rot = SGVec3d(0, 0, 0);
    mGroundProperty.pivot = SGVec3d(0, 0, 0);
  }

  void updateCullMode(osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;

    osg::StateAttribute* stateAttribute;
    stateAttribute = stateSet->getAttribute(osg::StateAttribute::CULLFACE);
    if (!stateAttribute)
      return;
    osg::CullFace* cullFace = static_cast<osg::CullFace*>(stateAttribute);
    mBackfaceCulling = cullFace->getMode() == osg::CullFace::BACK;
  }

  bool enterBoundingSphere(const osg::BoundingSphere& bs)
  {
    if (!bs.valid())
      return false;

    SGVec3d cntr(osg::Vec3d(bs.center())*mLocalToGlobal);
    double rc = bs.radius() + mCacheRadius;
    // Ok, this node might intersect the cache. Visit it in depth.
    double centerDist2 = distSqr(mCacheReference, cntr);
    if (centerDist2 < rc*rc) {
      sphIsec = true;
    } else {
      // Check if the down direction touches the bounding sphere of the node
      // if so, do at least croase agl computations.
      // Ther other thing is that we must check if we are in range of
      // cats or wires
      double rw = bs.radius() + mWireCacheRadius;
      if (rw*rw < centerDist2 &&
          !intersectsInf(SGRayd(mCacheReference, mDown),
                         SGSphered(cntr, bs.radius())))
        return false;
      sphIsec = false;
    }

    return true;
  }

  bool enterNode(osg::Node& node)
  {
    if (!enterBoundingSphere(node.getBound()))
      return false;

    updateCullMode(node.getStateSet());

    FGGroundCache::GroundProperty& gp = mGroundProperty;
    // get some material information for use in the gear model
    gp.material = globals->get_matlib()->findMaterial(&node);
    if (gp.material) {
      gp.type = gp.material->get_solid() ? FGInterface::Solid : FGInterface::Water;
      return true;
    }
    gp.type = FGInterface::Unknown;
    osg::Referenced* base = node.getUserData();
    if (!base)
      return true;
    FGAICarrierHardware *ud =
      dynamic_cast<FGAICarrierHardware*>(base);
    if (!ud)
      return true;

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
  
    return true;
  }

  void fillWith(osg::Drawable* drawable)
  {
    bool oldSphIsec = sphIsec;
    if (!enterBoundingSphere(drawable->getBound()))
      return;

    bool oldBackfaceCulling = mBackfaceCulling;
    updateCullMode(drawable->getStateSet());

    drawable->accept(mTriangleFunctor);

    mBackfaceCulling = oldBackfaceCulling;
    sphIsec = oldSphIsec;
  }

  virtual void apply(osg::Geode& geode)
  {
    bool oldBackfaceCulling = mBackfaceCulling;
    bool oldSphIsec = sphIsec;
    FGGroundCache::GroundProperty oldGp = mGroundProperty;
    if (!enterNode(geode))
      return;

    for(unsigned i = 0; i < geode.getNumDrawables(); ++i)
      fillWith(geode.getDrawable(i));
    sphIsec = oldSphIsec;
    mGroundProperty = oldGp;
    mBackfaceCulling = oldBackfaceCulling;
  }

  virtual void apply(osg::Group& group)
  {
    bool oldBackfaceCulling = mBackfaceCulling;
    bool oldSphIsec = sphIsec;
    FGGroundCache::GroundProperty oldGp = mGroundProperty;
    if (!enterNode(group))
      return;
    traverse(group);
    sphIsec = oldSphIsec;
    mBackfaceCulling = oldBackfaceCulling;
    mGroundProperty = oldGp;
  }

  virtual void apply(osg::Transform& transform)
  {
    if (!enterNode(transform))
      return;
    bool oldBackfaceCulling = mBackfaceCulling;
    bool oldSphIsec = sphIsec;
    FGGroundCache::GroundProperty oldGp = mGroundProperty;
    /// transform the caches center to local coords
    osg::Matrix oldLocalToGlobal = mLocalToGlobal;
    osg::Matrix oldGlobalToLocal = mGlobalToLocal;
    transform.computeLocalToWorldMatrix(mLocalToGlobal, this);
    transform.computeWorldToLocalMatrix(mGlobalToLocal, this);

    SGVec3d oldLocalCacheReference = mLocalCacheReference;
    mLocalCacheReference.osg() = mCacheReference.osg()*mGlobalToLocal;
    SGVec3d oldLocalDown = mLocalDown;
    mLocalDown.osg() = osg::Matrixd::transform3x3(mDown.osg(), mGlobalToLocal);

    // walk the children
    traverse(transform);

    // Restore that one
    mLocalDown = oldLocalDown;
    mLocalCacheReference = oldLocalCacheReference;
    mLocalToGlobal = oldLocalToGlobal;
    mGlobalToLocal = oldGlobalToLocal;
    sphIsec = oldSphIsec;
    mBackfaceCulling = oldBackfaceCulling;
    mGroundProperty = oldGp;
  }
  
  void addTriangle(const osg::Vec3& v1, const osg::Vec3& v2,
                   const osg::Vec3& v3)
  {
    SGVec3d v[3] = {
      SGVec3d(v1),
      SGVec3d(v2),
      SGVec3d(v3)
    };
    
    // a bounding sphere in the node local system
    SGVec3d boundCenter = (1.0/3)*(v[0] + v[1] + v[2]);
    double boundRadius = std::max(distSqr(v[0], boundCenter),
                                  distSqr(v[1], boundCenter));
    boundRadius = std::max(boundRadius, distSqr(v[2], boundCenter));
    boundRadius = sqrt(boundRadius);

    SGRayd ray(mLocalCacheReference, mLocalDown);

    // if we are not in the downward cylinder bail out
    if (!intersectsInf(ray, SGSphered(boundCenter, boundRadius + mCacheRadius)))
      return;

    SGTriangled triangle(v);
    
    // The normal and plane in the node local coordinate system
    SGVec3d n = cross(triangle.getEdge(0), triangle.getEdge(1));
    if (0 < dot(mLocalDown, n)) {
      if (mBackfaceCulling) {
        // Surface points downwards, ignore for altitude computations.
        return;
      } else {
        triangle.flip();
      }
    }
    
    // Only check if the triangle is in the cache sphere if the plane
    // containing the triangle is near enough
    if (sphIsec) {
      double d = dot(n, v[0] - mLocalCacheReference);
      if (d*d < mCacheRadius*dot(n, n)) {
        // Check if the sphere around the vehicle intersects the sphere
        // around that triangle. If so, put that triangle into the cache.
        double r2 = boundRadius + mCacheRadius;
        if (distSqr(boundCenter, mLocalCacheReference) < r2*r2) {
          FGGroundCache::Triangle t;
          t.triangle.setBaseVertex(SGVec3d(v[0].osg()*mLocalToGlobal));
          t.triangle.setEdge(0, SGVec3d(osg::Matrixd::transform3x3(triangle.getEdge(0).osg(), mLocalToGlobal)));
          t.triangle.setEdge(1, SGVec3d(osg::Matrixd::transform3x3(triangle.getEdge(1).osg(), mLocalToGlobal)));
          
          t.sphere.setCenter(SGVec3d(boundCenter.osg()*mLocalToGlobal));
          t.sphere.setRadius(boundRadius);
          
          t.velocity = mGroundProperty.vel;
          t.rotation = mGroundProperty.rot;
          t.rotation_pivot = mGroundProperty.pivot;
          t.type = mGroundProperty.type;
          t.material = mGroundProperty.material;
          mGroundCache->triangles.push_back(t);
        }
      }
    }
    
    // In case the cache is empty, we still provide agl computations.
    // But then we use the old way of having a fixed elevation value for
    // the whole lifetime of this cache.
    SGVec3d isectpoint;
    if (intersects(isectpoint, triangle, ray, 1e-4)) {
      mGroundCache->found_ground = true;
      isectpoint.osg() = isectpoint.osg()*mLocalToGlobal;
      double this_radius = length(isectpoint);
      if (mGroundCache->ground_radius < this_radius) {
        mGroundCache->ground_radius = this_radius;
        mGroundCache->_type = mGroundProperty.type;
        mGroundCache->_material = mGroundProperty.material;
      }
    }
  }
  
  void addLine(const osg::Vec3& v1, const osg::Vec3& v2)
  {
    SGVec3d gv1(osg::Vec3d(v1)*mLocalToGlobal);
    SGVec3d gv2(osg::Vec3d(v2)*mLocalToGlobal);

    SGVec3d boundCenter = 0.5*(gv1 + gv2);
    double boundRadius = length(gv1 - boundCenter);

    if (distSqr(boundCenter, mCacheReference)
        < (boundRadius + mWireCacheRadius)*(boundRadius + mWireCacheRadius) ) {
      if (mGroundProperty.type == FGInterface::Wire) {
        FGGroundCache::Wire wire;
        wire.ends[0] = gv1;
        wire.ends[1] = gv2;
        wire.velocity = mGroundProperty.vel;
        wire.rotation = mGroundProperty.rot;
        wire.rotation_pivot = mGroundProperty.pivot;
        wire.wire_id = mGroundProperty.wire_id;

        mGroundCache->wires.push_back(wire);
      }
      if (mGroundProperty.type == FGInterface::Catapult) {
        FGGroundCache::Catapult cat;
        // Trick to get the ends in the right order.
        // Use the x axis in the original coordinate system. Choose the
        // most negative x-axis as the one pointing forward
        if (v1[0] > v2[0]) {
          cat.start = gv1;
          cat.end = gv2;
        } else {
          cat.start = gv2;
          cat.end = gv1;
        }
        cat.velocity = mGroundProperty.vel;
        cat.rotation = mGroundProperty.rot;
        cat.rotation_pivot = mGroundProperty.pivot;

        mGroundCache->catapults.push_back(cat);
      }
    }
  }

  SGExtendedTriangleFunctor<GroundCacheFill> mTriangleFunctor;
  FGGroundCache* mGroundCache;
  SGVec3d mCacheReference;
  double mCacheRadius;
  double mWireCacheRadius;
  osg::Matrix mLocalToGlobal;
  osg::Matrix mGlobalToLocal;
  SGVec3d mDown;
  SGVec3d mLocalDown;
  SGVec3d mLocalCacheReference;
  bool sphIsec;
  bool mBackfaceCulling;
  FGGroundCache::GroundProperty mGroundProperty;
};

FGGroundCache::FGGroundCache()
{
  ground_radius = 0.0;
  cache_ref_time = 0.0;
  wire_id = 0;
  reference_wgs84_point = SGVec3d(0, 0, 0);
  reference_vehicle_radius = 0.0;
  found_ground = false;
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

  if (dt*dt*dot(src.velocity, src.velocity) < SGLimitsd::epsilon())
    return;

  SGVec3d baseVert = dst.getBaseVertex();
  SGVec3d pivotoff = baseVert - src.rotation_pivot;
  baseVert += dt*(src.velocity + cross(src.rotation, pivotoff));
  dst.setBaseVertex(baseVert);
  dst.setEdge(0, dst.getEdge(0) + dt*cross(src.rotation, dst.getEdge(0)));
  dst.setEdge(1, dst.getEdge(1) + dt*cross(src.rotation, dst.getEdge(1)));
}


bool
FGGroundCache::prepare_ground_cache(double ref_time, const SGVec3d& pt,
                                    double rad)
{
  // Empty cache.
  ground_radius = 0.0;
  found_ground = false;
  triangles.resize(0);
  catapults.resize(0);
  wires.resize(0);

  // Store the parameters we used to build up that cache.
  reference_wgs84_point = pt;
  reference_vehicle_radius = rad;
  // Store the time reference used to compute movements of moving triangles.
  cache_ref_time = ref_time;

  // Get a normalized down vector valid for the whole cache
  SGQuatd hlToEc = SGQuatd::fromLonLat(SGGeod::fromCart(pt));
  down = hlToEc.rotate(SGVec3d(0, 0, 1));

  // Prepare sphere around the aircraft.
  double cacheRadius = rad;

  // Prepare bigger sphere around the aircraft.
  // This one is required for reliably finding wires we have caught but
  // have already left the hopefully smaller sphere for the ground reactions.
  const double max_wire_dist = 300.0;
  double wireCacheRadius = max_wire_dist < rad ? rad : max_wire_dist;

  // Walk the scene graph and extract solid ground triangles and carrier data.
  GroundCacheFillVisitor gcfv(this, down, pt, cacheRadius, wireCacheRadius);
  globals->get_scenery()->get_scene_graph()->accept(gcfv);

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
    pivotoff = catapults[i].start - catapults[i].rotation_pivot;
    rvel[0] = catapults[i].velocity + cross(catapults[i].rotation, pivotoff);
    pivotoff = catapults[i].end - catapults[i].rotation_pivot;
    rvel[1] = catapults[i].velocity + cross(catapults[i].rotation, pivotoff);

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
        SGVec3d pivotoff = pt - triangles[i].rotation_pivot;
        vel = triangles[i].velocity + cross(triangles[i].rotation, pivotoff);
        // Save the ground type.
        *type = triangles[i].type;
        *agl = dot(down, contact - dpt);
        if (material)
          *material = triangles[i].material;
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
      SGVec3d pivotoff = le[k] - wires[i].rotation_pivot;
      SGVec3d vel = wires[i].velocity + cross(wires[i].rotation, pivotoff);
      le[k] += t*vel;
    }
    SGLineSegmentd lineSegment(le[0], le[1]);
    
    for (int k=0; k<2; ++k) {
      if (intersects(triangle[k], lineSegment)) {
        SG_LOG(SG_FLIGHT,SG_INFO, "Caught wire");
        // Store the wire id.
        wire_id = wires[i].wire_id;
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
    if (wires[i].wire_id == wire_id) {
      for (size_t k = 0; k < 2; ++k) {
        SGVec3d pivotoff = wires[i].ends[k] - wires[i].rotation_pivot;
        vel[k] = wires[i].velocity + cross(wires[i].rotation, pivotoff);
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
