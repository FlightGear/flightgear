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

#include <osg/Camera>
#include <osg/Transform>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/CameraView>
#include <osg/LOD>

#include <osgViewer/Viewer>

#include <simgear/constants.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/model/CheckSceneryVisitor.hxx>
#include <simgear/scene/sky/sky.hxx>

#include <simgear/bvh/BVHNode.hxx>
#include <simgear/bvh/BVHLineSegmentVisitor.hxx>
#include <simgear/structure/commands.hxx>

#include <Viewer/renderer.hxx>
#include <Main/fg_props.hxx>
#include <GUI/MouseCursor.hxx>

#include "tilemgr.hxx"
#include "scenery.hxx"

using namespace flightgear;
using namespace simgear;

class FGGroundPickCallback : public SGPickCallback {
public:
  FGGroundPickCallback() : SGPickCallback(PriorityScenery)
  { }
    
  virtual bool buttonPressed( int button,
                              const osgGA::GUIEventAdapter&,
                              const Info& info )
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

class FGSceneryIntersect : public osg::NodeVisitor {
public:
    FGSceneryIntersect(const SGLineSegmentd& lineSegment,
                       const osg::Node* skipNode) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
        _lineSegment(lineSegment),
        _skipNode(skipNode),
        _material(0),
        _haveHit(false)
    { }

    bool getHaveHit() const
    { return _haveHit; }
    const SGLineSegmentd& getLineSegment() const
    { return _lineSegment; }
    const simgear::BVHMaterial* getMaterial() const
    { return _material; }

    virtual void apply(osg::Node& node)
    {
        if (&node == _skipNode)
            return;
        if (!testBoundingSphere(node.getBound()))
            return;

        addBoundingVolume(node);
    }

    virtual void apply(osg::Group& group)
    {
        if (&group == _skipNode)
            return;
        if (!testBoundingSphere(group.getBound()))
            return;

        traverse(group);
        addBoundingVolume(group);
    }

    virtual void apply(osg::Transform& transform)
    { handleTransform(transform); }
    virtual void apply(osg::Camera& camera)
    {
        if (camera.getRenderOrder() != osg::Camera::NESTED_RENDER)
            return;
        handleTransform(camera);
    }
    virtual void apply(osg::CameraView& transform)
    { handleTransform(transform); }
    virtual void apply(osg::MatrixTransform& transform)
    { handleTransform(transform); }
    virtual void apply(osg::PositionAttitudeTransform& transform)
    { handleTransform(transform); }

private:
    void handleTransform(osg::Transform& transform)
    {
        if (&transform == _skipNode)
            return;
        // Hmm, may be this needs to be refined somehow ...
        if (transform.getReferenceFrame() != osg::Transform::RELATIVE_RF)
            return;

        if (!testBoundingSphere(transform.getBound()))
            return;

        osg::Matrix inverseMatrix;
        if (!transform.computeWorldToLocalMatrix(inverseMatrix, this))
            return;
        osg::Matrix matrix;
        if (!transform.computeLocalToWorldMatrix(matrix, this))
            return;

        SGLineSegmentd lineSegment = _lineSegment;
        bool haveHit = _haveHit;
        const simgear::BVHMaterial* material = _material;

        _haveHit = false;
        _lineSegment = lineSegment.transform(SGMatrixd(inverseMatrix.ptr()));

        addBoundingVolume(transform);
        traverse(transform);

        if (_haveHit) {
            _lineSegment = _lineSegment.transform(SGMatrixd(matrix.ptr()));
        } else {
            _lineSegment = lineSegment;
            _material = material;
            _haveHit = haveHit;
        }
    }

    simgear::BVHNode* getNodeBoundingVolume(osg::Node& node)
    {
        SGSceneUserData* userData = SGSceneUserData::getSceneUserData(&node);
        if (!userData)
            return 0;
        return userData->getBVHNode();
    }
    void addBoundingVolume(osg::Node& node)
    {
        simgear::BVHNode* bvNode = getNodeBoundingVolume(node);
        if (!bvNode)
            return;

        // Find ground intersection on the bvh nodes
        simgear::BVHLineSegmentVisitor lineSegmentVisitor(_lineSegment,
                                                          0/*startTime*/);
        bvNode->accept(lineSegmentVisitor);
        if (!lineSegmentVisitor.empty()) {
            _lineSegment = lineSegmentVisitor.getLineSegment();
            _material = lineSegmentVisitor.getMaterial();
            _haveHit = true;
        }
    }

    bool testBoundingSphere(const osg::BoundingSphere& bound) const
    {
        if (!bound.valid())
            return false;

        SGSphered sphere(toVec3d(toSG(bound._center)), bound._radius);
        return intersects(_lineSegment, sphere);
    }

    SGLineSegmentd _lineSegment;
    const osg::Node* _skipNode;

    const simgear::BVHMaterial* _material;
    bool _haveHit;
};

class FGScenery::ScenerySwitchListener : public SGPropertyChangeListener
{
public:
  ScenerySwitchListener(FGScenery* scenery) :
    _scenery(scenery)
  {
    SGPropertyNode_ptr maskNode = fgGetNode("/sim/rendering/draw-mask", true);
    maskNode->getChild("terrain", 0, true)->addChangeListener(this, true);
    maskNode->getChild("models", 0, true)->addChangeListener(this, true);
    maskNode->getChild("aircraft", 0, true)->addChangeListener(this, true);
    maskNode->getChild("clouds", 0, true)->addChangeListener(this, true);
    
    // legacy compatability option
    fgGetNode("/sim/rendering/draw-otw")->addChangeListener(this);
    
    // badly named property, this is what is set by --enable/disable-clouds
    fgGetNode("/environment/clouds/status")->addChangeListener(this);
  }
  
  ~ScenerySwitchListener()
  {
    SGPropertyNode_ptr maskNode = fgGetNode("/sim/rendering/draw-mask");
    for (int i=0; i < maskNode->nChildren(); ++i) {
      maskNode->getChild(i)->removeChangeListener(this);
    }
    
    fgGetNode("/sim/rendering/draw-otw")->removeChangeListener(this);
    fgGetNode("/environment/clouds/status")->removeChangeListener(this);
  }
  
  virtual void valueChanged (SGPropertyNode * node)
  {
    bool b = node->getBoolValue();
    std::string name(node->getNameString());
  
    if (name == "terrain") {
      _scenery->scene_graph->setChildValue(_scenery->terrain_branch, b);
    } else if (name == "models") {
      _scenery->scene_graph->setChildValue(_scenery->models_branch, b);
    } else if (name == "aircraft") {
      _scenery->scene_graph->setChildValue(_scenery->aircraft_branch, b);
    } else if (name == "clouds") {
      // clouds live elsewhere in the scene, but we handle them here
      globals->get_renderer()->getSky()->set_clouds_enabled(b);
    } else if (name == "draw-otw") {
      // legacy setting but let's keep it working
      fgGetNode("/sim/rendering/draw-mask")->setBoolValue("terrain", b);
      fgGetNode("/sim/rendering/draw-mask")->setBoolValue("models", b);
    } else if (name == "status") {
      fgGetNode("/sim/rendering/draw-mask")->setBoolValue("clouds", b);
    }
  }
private:
  FGScenery* _scenery;
};

////////////////////////////////////////////////////////////////////////////

// Scenery Management system
FGScenery::FGScenery() :
    _listener(NULL)
{
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing scenery subsystem" );
    // keep reference to pager singleton, so it cannot be destroyed while FGScenery lives
    _pager = FGScenery::getPagerSingleton();
}

FGScenery::~FGScenery()
{
    delete _listener;
}


// Initialize the Scenery Management system
void FGScenery::init() {
    // Scene graph root
    scene_graph = new osg::Switch;
    scene_graph->setName( "FGScenery" );

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

// choosing to make the interior branch a child of the main
// aircraft group, for the moment. This simplifes places which
// assume all aircraft elements are within this group - principally
// FGODGuage::set_aircraft_texture.
    interior_branch = new osg::Group;
    interior_branch->setName( "Interior" );
    
    osg::LOD* interiorLOD = new osg::LOD;
    interiorLOD->addChild(interior_branch.get(), 0.0, 50.0);
    aircraft_branch->addChild( interiorLOD );
    
    // Initials values needed by the draw-time object loader
    sgUserDataInit( globals->get_props() );
  
    _listener = new ScenerySwitchListener(this);
}

void FGScenery::shutdown()
{
    sgUserDataInit( NULL );
    
    scene_graph = NULL;
    terrain_branch = NULL;
    models_branch = NULL;
    aircraft_branch = NULL;
}


void FGScenery::update(double dt)
{
    SG_UNUSED(dt);
    // nothing here, don't call again
    suspend();
}


void FGScenery::bind() {
}


void FGScenery::unbind() {
}

bool
FGScenery::get_cart_elevation_m(const SGVec3d& pos, double max_altoff,
                                double& alt,
                                const simgear::BVHMaterial** material,
                                const osg::Node* butNotFrom)
{
  SGGeod geod = SGGeod::fromCart(pos);
  geod.setElevationM(geod.getElevationM() + max_altoff);
  return get_elevation_m(geod, alt, material, butNotFrom);
}

bool
FGScenery::get_elevation_m(const SGGeod& geod, double& alt,
                           const simgear::BVHMaterial** material,
                           const osg::Node* butNotFrom)
{
  SGVec3d start = SGVec3d::fromGeod(geod);

  SGGeod geodEnd = geod;
  geodEnd.setElevationM(SGMiscd::min(geod.getElevationM() - 10, -10000));
  SGVec3d end = SGVec3d::fromGeod(geodEnd);

  FGSceneryIntersect intersectVisitor(SGLineSegmentd(start, end), butNotFrom);
  intersectVisitor.setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
  get_scene_graph()->accept(intersectVisitor);

  if (!intersectVisitor.getHaveHit())
      return false;

  geodEnd = SGGeod::fromCart(intersectVisitor.getLineSegment().getEnd());
  alt = geodEnd.getElevationM();
  if (material)
      *material = intersectVisitor.getMaterial();

  return true;
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

  FGSceneryIntersect intersectVisitor(SGLineSegmentd(start, end), butNotFrom);
  intersectVisitor.setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
  get_scene_graph()->accept(intersectVisitor);

  if (!intersectVisitor.getHaveHit())
      return false;

  nearestHit = intersectVisitor.getLineSegment().getEnd();
  return true;
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
    simgear::CheckSceneryVisitor csnv(_pager, toOsg(p), range_m, framestamp);
    // currently the PagedLODs will not be loaded by the DatabasePager
    // while the splashscreen is there, so CheckSceneryVisitor force-loads
    // missing objects in the main thread
    get_scene_graph()->accept(csnv);
    if(!csnv.isLoaded()) {
        SG_LOG(SG_TERRAIN, SG_DEBUG, "FGScenery::scenery_available: waiting on CheckSceneryVisitor");
        return false;
    }
    return true;
  } else {
    SG_LOG(SG_TERRAIN, SG_DEBUG, "FGScenery::scenery_available: waiting on tile manager");
  }
  return false;
}

static osg::ref_ptr<SceneryPager> pager;

SceneryPager* FGScenery::getPagerSingleton()
{
    if (!pager)
        pager = new SceneryPager;
    return pager.get();
}

void FGScenery::resetPagerSingleton()
{
    pager = NULL;
}

