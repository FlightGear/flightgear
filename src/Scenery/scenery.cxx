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

#include <osgViewer/Viewer>
#include <osgUtil/IntersectVisitor>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/model/CheckSceneryVisitor.hxx>

#include <Main/renderer.hxx>
#include <Main/fg_props.hxx>

#include "tilemgr.hxx"
#include "scenery.hxx"

using namespace flightgear;
using namespace simgear;

class FGGroundPickCallback : public SGPickCallback {
public:
  virtual bool buttonPressed(int button, const Info& info)
  {
    // only on left mouse button
    if (button != 0)
      return false;

    SGGeod geod = SGGeod::fromCart(info.wgs84);
    SG_LOG( SG_TERRAIN, SG_INFO, "Got ground pick at " << geod );

    SGPropertyNode *c = fgGetNode("/sim/input/click", true);
    c->setDoubleValue("longitude-deg", geod.getLongitudeDeg());
    c->setDoubleValue("latitude-deg", geod.getLatitudeDeg());
    c->setDoubleValue("elevation-m", geod.getElevationM());
    c->setDoubleValue("elevation-ft", geod.getElevationFt());
    fgSetBool("/sim/signals/click", 1);

    return true;
  }
};

// Scenery Management system
FGScenery::FGScenery()
{
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing scenery subsystem" );
}

FGScenery::~FGScenery() {
}


// Initialize the Scenery Management system
void FGScenery::init() {
    // Scene graph root
    scene_graph = new osg::Group;
    scene_graph->setName( "Scene" );

    // Terrain branch
    terrain_branch = new osg::Group;
    terrain_branch->setName( "Terrain" );
    scene_graph->addChild( terrain_branch.get() );
    SGSceneUserData* userData;
    userData = SGSceneUserData::getOrCreateSceneUserData(terrain_branch.get());
    userData->setPickCallback(new FGGroundPickCallback);

    models_branch = new osg::Group;
    models_branch->setName( "Models" );
    scene_graph->addChild( models_branch.get() );

    aircraft_branch = new osg::Group;
    aircraft_branch->setName( "Aircraft" );
    scene_graph->addChild( aircraft_branch.get() );

    // Initials values needed by the draw-time object loader
    sgUserDataInit( globals->get_props() );
}


void FGScenery::update(double dt) {
}


void FGScenery::bind() {
}


void FGScenery::unbind() {
}

bool
FGScenery::get_cart_elevation_m(const SGVec3d& pos, double max_altoff,
                                double& alt, const SGMaterial** material,
                                const osg::Node* butNotFrom)
{
  SGGeod geod = SGGeod::fromCart(pos);
  geod.setElevationM(geod.getElevationM() + max_altoff);
  return get_elevation_m(geod, alt, material, butNotFrom);
}

bool
FGScenery::get_elevation_m(const SGGeod& geod, double& alt,
                           const SGMaterial** material,
                           const osg::Node* butNotFrom)
{
  SGVec3d start = SGVec3d::fromGeod(geod);

  SGGeod geodEnd = geod;
  geodEnd.setElevationM(SGMiscd::min(geod.getElevationM() - 10, -10000));
  SGVec3d end = SGVec3d::fromGeod(geodEnd);
  
  osgUtil::IntersectVisitor intersectVisitor;
  intersectVisitor.setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
  osg::ref_ptr<osg::LineSegment> lineSegment;
  lineSegment = new osg::LineSegment(toOsg(start), toOsg(end));
  intersectVisitor.addLineSegment(lineSegment.get());
  get_scene_graph()->accept(intersectVisitor);
  bool hits = false;
  if (intersectVisitor.hits()) {
    int nHits = intersectVisitor.getNumHits(lineSegment.get());
    alt = -SGLimitsd::max();
    for (int i = 0; i < nHits; ++i) {
      const osgUtil::Hit& hit
        = intersectVisitor.getHitList(lineSegment.get())[i];
      if (butNotFrom &&
          std::find(hit.getNodePath().begin(), hit.getNodePath().end(),
                    butNotFrom) != hit.getNodePath().end())
          continue;

      // We might need the double variant of the intersection point.
      // Thus we cannot use the float variant delivered by
      // hit.getWorldIntersectPoint() but we have to redo that with osg::Vec3d.
      osg::Vec3d point = hit.getLocalIntersectPoint();
      if (hit.getMatrix())
        point = point*(*hit.getMatrix());
      SGGeod geod = SGGeod::fromCart(toSG(point));
      double elevation = geod.getElevationM();
      if (alt < elevation) {
        alt = elevation;
        hits = true;
        if (material)
          *material = SGMaterialLib::findMaterial(hit.getGeode());
      }
    }
  }

  return hits;
}

bool
FGScenery::get_cart_ground_intersection(const SGVec3d& pos, const SGVec3d& dir,
                                        SGVec3d& nearestHit,
                                        const osg::Node* butNotFrom)
{
  // We assume that starting positions in the center of the earth are invalid
  if ( norm1(pos) < 1 )
    return false;

  // Make really sure the direction is normalized, is really cheap compared to
  // computation of ground intersection.
  SGVec3d start = pos;
  SGVec3d end = start + 1e5*normalize(dir); // FIXME visibility ???
  
  osgUtil::IntersectVisitor intersectVisitor;
  intersectVisitor.setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
  osg::ref_ptr<osg::LineSegment> lineSegment;
  lineSegment = new osg::LineSegment(toOsg(start), toOsg(end));
  intersectVisitor.addLineSegment(lineSegment.get());
  get_scene_graph()->accept(intersectVisitor);
  bool hits = false;
  if (intersectVisitor.hits()) {
    int nHits = intersectVisitor.getNumHits(lineSegment.get());
    double dist = SGLimitsd::max();
    for (int i = 0; i < nHits; ++i) {
      const osgUtil::Hit& hit
        = intersectVisitor.getHitList(lineSegment.get())[i];
      if (butNotFrom &&
          std::find(hit.getNodePath().begin(), hit.getNodePath().end(),
                    butNotFrom) != hit.getNodePath().end())
          continue;
      // We might need the double variant of the intersection point.
      // Thus we cannot use the float variant delivered by
      // hit.getWorldIntersectPoint() but we have to redo that with osg::Vec3d.
      osg::Vec3d point = hit.getLocalIntersectPoint();
      if (hit.getMatrix())
        point = point*(*hit.getMatrix());
      double newdist = length(start - toSG(point));
      if (newdist < dist) {
        dist = newdist;
        nearestHit = toSG(point);
        hits = true;
      }
    }
  }

  return hits;
}

bool FGScenery::scenery_available(const SGGeod& position, double range_m)
{
  if(globals->get_tile_mgr()->schedule_scenery(position, range_m, 0.0))
  {
    double elev;
    if (!get_elevation_m(SGGeod::fromGeodM(position, SG_MAX_ELEVATION_M), elev, 0, 0))
      return false;
    SGVec3f p = SGVec3f::fromGeod(SGGeod::fromGeodM(position, elev));
    osg::FrameStamp* framestamp
            = globals->get_renderer()->getViewer()->getFrameStamp();
    simgear::CheckSceneryVisitor csnv(getPagerSingleton(), toOsg(p), range_m, framestamp);
    // currently the PagedLODs will not be loaded by the DatabasePager
    // while the splashscreen is there, so CheckSceneryVisitor force-loads
    // missing objects in the main thread
    get_scene_graph()->accept(csnv);
    if(!csnv.isLoaded())
        return false;
    return true;
  }
  return false;
}

SceneryPager* FGScenery::getPagerSingleton()
{
    static osg::ref_ptr<SceneryPager> pager = new SceneryPager;
    return pager.get();
}

