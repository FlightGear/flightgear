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


#include <float.h>

#include <plib/sg.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <AIModel/AICarrier.hxx>

#include "flight.hxx"
#include "groundcache.hxx"

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
  cache_root.removeAllKids();
}

FGGroundCache::GroundProperty*
FGGroundCache::extractGroundProperty( ssgLeaf* l )
{
  // FIXME: Do more ...
  // Idea: have a get_globals() function which knows about that stuff.
  // Or most propably read that from a configuration file,
  // from property tree or whatever ...
  
  // Get ground dependent data.
  GroundProperty *gp = new GroundProperty;
  gp->wire_id = -1;
  
  FGAICarrierHardware *ud =
    dynamic_cast<FGAICarrierHardware*>(l->getUserData());
  if (ud) {
    switch (ud->type) {
    case FGAICarrierHardware::Wire:
      gp->type = FGInterface::Wire;
      gp->wire_id = ud->id;
      break;
    case FGAICarrierHardware::Catapult:
      gp->type = FGInterface::Catapult;
      break;
    default:
      gp->type = FGInterface::Solid;
      break;
    }

    // Copy the velocity from the carrier class.
    ud->carrier->getVelocityWrtEarth( gp->vel );
  }

  else {

    // Initialize velocity field.
    sgSetVec3( gp->vel, 0.0, 0.0, 0.0 );
  }
  
  // Get the texture name and decide what ground type we have.
  ssgState *st = l->getState();
  if (st != NULL && st->isAKindOf(ssgTypeSimpleState())) {
    ssgSimpleState *ss = (ssgSimpleState*)st;
    SGPath fullPath( ss->getTextureFilename() ? ss->getTextureFilename(): "" );
    string file = fullPath.file();
    SGPath dirPath(fullPath.dir());
    string category = dirPath.file();
    
    SG_LOG(SG_FLIGHT,SG_INFO,
           "New triangle in cache: " << category << " " << file );
    
    if (category == "Runway")
      gp->type = FGInterface::Solid;
    else {
      if (file == "asphault.rgb" || file == "airport.rgb")
        gp->type = FGInterface::Solid;
      else if (file == "water.rgb" || file == "water-lake.rgb")
        gp->type = FGInterface::Water;
      else if (file == "forest.rgb" || file == "cropwood.rgb")
        gp->type = FGInterface::Forest;
    }
  }
  
  return gp;
}

// Test if the line given by the point on the line pt_on_line and the
// line direction dir intersects the sphere sp.
// Adapted from plib.
static bool
sgIsectSphereInfLine(const sgSphere *sp,
                     const sgVec3 pt_on_line, const sgVec3 dir)
{
  sgVec3 r ;
  sgSubVec3 ( r, sp->getCenter(), pt_on_line ) ;

  SGfloat projectedDistance = sgScalarProductVec3(r, dir);
 
  SGfloat dist = sgScalarProductVec3 ( r, r ) -
    projectedDistance * projectedDistance; 

  SGfloat radius = sp->getRadius();
  return dist < radius*radius;
}

void
FGGroundCache::addAndFlattenLeaf(GLenum ty, ssgLeaf *l, ssgIndexArray *ia,
                                 const sgMat4 xform)
{
  // Extract data from the leaf which is just copied.
  ssgVertexArray *va = ((ssgVtxTable *)l)->getVertices();
  // Create a new leaf.
  ssgVtxArray *vtxa = new ssgVtxArray( ty, va, 0, 0, 0, ia );
  // Clones data ...
  vtxa->removeUnusedVertices();
  // Apply transform. We won't store transforms in our cache.
  vtxa->transform( xform );
  // Check for magic texture names object names and such ...
  vtxa->setUserData( extractGroundProperty( l ) );
  // Finally append to cache.
  cache_root.addKid((ssgEntity*)vtxa);
}

void
FGGroundCache::putLineLeafIntoCache(const sgSphere *wsp, const sgMat4 xform,
                                    ssgLeaf *l)
{
  ssgIndexArray *ia = 0;
  
  // Lines must have special meanings.
  // Wires and catapults are done with lines.
  int nl = l->getNumLines();
  for (int i = 0; i < nl; ++i) {
    sgSphere tmp;
    short v[2];
    l->getLine(i, v, v+1 );
    for (int k=0; k<2; ++k)
      tmp.extend( l->getVertex( v[k] ) );
    tmp.orthoXform(xform);
    
    if (wsp->intersects( &tmp )) {
      if (ia == 0)
        ia = new ssgIndexArray();
        
      ia->add( v[0] );
      ia->add( v[1] );
    }
  }
  if (!ia)
    return;
  
  addAndFlattenLeaf(GL_LINES, l, ia, xform);
}

void
FGGroundCache::putSurfaceLeafIntoCache(const sgSphere *sp, const sgMat4 xform,
                                       bool sphIsec, sgVec3 down, ssgLeaf *l)
{
  ssgIndexArray *ia = 0;
  
  int nt = l->getNumTriangles();
  for (int i = 0; i < nt; ++i) {
    // Build up a sphere around that particular triangle-
    sgSphere tmp;
    short v[3];
    l->getTriangle(i, v, v+1, v+2 );
    for (int k=0; k<3; ++k)
      tmp.extend( l->getVertex( v[k] ) );
    tmp.orthoXform(xform);

    // Check if the sphere around the vehicle intersects the sphere
    // around that triangle. If so, put that triangle into the cache.
    if (sphIsec && sp->intersects( &tmp )) {
      if (ia == 0)
        ia = new ssgIndexArray();
      
      ia->add( v[0] );
      ia->add( v[1] );
      ia->add( v[2] );
    }
    
    // In case the cache is empty, we still provide agl computations.
    // But then we use the old way of having a fixed elevation value for
    // the whole lifetime of this cache.
    if ( sgIsectSphereInfLine(&tmp, sp->getCenter(), down) ) {
      sgVec3 tri[3];
      for (int k=0; k<3; ++k) {
        sgCopyVec3( tri[k], l->getVertex( v[k] ) );
        sgXformPnt3( tri[k], xform );
      }
      
      sgVec4 plane;
      sgMakePlane( plane, tri[0], tri[1], tri[2]  );
      sgVec3 ac_cent;
      sgCopyVec3(ac_cent, sp->getCenter());
      sgVec3 dst;
      sgIsectInfLinePlane( dst, ac_cent, down, plane );
      if ( sgPointInTriangle ( dst, tri ) ) {
        found_ground = true;
        sgdVec3 ddst;
        sgdSetVec3(ddst, dst);
        sgdAddVec3(ddst, cache_center);
        double this_radius = sgdLengthVec3(ddst);
        if (ground_radius < this_radius)
          ground_radius = this_radius;
      }
    }
  }
  if (!ia)
    return;
  
  addAndFlattenLeaf(GL_TRIANGLES, l, ia, xform);
}

// Here is the point where rotation should be handled
void
FGGroundCache::extractCacheRelativeVertex(double t, ssgVtxArray *va,
                                          GroundProperty *gp,
                                          short i, sgVec3 rel_pos,
                                          sgdVec3 wgs84_vel)
{
  sgCopyVec3( rel_pos, va->getVertex( i ) );
  sgAddScaledVec3( rel_pos, gp->vel, t );

  // Set velocity.
  sgdSetVec3( wgs84_vel, gp->vel );
}

void
FGGroundCache::extractWgs84Vertex(double t, ssgVtxArray *va,
                                  GroundProperty *gp, short i,
                                  sgdVec3 wgs84_pos, sgdVec3 wgs84_vel)
{
  sgVec3 rel_pos;
  extractCacheRelativeVertex(t, va, gp, i, rel_pos, wgs84_vel);
  sgdSetVec3( wgs84_pos, rel_pos );
  sgdAddVec3( wgs84_pos, cache_center );
}


void
FGGroundCache::cache_fill(ssgBranch *branch, sgMat4 xform,
                          sgSphere* sp, sgVec3 down, sgSphere* wsp)
{
  // Travel through all kids.
  ssgEntity *e;
  for ( e = branch->getKid(0); e != NULL ; e = branch->getNextKid() ) {
    if ( !( e->getTraversalMask() & SSGTRAV_HOT) )
      continue;
    if ( e->getBSphere()->isEmpty() )
      continue;
    
    // Wee need to check further if either the sphere around the branch
    // intersects the sphere around the aircraft or the line downwards from
    // the aircraft intersects the branchs sphere.
    sgSphere esphere = *(e->getBSphere());
    esphere.orthoXform(xform);
    bool wspIsec = wsp->intersects(&esphere);
    bool downIsec = sgIsectSphereInfLine(&esphere, sp->getCenter(), down);
    if (!wspIsec && !downIsec)
      continue;
      
    // For branches collect up the transforms to reach that branch and
    // call cache_fill recursively.
    if ( e->isAKindOf( ssgTypeBranch() ) ) {
      ssgBranch *b = (ssgBranch *)e;
      if ( b->isAKindOf( ssgTypeTransform() ) ) {
        // Collect up the transfors required to reach that part of
        // the branch.
        sgMat4 xform2;
        sgMakeIdentMat4( xform2 );
        ssgTransform *t = (ssgTransform*)b;
        t->getTransform( xform2 );
        sgPostMultMat4( xform2, xform );
        cache_fill( b, xform2, sp, down, wsp );
      } else
        cache_fill( b, xform, sp, down, wsp );
    }
   
    // For leafs, check each triangle for intersection.
    // This will minimize the number of vertices/triangles in the cache.
    else if (e->isAKindOf(ssgTypeLeaf())) {
      // Since we reach that leaf if we have an intersection with the
      // most propably bigger wire/catapult cache sphere, we need to check
      // that here, if the smaller cache for the surface has a chance for hits.
      // Also, if the spheres do not intersect compute a croase agl value
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
  cache_root.removeAllKids();
  ground_radius = 0.0;
  found_ground = false;

  // Store the parameters we used to build up that cache.
  sgdCopyVec3(reference_wgs84_point, pt);
  reference_vehicle_radius = rad;
  // Store the time reference used to compute movements of moving triangles.
  cache_ref_time = ref_time;

  // The center of the cache.
  Point3D psc = globals->get_tile_mgr()->get_current_center();
  sgdSetVec3(cache_center, psc[0], psc[1], psc[2]);
  
  // Prepare sphere around the aircraft.
  sgSphere acSphere;
  acSphere.setRadius(rad);

  // Compute the postion of the aircraft relative to the scenery center.
  sgdVec3 doffset;
  sgdSubVec3( doffset, pt, cache_center );
  sgVec3 offset;
  sgSetVec3( offset, doffset[0], doffset[1], doffset[2] );
  acSphere.setCenter( offset );

  // Prepare bigger sphere around the aircraft.
  // This one is required for reliably finding wires we have caught but
  // have already left the hopefully smaller sphere for the ground reactions.
  const double max_wire_dist = 300.0;
  sgSphere wireSphere;
  wireSphere.setRadius( max_wire_dist < rad ? rad : max_wire_dist );
  wireSphere.setCenter( offset );

  // Down vector. Is used for croase agl computations when we are far enough
  // from ground that we have an empty cache.
  sgVec3 down;
  sgSetVec3(down, -pt[0], -pt[1], -pt[2]);
  sgNormalizeVec3(down);

  // We collaps all transforms we need to reach a particular leaf.
  // The leafs itself will be then transformed later.
  // So our cache is just flat.
  // For leafs which are moving (carriers surface, etc ...)
  // we will later store a speed in the GroundType class. We can then apply
  // some translations to that nodes according to the time which has passed
  // compared to that snapshot.
  sgMat4 xform;
  sgMakeIdentMat4(xform);

  // Walk the terrain branch for now.
  ssgBranch *terrain = globals->get_scenery()->get_scene_graph();
  cache_fill(terrain, xform, &acSphere, down, &wireSphere);

  // some stats
  SG_LOG(SG_FLIGHT,SG_INFO, "prepare_ground_cache(): ac radius = " << rad
         << ", # leafs = " << cache_root.getNumKids()
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

  // We know that we have a flat cache ...
  ssgEntity *e;
  for ( e = cache_root.getKid(0); e != NULL ; e = cache_root.getNextKid() ) {
    // We just know that, because we build that ourselfs ...
    ssgVtxArray *va = (ssgVtxArray *)e;
    // Only lines are interresting ...
    if (va->getPrimitiveType() != GL_LINES)
      continue;
    GroundProperty *gp = dynamic_cast<GroundProperty*>(va->getUserData());
    // Assertation???
    if ( !gp )
      continue;
    // Check if we have a catapult ...
    if ( gp->type != FGInterface::Catapult )
      continue;

    int nl = va->getNumLines();
    for (int i=0; i < nl; ++i) {
      sgdLineSegment3 ls;
      sgdVec3 lsVel[2];
      short vi[2];
      va->getLine(i, vi, vi+1 );
      extractWgs84Vertex(t, va, gp, vi[0], ls.a, lsVel[0]);
      extractWgs84Vertex(t, va, gp, vi[1], ls.b, lsVel[1]);
      
      double this_dist = sgdDistSquaredToLineSegmentVec3( ls, dpt );
      if (this_dist < dist) {
        dist = this_dist;
        
        // end[0] is the end where the cat starts.
        // end[1] is the end where the cat ends.
        // The carrier code takes care of that ordering.
        sgdCopyVec3( end[0], ls.a );
        sgdCopyVec3( end[1], ls.b );
        sgdCopyVec3( vel[0], lsVel[0] );
        sgdCopyVec3( vel[1], lsVel[1] );
      }
    }
  }

  // At the end take the root, we only computed squared distances ...
  return sqrt(dist);
}

bool
FGGroundCache::get_agl(double t, const double dpt[3],
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
  sgdVec3 tmp;
  sgdSubVec3( tmp, dpt, cache_center );
  sgVec3 pt;
  sgSetVec3( pt, tmp );

  // The search direction
  sgVec3 dir;
  sgSetVec3( dir, -dpt[0], -dpt[1], -dpt[2] );

  // Initialize to something sensible
  double sqdist = DBL_MAX;

  // We know that we have a flat cache ...
  // We just know that, because we build that ourselfs ...
  ssgEntity *e;
  for ( e = cache_root.getKid(0) ; e != NULL ; e = cache_root.getNextKid() ) {
    // We just know that, because we build that ourselfs ...
    ssgVtxArray *va = (ssgVtxArray *)e;
    // AGL computations are done with triangle/surface leafs.
    if (va->getPrimitiveType() != GL_TRIANGLES)
      continue;
    GroundProperty *gp = dynamic_cast<GroundProperty*>(va->getUserData());
    // Assertation???
    if ( !gp )
      continue;

    int nt = va->getNumTriangles();
    for (int i=0; i < nt; ++i) {
      short vi[3];
      va->getTriangle( i, vi, vi+1, vi+2 );
      
      sgVec3 tri[3];
      sgdVec3 dvel[3];
      for (int k=0; k<3; ++k)
        extractCacheRelativeVertex(t, va, gp, vi[k], tri[k], dvel[k]);
      sgVec4 plane;
      sgMakePlane( plane, tri[0], tri[1], tri[2] );
      
      // Check for intersection.
      sgVec3 isecpoint;
      if ( sgIsectInfLinePlane( isecpoint, pt, dir, plane ) &&
           sgPointInTriangle3( isecpoint, tri ) ) {
        // Check for the closest intersection point.
        // FIXME: is this the right one?
        double newSqdist = sgDistanceSquaredVec3( isecpoint, pt );
        if ( newSqdist < sqdist ) {
          sqdist = newSqdist;
          ret = true;
          // Save the new potential intersection point.
          sgdSetVec3( contact, isecpoint );
          sgdAddVec3( contact, cache_center );
          // The first three values in the vector are the plane normal.
          sgdSetVec3( normal, plane );
          // The velocity wrt earth.
          /// FIXME: only true for non rotating objects!!!!
          sgdCopyVec3( vel, dvel[0] );
          // Save the ground type.
          *type = gp->type;
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

bool FGGroundCache::caught_wire(double t, const double cpt[4][3])
{
  bool ret = false;

  // Time difference to the reference time.
  t -= cache_ref_time;

  bool firsttime = true;
  sgVec4 plane[2];
  sgVec3 tri[2][3];

  // We know that we have a flat cache ...
  ssgEntity *e;
  for ( e = cache_root.getKid(0); e != NULL ; e = cache_root.getNextKid() ) {
    // We just know that, because we build that ourselfs ...
    ssgVtxArray *va = (ssgVtxArray *)e;
    // Only lines are interresting ...
    if (va->getPrimitiveType() != GL_LINES)
      continue;
    GroundProperty *gp = dynamic_cast<GroundProperty*>(va->getUserData());
    // Assertation???
    if ( !gp )
      continue;
    // Check if we have a catapult ...
    if ( gp->type != FGInterface::Wire )
      continue;

    // Lazy compute the values required for intersectiion tests.
    // Since we normally do not have wires in the cache this is a
    // huge benefit.
    if (firsttime) {
      firsttime = false;
      sgVec3 pt[4];
      for (int k=0; k<4; ++k) {
        sgdVec3 tmp;
        sgdSubVec3( tmp, cpt[k], cache_center );
        sgSetVec3( pt[k], tmp );
      }
      sgMakePlane( plane[0], pt[0], pt[1], pt[2] );
      sgCopyVec3( tri[0][0], pt[0] );
      sgCopyVec3( tri[0][1], pt[1] );
      sgCopyVec3( tri[0][2], pt[2] );
      sgMakePlane( plane[1], pt[0], pt[2], pt[3] );
      sgCopyVec3( tri[1][0], pt[0] );
      sgCopyVec3( tri[1][1], pt[2] );
      sgCopyVec3( tri[1][2], pt[3] );
    }
    
    int nl = va->getNumLines();
    for (int i=0; i < nl; ++i) {
      short vi[2];
      va->getLine(i, vi, vi+1 );
      sgVec3 le[2];
      sgdVec3 dummy;
      extractCacheRelativeVertex(t, va, gp, vi[0], le[0], dummy);
      extractCacheRelativeVertex(t, va, gp, vi[1], le[1], dummy);
      
      for (int k=0; k<2; ++k) {
        sgVec3 isecpoint;
        float isecval = sgIsectLinesegPlane( isecpoint, le[0], le[1], plane[k] );
        
        if ( 0.0 <= isecval && isecval <= 1.0 &&
             sgPointInTriangle( isecpoint, tri[k] ) ) {
          // Store the wire id.
          wire_id = gp->wire_id;
          ret = true;
        }
      }
    }
  }

  return ret;
}

bool FGGroundCache::get_wire_ends(double t, double end[2][3], double vel[2][3])
{
  // Fast return if we do not have an active wire.
  if (wire_id < 0)
    return false;

  bool ret = false;

  // Time difference to th reference time.
  t -= cache_ref_time;

  // We know that we have a flat cache ...
  ssgEntity *e;
  for ( e = cache_root.getKid(0); e != NULL ; e = cache_root.getNextKid() ) {
    // We just know that, because we build that ourselfs ...
    ssgVtxArray *va = (ssgVtxArray *)e;
    // Only lines are interresting ...
    if (va->getPrimitiveType() != GL_LINES)
      continue;
    GroundProperty *gp = dynamic_cast<GroundProperty*>(va->getUserData());
    // Assertation???
    if ( !gp )
      continue;
    // Check if we have a catapult ...
    if ( gp->type != FGInterface::Wire )
      continue;
    if ( gp->wire_id != wire_id )
      continue;

    // Get the line ends, that are the wire endpoints.
    short vi[2];
    va->getLine(0, vi, vi+1 );
    extractWgs84Vertex(t, va, gp, vi[0], end[0], vel[0]);
    extractWgs84Vertex(t, va, gp, vi[1], end[1], vel[1]);

    ret = true;
  }

  return ret;
}

void FGGroundCache::release_wire(void)
{
  wire_id = -1;
}
