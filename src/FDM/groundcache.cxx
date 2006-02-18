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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <float.h>

#include <plib/sg.h>

#include <simgear/sg_inlines.h>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <AIModel/AICarrier.hxx>

#include "flight.hxx"
#include "groundcache.hxx"

// Specialized version of sgMultMat4 needed because of mixed matrix
// types
static inline void fgMultMat4(sgdMat4 dst, sgdMat4 m1, sgMat4 m2) {
    for ( int j = 0 ; j < 4 ; j++ ) {
        dst[0][j] = m2[0][0] * m1[0][j] +
                    m2[0][1] * m1[1][j] +
                    m2[0][2] * m1[2][j] +
                    m2[0][3] * m1[3][j] ;

        dst[1][j] = m2[1][0] * m1[0][j] +
                    m2[1][1] * m1[1][j] +
                    m2[1][2] * m1[2][j] +
                    m2[1][3] * m1[3][j] ;

        dst[2][j] = m2[2][0] * m1[0][j] +
                    m2[2][1] * m1[1][j] +
                    m2[2][2] * m1[2][j] +
                    m2[2][3] * m1[3][j] ;

        dst[3][j] = m2[3][0] * m1[0][j] +
                    m2[3][1] * m1[1][j] +
                    m2[3][2] * m1[2][j] +
                    m2[3][3] * m1[3][j] ;
    }
}

static inline bool fgdPointInTriangle( sgdVec3 point, sgdVec3 tri[3] )
{
    sgdVec3 dif;

    // Some tolerance in meters we accept a point to be outside of the triangle
    // and still return that it is inside.
    SGDfloat eps = 1e-2;
    SGDfloat min, max;
    // punt if outside bouding cube
    SG_MIN_MAX3 ( min, max, tri[0][0], tri[1][0], tri[2][0] );
    if( (point[0] < min - eps) || (point[0] > max + eps) )
        return false;
    dif[0] = max - min;

    SG_MIN_MAX3 ( min, max, tri[0][1], tri[1][1], tri[2][1] );
    if( (point[1] < min - eps) || (point[1] > max + eps) )
        return false;
    dif[1] = max - min;

    SG_MIN_MAX3 ( min, max, tri[0][2], tri[1][2], tri[2][2] );
    if( (point[2] < min - eps) || (point[2] > max + eps) )
        return false;
    dif[2] = max - min;

    // drop the smallest dimension so we only have to work in 2d.
    SGDfloat min_dim = SG_MIN3 (dif[0], dif[1], dif[2]);
    SGDfloat x1, y1, x2, y2, x3, y3, rx, ry;
    if ( fabs(min_dim-dif[0]) <= DBL_EPSILON ) {
        // x is the smallest dimension
        x1 = point[1];
        y1 = point[2];
        x2 = tri[0][1];
        y2 = tri[0][2];
        x3 = tri[1][1];
        y3 = tri[1][2];
        rx = tri[2][1];
        ry = tri[2][2];
    } else if ( fabs(min_dim-dif[1]) <= DBL_EPSILON ) {
        // y is the smallest dimension
        x1 = point[0];
        y1 = point[2];
        x2 = tri[0][0];
        y2 = tri[0][2];
        x3 = tri[1][0];
        y3 = tri[1][2];
        rx = tri[2][0];
        ry = tri[2][2];
    } else if ( fabs(min_dim-dif[2]) <= DBL_EPSILON ) {
        // z is the smallest dimension
        x1 = point[0];
        y1 = point[1];
        x2 = tri[0][0];
        y2 = tri[0][1];
        x3 = tri[1][0];
        y3 = tri[1][1];
        rx = tri[2][0];
        ry = tri[2][1];
    } else {
        // all dimensions are really small so lets call it close
        // enough and return a successful match
        return true;
    }

    // check if intersection point is on the same side of p1 <-> p2 as p3
    SGDfloat tmp = (y2 - y3);
    SGDfloat tmpn = (x2 - x3);
    int side1 = SG_SIGN (tmp * (rx - x3) + (y3 - ry) * tmpn);
    int side2 = SG_SIGN (tmp * (x1 - x3) + (y3 - y1) * tmpn
                         + side1 * eps * fabs(tmpn));
    if ( side1 != side2 ) {
        // printf("failed side 1 check\n");
        return false;
    }

    // check if intersection point is on correct side of p2 <-> p3 as p1
    tmp = (y3 - ry);
    tmpn = (x3 - rx);
    side1 = SG_SIGN (tmp * (x2 - rx) + (ry - y2) * tmpn);
    side2 = SG_SIGN (tmp * (x1 - rx) + (ry - y1) * tmpn
                     + side1 * eps * fabs(tmpn));
    if ( side1 != side2 ) {
        // printf("failed side 2 check\n");
        return false;
    }

    // check if intersection point is on correct side of p1 <-> p3 as p2
    tmp = (y2 - ry);
    tmpn = (x2 - rx);
    side1 = SG_SIGN (tmp * (x3 - rx) + (ry - y3) * tmpn);
    side2 = SG_SIGN (tmp * (x1 - rx) + (ry - y1) * tmpn
                     + side1 * eps * fabs(tmpn));
    if ( side1 != side2 ) {
        // printf("failed side 3  check\n");
        return false;
    }

    return true;
}

// Test if the line given by the point on the line pt_on_line and the
// line direction dir intersects the sphere sp.
// Adapted from plib.
static inline bool
fgdIsectSphereInfLine(const sgdSphere& sp,
                      const sgdVec3 pt_on_line, const sgdVec3 dir)
{
  sgdVec3 r;
  sgdSubVec3( r, sp.getCenter(), pt_on_line ) ;

  SGDfloat projectedDistance = sgdScalarProductVec3(r, dir);
 
  SGDfloat dist = sgdScalarProductVec3 ( r, r ) -
    projectedDistance * projectedDistance;

  SGDfloat radius = sp.getRadius();
  return dist < radius*radius;
}

FGGroundCache::FGGroundCache()
{
  sgdSetVec3(cache_center, 0.0, 0.0, 0.0);
  ground_radius = 0.0;
  cache_ref_time = 0.0;
  wire_id = 0;
  sgdSetVec3(reference_wgs84_point, 0.0, 0.0, 0.0);
  reference_vehicle_radius = 0.0;
  found_ground = false;
}

FGGroundCache::~FGGroundCache()
{
}

FGGroundCache::GroundProperty
FGGroundCache::extractGroundProperty( ssgLeaf* l )
{
  // FIXME: Do more ...
  // Idea: have a get_globals() function which knows about that stuff.
  // Or most probably read that from a configuration file,
  // from property tree or whatever ...
  
  // Get ground dependent data.
  GroundProperty gp;
  gp.wire_id = -1;
  
  FGAICarrierHardware *ud =
    dynamic_cast<FGAICarrierHardware*>(l->getUserData());
  if (ud) {
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
    ud->carrier->getVelocityWrtEarth( gp.vel, gp.rot, gp.pivot );
  }

  else {

    // Initialize velocity field.
    sgdSetVec3( gp.vel, 0.0, 0.0, 0.0 );
    sgdSetVec3( gp.rot, 0.0, 0.0, 0.0 );
    sgdSetVec3( gp.pivot, 0.0, 0.0, 0.0 );
  }
  
  // Get the texture name and decide what ground type we have.
  ssgState *st = l->getState();
  if (st != NULL && st->isAKindOf(ssgTypeSimpleState())) {
    ssgSimpleState *ss = (ssgSimpleState*)st;
    SGPath fullPath( ss->getTextureFilename() ? ss->getTextureFilename(): "" );
    string file = fullPath.file();
    SGPath dirPath(fullPath.dir());
    string category = dirPath.file();
    
    if (category == "Runway")
      gp.type = FGInterface::Solid;
    else {
      if (file == "asphault.rgb" || file == "airport.rgb")
        gp.type = FGInterface::Solid;
      else if (file == "water.rgb" || file == "water-lake.rgb")
        gp.type = FGInterface::Water;
      else if (file == "forest.rgb" || file == "cropwood.rgb")
        gp.type = FGInterface::Forest;
    }
  }
  
  return gp;
}

void
FGGroundCache::putLineLeafIntoCache(const sgdSphere *wsp, const sgdMat4 xform,
                                    ssgLeaf *l)
{
  GroundProperty gp = extractGroundProperty(l);

  // Lines must have special meanings.
  // Wires and catapults are done with lines.
  int nl = l->getNumLines();
  for (int i = 0; i < nl; ++i) {
    sgdSphere sphere;
    sphere.empty();
    sgdVec3 ends[2];
    short v[2];
    l->getLine(i, v, v+1 );
    for (int k=0; k<2; ++k) {
      sgdSetVec3(ends[k], l->getVertex(v[k]));
      sgdXformPnt3(ends[k], xform);
      sphere.extend(ends[k]);
    }

    if (wsp->intersects( &sphere )) {
      if (gp.type == FGInterface::Wire) {
        Wire wire;
        sgdCopyVec3(wire.ends[0], ends[0]);
        sgdCopyVec3(wire.ends[1], ends[1]);
        sgdCopyVec3(wire.velocity, gp.vel);
        sgdCopyVec3(wire.rotation, gp.rot);
        sgdSubVec3(wire.rotation_pivot, gp.pivot, cache_center);
        wire.wire_id = gp.wire_id;

        wires.push_back(wire);
      }
      if (gp.type == FGInterface::Catapult) {
        Catapult cat;
        sgdCopyVec3(cat.start, ends[0]);
        sgdCopyVec3(cat.end, ends[1]);
        sgdCopyVec3(cat.velocity, gp.vel);
        sgdCopyVec3(cat.rotation, gp.rot);
        sgdSubVec3(cat.rotation_pivot, gp.pivot, cache_center);

        catapults.push_back(cat);
      }
    }
  }
}

void
FGGroundCache::putSurfaceLeafIntoCache(const sgdSphere *sp,
                                       const sgdMat4 xform, bool sphIsec,
                                       sgdVec3 down, ssgLeaf *l)
{
  GroundProperty gp = extractGroundProperty(l);

  int nt = l->getNumTriangles();
  for (int i = 0; i < nt; ++i) {
    Triangle t;
    t.sphere.empty();
    short v[3];
    l->getTriangle(i, &v[0], &v[1], &v[2]);
    for (int k = 0; k < 3; ++k) {
      sgdSetVec3(t.vertices[k], l->getVertex(v[k]));
      sgdXformPnt3(t.vertices[k], xform);
      t.sphere.extend(t.vertices[k]);
    }

    sgdMakePlane(t.plane, t.vertices[0], t.vertices[1], t.vertices[2]);
    SGDfloat dot = sgdScalarProductVec3(down, t.plane);
    if (dot > 0) {
      if (!l->getCullFace()) {
        // Surface points downwards, ignore for altitude computations.
        continue;
      } else
        sgdScaleVec4( t.plane, -1 );
    }

    // Check if the sphere around the vehicle intersects the sphere
    // around that triangle. If so, put that triangle into the cache.
    if (sphIsec && sp->intersects(&t.sphere)) {
      sgdCopyVec3(t.velocity, gp.vel);
      sgdCopyVec3(t.rotation, gp.rot);
      sgdSubVec3(t.rotation_pivot, gp.pivot, cache_center);
      t.type = gp.type;
      triangles.push_back(t);
    }
    
    // In case the cache is empty, we still provide agl computations.
    // But then we use the old way of having a fixed elevation value for
    // the whole lifetime of this cache.
    if ( fgdIsectSphereInfLine(t.sphere, sp->getCenter(), down) ) {
      sgdVec3 tmp;
      sgdSetVec3(tmp, sp->center[0], sp->center[1], sp->center[2]);
      sgdVec3 isectpoint;
      if ( sgdIsectInfLinePlane( isectpoint, tmp, down, t.plane ) &&
           fgdPointInTriangle( isectpoint, t.vertices ) ) {
        // Compute the offset to the ground cache midpoint
        sgdVec3 off;
        sgdSubVec3(off, isectpoint, tmp);
        // Only accept the altitude if the intersection point is below the
        // ground cache midpoint
        if (0 < sgdScalarProductVec3( off, down )) {
          found_ground = true;
          sgdAddVec3(isectpoint, cache_center);
          double this_radius = sgdLengthVec3(isectpoint);
          if (ground_radius < this_radius)
            ground_radius = this_radius;
        }
      }
    }
  }
}

inline void
FGGroundCache::velocityTransformTriangle(double dt,
                                         FGGroundCache::Triangle& dst,
                                         const FGGroundCache::Triangle& src)
{
  sgdCopyVec3(dst.vertices[0], src.vertices[0]);
  sgdCopyVec3(dst.vertices[1], src.vertices[1]);
  sgdCopyVec3(dst.vertices[2], src.vertices[2]);

  sgdCopyVec4(dst.plane, src.plane);
  
  sgdCopyVec3(dst.sphere.center, src.sphere.center);
  dst.sphere.radius = src.sphere.radius;

  sgdCopyVec3(dst.velocity, src.velocity);
  sgdCopyVec3(dst.rotation, src.rotation);
  sgdCopyVec3(dst.rotation_pivot, src.rotation_pivot);

  dst.type = src.type;

  if (dt*sgdLengthSquaredVec3(src.velocity) != 0) {
    sgdVec3 pivotoff, vel;
    for (int i = 0; i < 3; ++i) {
      sgdSubVec3(pivotoff, src.vertices[i], src.rotation_pivot);
      sgdVectorProductVec3(vel, src.rotation, pivotoff);
      sgdAddVec3(vel, src.velocity);
      sgdAddScaledVec3(dst.vertices[i], vel, dt);
    }
    
    // Transform the plane equation
    sgdSubVec3(pivotoff, dst.plane, src.rotation_pivot);
    sgdVectorProductVec3(vel, src.rotation, pivotoff);
    sgdAddVec3(vel, src.velocity);
    dst.plane[3] += dt*sgdScalarProductVec3(dst.plane, vel);

    sgdAddScaledVec3(dst.sphere.center, src.velocity, dt);
  }
}

void
FGGroundCache::cache_fill(ssgBranch *branch, sgdMat4 xform,
                          sgdSphere* sp, sgdVec3 down, sgdSphere* wsp)
{
  // Travel through all kids.
  ssgEntity *e;
  for ( e = branch->getKid(0); e != NULL ; e = branch->getNextKid() ) {
    if ( !(e->getTraversalMask() & SSGTRAV_HOT) )
      continue;
    if ( e->getBSphere()->isEmpty() )
      continue;
    
    // We need to check further if either the sphere around the branch
    // intersects the sphere around the aircraft or the line downwards from
    // the aircraft intersects the branchs sphere.
    sgdSphere esphere;
    sgdSetVec3(esphere.center, e->getBSphere()->center);
    esphere.radius = e->getBSphere()->radius;
    esphere.orthoXform(xform);
    bool wspIsec = wsp->intersects(&esphere);
    bool downIsec = fgdIsectSphereInfLine(esphere, sp->getCenter(), down);
    if (!wspIsec && !downIsec)
      continue;

    // For branches collect up the transforms to reach that branch and
    // call cache_fill recursively.
    if ( e->isAKindOf( ssgTypeBranch() ) ) {
      ssgBranch *b = (ssgBranch *)e;
      if ( b->isAKindOf( ssgTypeTransform() ) ) {
        // Collect up the transforms required to reach that part of
        // the branch.
        sgMat4 xform2;
        sgMakeIdentMat4( xform2 );
        ssgTransform *t = (ssgTransform*)b;
        t->getTransform( xform2 );
        sgdMat4 xform3;
        fgMultMat4(xform3, xform, xform2);
        cache_fill( b, xform3, sp, down, wsp );
      } else
        cache_fill( b, xform, sp, down, wsp );
    }
   
    // For leafs, check each triangle for intersection.
    // This will minimize the number of vertices/triangles in the cache.
    else if (e->isAKindOf(ssgTypeLeaf())) {
      // Since we reach that leaf if we have an intersection with the
      // most probably bigger wire/catapult cache sphere, we need to check
      // that here, if the smaller cache for the surface has a chance for hits.
      // Also, if the spheres do not intersect compute a coarse agl value
      // by following the line downwards originating at the aircraft.
      bool spIsec = sp->intersects(&esphere);
      putSurfaceLeafIntoCache(sp, xform, spIsec, down, (ssgLeaf *)e);

      // If we are here, we need to put all special hardware here into
      // the cache.
      if (wspIsec)
        putLineLeafIntoCache(wsp, xform, (ssgLeaf *)e);
    }
  }
}

bool
FGGroundCache::prepare_ground_cache(double ref_time, const double pt[3],
                                    double rad)
{
  // Empty cache.
  ground_radius = 0.0;
  found_ground = false;
  triangles.resize(0);
  catapults.resize(0);
  wires.resize(0);

  // Store the parameters we used to build up that cache.
  sgdCopyVec3(reference_wgs84_point, pt);
  reference_vehicle_radius = rad;
  // Store the time reference used to compute movements of moving triangles.
  cache_ref_time = ref_time;

  // Decide where we put the scenery center.
  Point3D old_cntr = globals->get_scenery()->get_center();
  Point3D cntr(pt[0], pt[1], pt[2]);
  // Only move the cache center if it is unacceptable far away.
  if (40*40 < old_cntr.distance3Dsquared(cntr))
    globals->get_scenery()->set_center(cntr);
  else
    cntr = old_cntr;

  // The center of the cache.
  sgdSetVec3(cache_center, cntr[0], cntr[1], cntr[2]);
  
  sgdVec3 ptoff;
  sgdSubVec3(ptoff, pt, cache_center);
  // Prepare sphere around the aircraft.
  sgdSphere acSphere;
  acSphere.setRadius(rad);
  acSphere.setCenter(ptoff);

  // Prepare bigger sphere around the aircraft.
  // This one is required for reliably finding wires we have caught but
  // have already left the hopefully smaller sphere for the ground reactions.
  const double max_wire_dist = 300.0;
  sgdSphere wireSphere;
  wireSphere.setRadius(max_wire_dist < rad ? rad : max_wire_dist);
  wireSphere.setCenter(ptoff);

  // Down vector. Is used for croase agl computations when we are far enough
  // from ground that we have an empty cache.
  sgdVec3 down;
  sgdSetVec3(down, -pt[0], -pt[1], -pt[2]);
  sgdNormalizeVec3(down);

  // We collapse all transforms we need to reach a particular leaf.
  // The leafs itself will be then transformed later.
  // So our cache is just flat.
  // For leafs which are moving (carriers surface, etc ...)
  // we will later store a speed in the GroundType class. We can then apply
  // some translations to that nodes according to the time which has passed
  // compared to that snapshot.
  sgdMat4 xform;
  sgdMakeIdentMat4( xform );


  // Walk the scene graph and extract solid ground triangles and carrier data.
  ssgBranch *terrain = globals->get_scenery()->get_scene_graph();
  cache_fill(terrain, xform, &acSphere, down, &wireSphere);

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

  if (cntr != old_cntr)
    globals->get_scenery()->set_center(old_cntr);

  return found_ground;
}

bool
FGGroundCache::is_valid(double *ref_time, double pt[3], double *rad)
{
  sgdCopyVec3(pt, reference_wgs84_point);
  *rad = reference_vehicle_radius;
  *ref_time = cache_ref_time;
  return found_ground;
}

double
FGGroundCache::get_cat(double t, const double dpt[3],
                       double end[2][3], double vel[2][3])
{
  // start with a distance of 1e10 meters...
  double dist = 1e10;

  // Time difference to the reference time.
  t -= cache_ref_time;

  size_t sz = catapults.size();
  for (size_t i = 0; i < sz; ++i) {
    sgdVec3 pivotoff, rvel[2];
    sgdLineSegment3 ls;
    sgdCopyVec3(ls.a, catapults[i].start);
    sgdCopyVec3(ls.b, catapults[i].end);

    sgdSubVec3(pivotoff, ls.a, catapults[i].rotation_pivot);
    sgdVectorProductVec3(rvel[0], catapults[i].rotation, pivotoff);
    sgdAddVec3(rvel[0], catapults[i].velocity);
    sgdSubVec3(pivotoff, ls.b, catapults[i].rotation_pivot);
    sgdVectorProductVec3(rvel[1], catapults[i].rotation, pivotoff);
    sgdAddVec3(rvel[1], catapults[i].velocity);

    sgdAddVec3(ls.a, cache_center);
    sgdAddVec3(ls.b, cache_center);

    sgdAddScaledVec3(ls.a, rvel[0], t);
    sgdAddScaledVec3(ls.b, rvel[1], t);
    
    double this_dist = sgdDistSquaredToLineSegmentVec3( ls, dpt );
    if (this_dist < dist) {
      SG_LOG(SG_FLIGHT,SG_INFO, "Found catapult "
             << this_dist << " meters away");
      dist = this_dist;
        
      // The carrier code takes care of that ordering.
      sgdCopyVec3( end[0], ls.a );
      sgdCopyVec3( end[1], ls.b );
      sgdCopyVec3( vel[0], rvel[0] );
      sgdCopyVec3( vel[1], rvel[1] );
    }
  }

  // At the end take the root, we only computed squared distances ...
  return sqrt(dist);
}

bool
FGGroundCache::get_agl(double t, const double dpt[3], double max_altoff,
                       double contact[3], double normal[3], double vel[3],
                       int *type, double *loadCapacity,
                       double *frictionFactor, double *agl)
{
  bool ret = false;

  *type = FGInterface::Unknown;
//   *agl = 0.0;
  *loadCapacity = DBL_MAX;
  *frictionFactor = 1.0;
  sgdSetVec3( vel, 0.0, 0.0, 0.0 );
  sgdSetVec3( contact, 0.0, 0.0, 0.0 );
  sgdSetVec3( normal, 0.0, 0.0, 0.0 );

  // Time difference to th reference time.
  t -= cache_ref_time;

  // The double valued point we start to search for intersection.
  sgdVec3 pt;
  sgdSubVec3( pt, dpt, cache_center );

  // The search direction
  sgdVec3 dir;
  sgdSetVec3( dir, -dpt[0], -dpt[1], -dpt[2] );
  sgdNormaliseVec3( dir );

  // Initialize to something sensible
  double current_radius = 0.0;

  size_t sz = triangles.size();
  for (size_t i = 0; i < sz; ++i) {
    Triangle triangle;
    velocityTransformTriangle(t, triangle, triangles[i]);
    if (!fgdIsectSphereInfLine(triangle.sphere, pt, dir))
      continue;

    // Check for intersection.
    sgdVec3 isecpoint;
    if ( sgdIsectInfLinePlane( isecpoint, pt, dir, triangle.plane ) &&
         sgdPointInTriangle( isecpoint, triangle.vertices ) ) {
      // Compute the vector from pt to the intersection point ...
      sgdVec3 off;
      sgdSubVec3(off, isecpoint, pt);
      // ... and check if it is too high or not
      if (-max_altoff < sgdScalarProductVec3( off, dir )) {
        // Transform to the wgs system
        sgdAddVec3( isecpoint, cache_center );
        // compute the radius, good enough approximation to take the geocentric radius
        SGDfloat radius = sgdLengthSquaredVec3(isecpoint);
        if (current_radius < radius) {
          current_radius = radius;
          ret = true;
          // Save the new potential intersection point.
          sgdCopyVec3( contact, isecpoint );
          // The first three values in the vector are the plane normal.
          sgdCopyVec3( normal, triangle.plane );
          // The velocity wrt earth.
          sgdVec3 pivotoff;
          sgdSubVec3(pivotoff, pt, triangle.rotation_pivot);
          sgdVectorProductVec3(vel, triangle.rotation, pivotoff);
          sgdAddVec3(vel, triangle.velocity);
          // Save the ground type.
          *type = triangle.type;
          // FIXME: figure out how to get that sign ...
//           *agl = sqrt(sqdist);
          *agl = sgdLengthVec3( dpt ) - sgdLengthVec3( contact );
//           *loadCapacity = DBL_MAX;
//           *frictionFactor = 1.0;
        }
      }
    }
  }

  if (ret)
    return true;

  // Whenever we did not have a ground triangle for the requested point,
  // take the ground level we found during the current cache build.
  // This is as good as what we had before for agl.
  double r = sgdLengthVec3( dpt );
  sgdCopyVec3( contact, dpt );
  sgdScaleVec3( contact, ground_radius/r );
  sgdCopyVec3( normal, dpt );
  sgdNormaliseVec3( normal );
  sgdSetVec3( vel, 0.0, 0.0, 0.0 );
  
  // The altitude is the distance of the requested point from the
  // contact point.
  *agl = sgdLengthVec3( dpt ) - sgdLengthVec3( contact );
  *type = FGInterface::Unknown;
  *loadCapacity = DBL_MAX;
  *frictionFactor = 1.0;

  return ret;
}

bool FGGroundCache::caught_wire(double t, const double pt[4][3])
{
  size_t sz = wires.size();
  if (sz == 0)
    return false;

  // Time difference to the reference time.
  t -= cache_ref_time;

  // Build the two triangles spanning the area where the hook has moved
  // during the past step.
  sgdVec4 plane[2];
  sgdVec3 tri[2][3];
  sgdMakePlane( plane[0], pt[0], pt[1], pt[2] );
  sgdCopyVec3( tri[0][0], pt[0] );
  sgdCopyVec3( tri[0][1], pt[1] );
  sgdCopyVec3( tri[0][2], pt[2] );
  sgdMakePlane( plane[1], pt[0], pt[2], pt[3] );
  sgdCopyVec3( tri[1][0], pt[0] );
  sgdCopyVec3( tri[1][1], pt[2] );
  sgdCopyVec3( tri[1][2], pt[3] );

  // Intersect the wire lines with each of these triangles.
  // You have caught a wire if they intersect.
  for (size_t i = 0; i < sz; ++i) {
    sgdVec3 le[2];
    for (int k = 0; k < 2; ++k) {
      sgdVec3 pivotoff, vel;
      sgdCopyVec3(le[k], wires[i].ends[k]);
      sgdSubVec3(pivotoff, le[k], wires[i].rotation_pivot);
      sgdVectorProductVec3(vel, wires[i].rotation, pivotoff);
      sgdAddVec3(vel, wires[i].velocity);
      sgdAddScaledVec3(le[k], vel, t);
      sgdAddVec3(le[k], cache_center);
    }
    
    for (int k=0; k<2; ++k) {
      sgdVec3 isecpoint;
      double isecval = sgdIsectLinesegPlane(isecpoint, le[0], le[1], plane[k]);
      if ( 0.0 <= isecval && isecval <= 1.0 &&
           sgdPointInTriangle( isecpoint, tri[k] ) ) {
        SG_LOG(SG_FLIGHT,SG_INFO, "Caught wire");
        // Store the wire id.
        wire_id = wires[i].wire_id;
        return true;
      }
    }
  }

  return false;
}

bool FGGroundCache::get_wire_ends(double t, double end[2][3], double vel[2][3])
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
        sgdVec3 pivotoff;
        sgdCopyVec3(end[k], wires[i].ends[k]);
        sgdSubVec3(pivotoff, end[k], wires[i].rotation_pivot);
        sgdVectorProductVec3(vel[k], wires[i].rotation, pivotoff);
        sgdAddVec3(vel[k], wires[i].velocity);
        sgdAddScaledVec3(end[k], vel[k], t);
        sgdAddVec3(end[k], cache_center);
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
