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


#include <config.h>
#include <simgear/simgear_config.h>

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
#include <simgear/scene/util/SGSceneFeatures.hxx>

#include <simgear/bvh/BVHNode.hxx>
#include <simgear/bvh/BVHLineSegmentVisitor.hxx>
#include <simgear/structure/commands.hxx>

#include <Viewer/renderer.hxx>
#include <Main/fg_props.hxx>
#include <GUI/MouseCursor.hxx>
#include <Main/sentryIntegration.hxx>

#include "scenery.hxx"
#include "terrain_stg.hxx"

#ifdef ENABLE_GDAL
#include "terrain_pgt.hxx"
#endif

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
class FGScenery::TextureCacheListener : public SGPropertyChangeListener
{
protected:
    const char* root_node_path = "/sim/rendering/texture-cache";
public:
    TextureCacheListener()
    {
        SGPropertyNode_ptr textureCacheNode = fgGetNode(root_node_path, true);
        setupPropertyListener(textureCacheNode, "cache-enabled");
        setupPropertyListener(textureCacheNode, "compress-transparent");
        setupPropertyListener(textureCacheNode, "compress-solid");
        setupPropertyListener(textureCacheNode, "compress");
    }

    ~TextureCacheListener()
    {
        SGPropertyNode_ptr maskNode = fgGetNode(root_node_path);
        for (int i = 0; i < maskNode->nChildren(); ++i) {
            maskNode->getChild(i)->removeChangeListener(this);
        }
    }

    void setupPropertyListener(SGPropertyNode_ptr textureCacheNode, const char *node)
    {
        textureCacheNode->getChild(node, 0, true)->addChangeListener(this, true);
    }

    virtual void valueChanged(SGPropertyNode * node)
    {
        bool b = node->getBoolValue();
        std::string name(node->getNameString());

        if (name == "cache-enabled") {
            SGSceneFeatures::instance()->setTextureCacheActive(b);
        }
        else if (name == "compress-transparent" || name == "compress") {
            SGSceneFeatures::instance()->setTextureCacheCompressionActiveTransparent(b);
        }
        else if (name == "compress-solid" || name == "compress") {
            SGSceneFeatures::instance()->setTextureCacheCompressionActive(b);
        }
    }
};

class FGScenery::ElevationMeshListener : public SGPropertyChangeListener
{
protected:
    const char* root_node_path = "/scenery/elevation-mesh";
public:
    ElevationMeshListener()
    {
        SGPropertyNode_ptr elevationMeshNode = fgGetNode(root_node_path, true);
        setupPropertyListener(elevationMeshNode, "constraint-gap-m");
        setupPropertyListener(elevationMeshNode, "sample-ratio");
        setupPropertyListener(elevationMeshNode, "vertical-scale");
    }

    ~ElevationMeshListener()
    {
        SGPropertyNode_ptr node = fgGetNode(root_node_path);
        for (int i = 0; i < node->nChildren(); ++i) {
            node->getChild(i)->removeChangeListener(this);
        }
    }

    void setupPropertyListener(SGPropertyNode_ptr elevationMeshNode, const char *node)
    {
        elevationMeshNode->getChild(node, 0, true)->addChangeListener(this, true);
    }

    virtual void valueChanged(SGPropertyNode * node)
    {
        float f = node->getFloatValue();
        std::string name(node->getNameString());

        if (name == "constraint-gap-m") {
            SGSceneFeatures::instance()->setVPBConstraintGap(f);
        } else if (name == "sample-ratio") {
            SGSceneFeatures::instance()->setVPBSampleRatio(f);
        } else if (name == "vertical-scale") {
            SGSceneFeatures::instance()->setVPBVerticalScale(f);
        } else {
            SG_LOG(SG_TERRAIN, SG_ALERT, "Unexpected property in listener " << node->getPath());
        }
    }
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

    auto vpb_active = fgGetNode("/scenery/use-vpb");
    if (vpb_active) {
        vpb_active->addChangeListener(this);
        SGSceneFeatures::instance()->setVPBActive(vpb_active->getBoolValue());
        flightgear::addSentryTag("use-vpb", "yes");

    } else {
        flightgear::addSentryTag("use-vpb", "no");
    }
  }

  ~ScenerySwitchListener()
  {
    SGPropertyNode_ptr maskNode = fgGetNode("/sim/rendering/draw-mask");
    for (int i=0; i < maskNode->nChildren(); ++i) {
      maskNode->getChild(i)->removeChangeListener(this);
    }

    fgGetNode("/sim/rendering/draw-otw")->removeChangeListener(this);
    fgGetNode("/environment/clouds/status")->removeChangeListener(this);
    fgGetNode("/scenery/use-vpb")->removeChangeListener(this);
  }

  virtual void valueChanged (SGPropertyNode * node)
  {
    bool b = node->getBoolValue();
    std::string name(node->getNameString());

    if (name == "use-vpb") {
        SGSceneFeatures::instance()->setVPBActive(b);
    } else if (name == "terrain") {
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
    _listener(nullptr), _textureCacheListener(nullptr), _elevationMeshListener(nullptr)
{
    // keep reference to pager singleton, so it cannot be destroyed while FGScenery lives
    _pager = FGScenery::getPagerSingleton();

    // Initialise the state of the scene graph.
    _inited = false;
}

FGScenery::~FGScenery()
{
    delete _listener;
    delete _textureCacheListener;
}


// Initialize the Scenery Management system
void FGScenery::init() {
    // Already set up.
    if (_inited)
        return;

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

    // Set up the particle system as a directly accessible branch of the scene graph.
    auto paricles = simgear::ParticlesGlobalManager::instance();
    particles_branch = paricles->getCommonRoot();
    particles_branch->setName("Particles");
    scene_graph->addChild(particles_branch.get());
    paricles->setSwitchNode(fgGetNode("/sim/rendering/particles", true));
    paricles->initFromMainThread();

    // Set up the precipitation system.
    precipitation_branch = new osg::Group;
    precipitation_branch->setName("Precipitation");
    scene_graph->addChild(precipitation_branch.get());

    // initialize the terrian based on selected engine
    std::string engine = fgGetString("/sim/scenery/engine", "tilecache" );
    SG_LOG( SG_TERRAIN, SG_INFO, "Selected scenery is " << engine );

    if ( engine == "pagedLOD" ) {
#ifdef ENABLE_GDAL
        _terrain.reset(new FGPgtTerrain);
#else
        _terrain.reset(new FGStgTerrain);
#endif
    } else {
        _terrain.reset(new FGStgTerrain);
    }
    _terrain->init( terrain_branch.get() );

    _listener = new ScenerySwitchListener(this);
    _textureCacheListener = new TextureCacheListener();
    _elevationMeshListener = new ElevationMeshListener();

    // Toggle the setup flag.
    _inited = true;
}

void FGScenery::reinit()
{
    flightgear::addSentryBreadcrumb("reloading scenery", "info");
    fgSetBool("/sim/rendering/scenery-reload-required", false);
    _terrain->reinit();
}

void FGScenery::shutdown()
{
    _terrain->shutdown();

    scene_graph = NULL;
    terrain_branch = NULL;
    models_branch = NULL;
    aircraft_branch = NULL;
    particles_branch = NULL;
    precipitation_branch = NULL;

    _terrain.reset();

    // Toggle the setup flag.
    _inited = false;

    simgear::ParticlesGlobalManager::clear();
}


void FGScenery::update(double dt)
{
    _terrain->update(dt);
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
    return _terrain->get_cart_elevation_m(pos, max_altoff, alt,
                                          material, butNotFrom);
}

bool
FGScenery::get_elevation_m(const SGGeod& geod, double& alt,
                           const simgear::BVHMaterial** material,
                           const osg::Node* butNotFrom)
{
    return _terrain->get_elevation_m( geod, alt, material,
                                      butNotFrom );
}

bool
FGScenery::get_cart_ground_intersection(const SGVec3d& pos, const SGVec3d& dir,
                                        SGVec3d& nearestHit,
                                        const osg::Node* butNotFrom)
{
    return _terrain->get_cart_ground_intersection( pos, dir, nearestHit, butNotFrom );
}

bool FGScenery::scenery_available(const SGGeod& position, double range_m)
{
    return _terrain->scenery_available( position, range_m );
}

bool FGScenery::schedule_scenery(const SGGeod& position, double range_m, double duration)
{
    return _terrain->schedule_scenery( position, range_m, duration );
}

void FGScenery::materialLibChanged()
{
    _terrain->materialLibChanged();
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


// Register the subsystem.
SGSubsystemMgr::Registrant<FGScenery> registrantFGScenery(
    SGSubsystemMgr::DISPLAY,
    {{"FGRenderer", SGSubsystemMgr::Dependency::NONSUBSYSTEM_HARD},
        {"SGSky", SGSubsystemMgr::Dependency::NONSUBSYSTEM_HARD}});
