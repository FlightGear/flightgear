// scenery.cxx -- data structures and routines for managing scenery.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/model/placementtrans.hxx>

#include <Main/fg_props.hxx>

#include "hitlist.hxx"
#include "scenery.hxx"


// Scenery Management system
FGScenery::FGScenery() {
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing scenery subsystem" );

    center = Point3D(0.0);
}


// Initialize the Scenery Management system
FGScenery::~FGScenery() {
}


void FGScenery::init() {
    // Scene graph root
    scene_graph = new ssgRoot;
    scene_graph->setName( "Scene" );

    // Terrain branch
    terrain_branch = new ssgBranch;
    terrain_branch->setName( "Terrain" );
    scene_graph->addKid( terrain_branch );

    models_branch = new ssgBranch;
    models_branch->setName( "Models" );
    scene_graph->addKid( models_branch );

    aircraft_branch = new ssgBranch;
    aircraft_branch->setName( "Aircraft" );
    scene_graph->addKid( aircraft_branch );

    // Lighting
    gnd_lights_root = new ssgRoot;
    gnd_lights_root->setName( "Ground Lighting Root" );

    vasi_lights_root = new ssgRoot;
    vasi_lights_root->setName( "VASI/PAPI Lighting Root" );

    rwy_lights_root = new ssgRoot;
    rwy_lights_root->setName( "Runway Lighting Root" );

    taxi_lights_root = new ssgRoot;
    taxi_lights_root->setName( "Taxi Lighting Root" );

    // Initials values needed by the draw-time object loader
    sgUserDataInit( globals->get_model_lib(), globals->get_fg_root(),
                    globals->get_props(), globals->get_sim_time_sec() );
}


void FGScenery::update(double dt) {
}


void FGScenery::bind() {
}


void FGScenery::unbind() {
}

void FGScenery::set_center( const Point3D& p ) {
    center = p;
    sgdVec3 c;
    sgdSetVec3(c, p.x(), p.y(), p.z());
    placement_list_type::iterator it = _placement_list.begin();
    while (it != _placement_list.end()) {
        (*it)->setSceneryCenter(c);
        ++it;
    }
}

void FGScenery::register_placement_transform(ssgPlacementTransform *trans) {
    _placement_list.push_back(trans);        
    sgdVec3 c;
    sgdSetVec3(c, center.x(), center.y(), center.z());
    trans->setSceneryCenter(c);
}

void FGScenery::unregister_placement_transform(ssgPlacementTransform *trans) {
    placement_list_type::iterator it = _placement_list.begin();
    while (it != _placement_list.end()) {
        if ((*it) == trans) {
            it = _placement_list.erase(it);        
        } else
            ++it;
    }
}

bool
FGScenery::get_elevation_m(double lat, double lon, double max_alt,
                           double& alt, bool exact)
{
//   std::cout << __PRETTY_FUNCTION__ << " "
//             << lat << " "
//             << lon << " "
//             << max_alt
//             << std::endl;
  sgdVec3 pos;
  sgGeodToCart(lat*SG_DEGREES_TO_RADIANS, lon*SG_DEGREES_TO_RADIANS,
               max_alt, pos);
  return get_cart_elevation_m(pos, 0, alt, exact);
}

bool
FGScenery::get_cart_elevation_m(const sgdVec3& pos, double max_altoff,
                                double& alt, bool exact)
{
  Point3D saved_center = center;
  bool replaced_center = false;
  if (exact) {
    Point3D ppos(pos[0], pos[1], pos[2]);
    if (30.0*30.0 < ppos.distance3Dsquared(center)) {
      set_center( ppos );
      replaced_center = true;
    }
  }

  // overridden with actual values if a terrain intersection is
  // found
  double hit_radius = 0.0;
  sgdVec3 hit_normal = { 0.0, 0.0, 0.0 };
  
  bool hit = false;
  if ( fabs(pos[0]) > 1.0 || fabs(pos[1]) > 1.0 || fabs(pos[2]) > 1.0 ) {
    sgdVec3 sc;
    sgdSetVec3(sc, center[0], center[1], center[2]);
    
    sgdVec3 ncpos;
    sgdCopyVec3(ncpos, pos);
    
    FGHitList hit_list;
    
    // scenery center has been properly defined so any hit should
    // be valid (and not just luck)
    hit = fgCurrentElev(ncpos, max_altoff+sgdLengthVec3(pos),
                        sc, (ssgTransform*)get_scene_graph(),
                        &hit_list, &alt, &hit_radius, hit_normal);
  }

  if (replaced_center)
    set_center( saved_center );
  
  return hit;
}


bool
FGScenery::get_cart_ground_intersection(const sgdVec3& pos,
                                        const sgdVec3& dir,
                                        sgdVec3& nearestHit, bool exact)
{
  // We assume that starting positions in the center of the earth are invalid
  if ( fabs(pos[0]) < 1.0 && fabs(pos[1]) < 1.0 && fabs(pos[2]) < 1.0 )
    return false;

  // Well that 'exactness' is somehow problematic, but makes at least sure
  // that we don't compute that with a cenery center at the other side of
  // the world ...
  Point3D saved_center = center;
  bool replaced_center = false;
  if (exact) {
    Point3D ppos(pos[0], pos[1], pos[2]);
    if (30.0*30.0 < ppos.distance3Dsquared(center)) {
      set_center( ppos );
      replaced_center = true;
    }
  }

  // Not yet found any hit ...
  bool result = false;

  // Make really sure the direction is normalized, is really cheap compared to
  // computation of ground intersection.
  sgdVec3 normalizedDir;
  sgdCopyVec3(normalizedDir, dir);
  sgdNormaliseVec3(normalizedDir);

  sgdVec3 sceneryCenter;
  sgdSetVec3(sceneryCenter, center[0], center[1], center[2]);
  sgdVec3 relativePos;
  sgdSubVec3(relativePos, pos, sceneryCenter);

  // At the moment only intersection with the terrain?
  FGHitList hit_list;
  hit_list.Intersect(globals->get_scenery()->get_terrain_branch(),
                     relativePos, normalizedDir);

  double dist = DBL_MAX;
  int hitcount = hit_list.num_hits();
  for (int i = 0; i < hitcount; ++i) {
    // Check for the nearest hit
    sgdVec3 diff;
    sgdSubVec3(diff, hit_list.get_point(i), relativePos);
    
    // We only want hits in front of us ...
    if (sgdScalarProductVec3(normalizedDir, diff) < 0)
      continue;

    // find the nearest hit
    double nDist = sgdScalarProductVec3(diff, diff);
    if (dist < nDist)
      continue;

    // Store the hit point
    dist = nDist;
    sgdAddVec3(nearestHit, hit_list.get_point(i), sceneryCenter);
    result = true;
  }

  if (replaced_center)
    set_center( saved_center );

  return result;
}

