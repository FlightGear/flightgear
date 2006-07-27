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
#include <simgear/scene/material/matlib.hxx>

#include <Main/fg_props.hxx>

#include "hitlist.hxx"
#include "scenery.hxx"


// Scenery Management system
FGScenery::FGScenery() :
  center(0, 0, 0)
{
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing scenery subsystem" );
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

void FGScenery::set_center( const SGVec3d& p ) {
    if (center == p)
      return;
    center = p;
    placement_list_type::iterator it = _placement_list.begin();
    while (it != _placement_list.end()) {
        (*it)->setSceneryCenter(center.sg());
        ++it;
    }
}

void FGScenery::register_placement_transform(ssgPlacementTransform *trans) {
    _placement_list.push_back(trans);        
    trans->setSceneryCenter(center.sg());
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
                           double& alt, const SGMaterial** material,
                           bool exact)
{
  SGGeod geod = SGGeod::fromDegM(lon, lat, max_alt);
  SGVec3d pos = SGVec3d::fromGeod(geod);
  return get_cart_elevation_m(pos, 0, alt, material, exact);
}

bool
FGScenery::get_cart_elevation_m(const SGVec3d& pos, double max_altoff,
                                double& alt, const SGMaterial** material,
                                bool exact)
{
  if ( norm1(pos) < 1 )
    return false;

  SGVec3d saved_center = center;
  bool replaced_center = false;
  if (exact) {
    if (30*30 < distSqr(pos, center)) {
      set_center( pos );
      replaced_center = true;
    }
  }

  // overridden with actual values if a terrain intersection is
  // found
  int this_hit;
  double hit_radius = 0.0;
  SGVec3d hit_normal(0, 0, 0);
  
  SGVec3d sc = center;
  SGVec3d ncpos = pos;

  FGHitList hit_list;
  // scenery center has been properly defined so any hit should
  // be valid (and not just luck)
  bool hit = fgCurrentElev(ncpos.sg(), max_altoff+length(pos), sc.sg(),
                           get_scene_graph(), &hit_list, &alt,
                           &hit_radius, hit_normal.sg(), this_hit);

  if (material) {
    *material = 0;
    if (hit) {
      ssgEntity *entity = hit_list.get_entity( this_hit );
      if (entity && entity->isAKindOf(ssgTypeLeaf())) {
        ssgLeaf* leaf = static_cast<ssgLeaf*>(entity);
        *material = globals->get_matlib()->findMaterial(leaf);
      }
    }
  }

  if (replaced_center)
    set_center( saved_center );
  
  return hit;
}

bool
FGScenery::get_cart_ground_intersection(const SGVec3d& pos, const SGVec3d& dir,
                                        SGVec3d& nearestHit, bool exact)
{
  // We assume that starting positions in the center of the earth are invalid
  if ( norm1(pos) < 1 )
    return false;

  // Well that 'exactness' is somehow problematic, but makes at least sure
  // that we don't compute that with a cenery center at the other side of
  // the world ...
  SGVec3d saved_center = center;
  bool replaced_center = false;
  if (exact) {
    if (30*30 < distSqr(pos, center)) {
      set_center( pos );
      replaced_center = true;
    }
  }

  // Not yet found any hit ...
  bool result = false;

  // Make really sure the direction is normalized, is really cheap compared to
  // computation of ground intersection.
  SGVec3d normalizedDir = normalize(dir);
  SGVec3d relativePos = pos - center;

  // At the moment only intersection with the terrain?
  FGHitList hit_list;
  hit_list.Intersect(globals->get_scenery()->get_terrain_branch(),
                     relativePos.sg(), normalizedDir.sg());

  double dist = DBL_MAX;
  int hitcount = hit_list.num_hits();
  for (int i = 0; i < hitcount; ++i) {
    // Check for the nearest hit
    SGVec3d diff = SGVec3d(hit_list.get_point(i)) - relativePos;
    
    // We only want hits in front of us ...
    if (dot(normalizedDir, diff) < 0)
      continue;

    // find the nearest hit
    double nDist = dot(diff, diff);
    if (dist < nDist)
      continue;

    // Store the hit point
    dist = nDist;
    nearestHit = SGVec3d(hit_list.get_point(i)) + center;
    result = true;
  }

  if (replaced_center)
    set_center( saved_center );

  return result;
}

