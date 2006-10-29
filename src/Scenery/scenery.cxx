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

#include <osgUtil/IntersectVisitor>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/model/placementtrans.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include <Main/fg_props.hxx>

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
    scene_graph = new osg::Group;
    scene_graph->setName( "Scene" );

    // Terrain branch
    terrain_branch = new osg::Group;
    terrain_branch->setName( "Terrain" );
    scene_graph->addChild( terrain_branch.get() );

    models_branch = new osg::Group;
    models_branch->setName( "Models" );
    scene_graph->addChild( models_branch.get() );

    aircraft_branch = new osg::Group;
    aircraft_branch->setName( "Aircraft" );
    scene_graph->addChild( aircraft_branch.get() );

    // Lighting
    gnd_lights_root = new osg::Group;
    gnd_lights_root->setName( "Ground Lighting Root" );

    vasi_lights_root = new osg::Group;
    vasi_lights_root->setName( "VASI/PAPI Lighting Root" );

    rwy_lights_root = new osg::Group;
    rwy_lights_root->setName( "Runway Lighting Root" );

    taxi_lights_root = new osg::Group;
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
        (*it)->setSceneryCenter(center);
        ++it;
    }
}

void FGScenery::register_placement_transform(SGPlacementTransform *trans) {
    _placement_list.push_back(trans);        
    trans->setSceneryCenter(center);
}

void FGScenery::unregister_placement_transform(SGPlacementTransform *trans) {
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


  SGVec3d start = pos + max_altoff*normalize(pos) - center;
  SGVec3d end = - center;
  
  osgUtil::IntersectVisitor intersectVisitor;
  intersectVisitor.setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
  osg::ref_ptr<osg::LineSegment> lineSegment;
  lineSegment = new osg::LineSegment(start.osg(), end.osg());
  intersectVisitor.addLineSegment(lineSegment.get());
  get_scene_graph()->accept(intersectVisitor);
  bool hits = intersectVisitor.hits();
  if (hits) {
    int nHits = intersectVisitor.getNumHits(lineSegment.get());
    alt = -SGLimitsd::max();
    for (int i = 0; i < nHits; ++i) {
      const osgUtil::Hit& hit
        = intersectVisitor.getHitList(lineSegment.get())[i];
      SGVec3d point;
      point.osg() = hit.getWorldIntersectPoint();
      point += center;
      SGGeod geod = SGGeod::fromCart(point);
      double elevation = geod.getElevationM();
      if (alt < elevation) {
        alt = elevation;
        if (material)
          *material = globals->get_matlib()->findMaterial(hit.getGeode());
      }
    }
  }

  if (replaced_center)
    set_center( saved_center );
  
  return hits;
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

  // Make really sure the direction is normalized, is really cheap compared to
  // computation of ground intersection.
  SGVec3d start = pos - center;
  SGVec3d end = start + 1e5*dir;
  
  osgUtil::IntersectVisitor intersectVisitor;
  intersectVisitor.setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
  osg::ref_ptr<osg::LineSegment> lineSegment;
  lineSegment = new osg::LineSegment(start.osg(), end.osg());
  intersectVisitor.addLineSegment(lineSegment.get());
  get_scene_graph()->accept(intersectVisitor);
  bool hits = intersectVisitor.hits();
  if (hits) {
    int nHits = intersectVisitor.getNumHits(lineSegment.get());
    double dist = SGLimitsd::max();
    for (int i = 0; i < nHits; ++i) {
      const osgUtil::Hit& hit
        = intersectVisitor.getHitList(lineSegment.get())[i];
      SGVec3d point;
      point.osg() = hit.getWorldIntersectPoint();
      double newdist = length(start - point);
      if (newdist < dist) {
        dist = newdist;
        nearestHit = point + center;
      }
    }
  }

  if (replaced_center)
    set_center( saved_center );

  return hits;
}

