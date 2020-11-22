// renderer.cxx -- top level sim routines
//
// Written by Curtis Olson, started May 1997.
// This file contains parts of main.cxx prior to october 2004
//
// Copyright (C) 1997 - 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <config.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
#include <typeinfo>

#include <osg/ref_ptr>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/CullFace>
#include <osg/CullStack>
#include <osg/Depth>
#include <osg/Fog>
#include <osg/Group>
#include <osg/Hint>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/LightSource>
#include <osg/Material>
#include <osg/Math>
#include <osg/NodeCallback>
#include <osg/Notify>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/Program>
#include <osg/Version>
#include <osg/TexEnv>

#include <osgUtil/LineSegmentIntersector>

#include <osg/io_utils>
#include <osgDB/WriteFile>
#include <osgViewer/Renderer>

#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/EffectBuilder.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/scene/util/DeletionManager.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/tgdb/GroundLightManager.hxx>
#include <simgear/scene/tgdb/pt_lights.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/structure/OSGUtils.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/sg_random.h>

#include <Time/light.hxx>
#include <Time/light.hxx>
#include <Cockpit/panel.hxx>

#include <Model/panelnode.hxx>
#include <Model/modelmgr.hxx>
#include <Model/acmodel.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/redout.hxx>
#include <GUI/new_gui.hxx>
#include <GUI/gui.h>

#include <Instrumentation/HUD/HUD.hxx>
#include <Environment/precipitation_mgr.hxx>
#include <Environment/environment_mgr.hxx>
#include <Environment/ephemeris.hxx>

//#include <Main/main.hxx>
#include "view.hxx"
#include "viewmgr.hxx"
#include "splash.hxx"
#include "renderer.hxx"
#include "CameraGroup.hxx"
#include "FGEventHandler.hxx"
#include <Main/sentryIntegration.hxx>

#if defined(ENABLE_QQ_UI)
#include <GUI/QQuickDrawable.hxx>
#endif

#if defined(HAVE_PUI)
#include <Viewer/PUICamera.hxx>
#endif

using namespace osg;
using namespace flightgear;

class FGHintUpdateCallback : public osg::StateAttribute::Callback {
public:
  FGHintUpdateCallback(const char* configNode) :
    mConfigNode(fgGetNode(configNode, true))
  { }
  virtual void operator()(osg::StateAttribute* stateAttribute,
                          osg::NodeVisitor*)
  {
    assert(dynamic_cast<osg::Hint*>(stateAttribute));
    osg::Hint* hint = static_cast<osg::Hint*>(stateAttribute);

    const char* value = mConfigNode->getStringValue();
    if (!value)
      hint->setMode(GL_DONT_CARE);
    else if (0 == strcmp(value, "nicest"))
      hint->setMode(GL_NICEST);
    else if (0 == strcmp(value, "fastest"))
      hint->setMode(GL_FASTEST);
    else
      hint->setMode(GL_DONT_CARE);
  }
private:
  SGPropertyNode_ptr mConfigNode;
};


class SGHUDDrawable : public osg::Drawable {
public:
  SGHUDDrawable()
  {
    // Dynamic stuff, do not store geometry
    setUseDisplayList(false);
    setDataVariance(Object::DYNAMIC);

    osg::StateSet* stateSet = getOrCreateStateSet();
    stateSet->setRenderBinDetails(1000, "RenderBin");

    // speed optimization?
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setAttribute(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    stateSet->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::MODULATE));
  }
  virtual void drawImplementation(osg::RenderInfo& renderInfo) const
  { drawImplementation(*renderInfo.getState()); }
  void drawImplementation(osg::State& state) const
  {
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);
    state.disableAllVertexArrays();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(~0u);

    // HUD can be NULL
      HUD *hud = static_cast<HUD*>(globals->get_subsystem("hud"));
      if (hud) {
          hud->draw(state);
      }

    glPopClientAttrib();
    glPopAttrib();
  }

  virtual osg::Object* cloneType() const { return new SGHUDDrawable; }
  virtual osg::Object* clone(const osg::CopyOp&) const { return new SGHUDDrawable; }

private:
};

class FGLightSourceUpdateCallback : public osg::NodeCallback {
public:

  /**
   * @param isSun true if the light is the actual sun i.e., for
   * illuminating the moon.
   */
  FGLightSourceUpdateCallback(bool isSun = false) : _isSun(isSun) {}
  FGLightSourceUpdateCallback(const FGLightSourceUpdateCallback& nc,
                              const CopyOp& op)
    : NodeCallback(nc, op), _isSun(nc._isSun)
  {}
  META_Object(flightgear,FGLightSourceUpdateCallback);

  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::LightSource*>(node));
    osg::LightSource* lightSource = static_cast<osg::LightSource*>(node);
    osg::Light* light = lightSource->getLight();

    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));
      if (!l) {
          // lighting is down during re-init
          return;
      }

    if (_isSun) {
      light->setAmbient(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
      light->setDiffuse(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
      light->setSpecular(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    } else {
      light->setAmbient(toOsg(l->scene_ambient()));
      light->setDiffuse(toOsg(l->scene_diffuse()));
      light->setSpecular(toOsg(l->scene_specular()));
    }
    osg::Vec4f position(l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2], 0);
    light->setPosition(position);

    traverse(node, nv);
  }
private:
  const bool _isSun;
};

class FGWireFrameModeUpdateCallback : public osg::StateAttribute::Callback {
public:
  FGWireFrameModeUpdateCallback() :
    mWireframe(fgGetNode("/sim/rendering/wireframe", true))
  { }
  virtual void operator()(osg::StateAttribute* stateAttribute,
                          osg::NodeVisitor*)
  {
    assert(dynamic_cast<osg::PolygonMode*>(stateAttribute));
    osg::PolygonMode* polygonMode;
    polygonMode = static_cast<osg::PolygonMode*>(stateAttribute);

    if (mWireframe->getBoolValue())
      polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                           osg::PolygonMode::LINE);
    else
      polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                           osg::PolygonMode::FILL);
  }
private:
  SGPropertyNode_ptr mWireframe;
};

class FGLightModelUpdateCallback : public osg::StateAttribute::Callback {
public:
  FGLightModelUpdateCallback() :
    mHighlights(fgGetNode("/sim/rendering/specular-highlight", true))
  { }
  virtual void operator()(osg::StateAttribute* stateAttribute,
                          osg::NodeVisitor*)
  {
    assert(dynamic_cast<osg::LightModel*>(stateAttribute));
    osg::LightModel* lightModel;
    lightModel = static_cast<osg::LightModel*>(stateAttribute);

#if 0
    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));
    lightModel->setAmbientIntensity(toOsg(l->scene_ambient());
#else
    lightModel->setAmbientIntensity(osg::Vec4(0, 0, 0, 1));
#endif
    lightModel->setTwoSided(true);
    lightModel->setLocalViewer(false);

    if (mHighlights->getBoolValue()) {
      lightModel->setColorControl(osg::LightModel::SEPARATE_SPECULAR_COLOR);
    } else {
      lightModel->setColorControl(osg::LightModel::SINGLE_COLOR);
    }
  }
private:
  SGPropertyNode_ptr mHighlights;
};

class FGFogEnableUpdateCallback : public osg::StateSet::Callback {
public:
  FGFogEnableUpdateCallback() :
    mFogEnabled(fgGetNode("/sim/rendering/fog", true))
  { }
  virtual void operator()(osg::StateSet* stateSet, osg::NodeVisitor*)
  {
    if (strcmp(mFogEnabled->getStringValue(), "disabled") == 0) {
      stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    } else {
      stateSet->setMode(GL_FOG, osg::StateAttribute::ON);
    }
  }
private:
  SGPropertyNode_ptr mFogEnabled;
};

class FGFogUpdateCallback : public osg::StateAttribute::Callback {
public:
  virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<SGUpdateVisitor*>(nv));
    assert(dynamic_cast<osg::Fog*>(sa));
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);
    osg::Fog* fog = static_cast<osg::Fog*>(sa);
    fog->setMode(osg::Fog::EXP2);
    fog->setColor(toOsg(updateVisitor->getFogColor()));
    fog->setDensity(updateVisitor->getFogExp2Density());
  }
};

// update callback for the switch node guarding that splash
class FGScenerySwitchCallback : public osg::NodeCallback {
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::Switch*>(node));
    osg::Switch* sw = static_cast<osg::Switch*>(node);

    bool enabled = scenery_enabled;
    sw->setValue(0, enabled);
    if (!enabled)
      return;
    traverse(node, nv);
  }

  static bool scenery_enabled;
};

bool FGScenerySwitchCallback::scenery_enabled = false;

FGRenderer::FGRenderer() :
    _sky(NULL),
    MaximumTextureSize(0)
{
	_root = new osg::Group;
	_root->setName("fakeRoot");
    _updateVisitor = new SGUpdateVisitor;
}

FGRenderer::~FGRenderer()
{
    SGPropertyChangeListenerVec::iterator i = _listeners.begin();
    for (; i != _listeners.end(); ++i) {
        delete *i;
    }

	// replace the viewer's scene completely
    if (getView()) {
        getView()->setSceneData(new osg::Group);
    }

    delete _sky;
}

class PointSpriteListener : public SGPropertyChangeListener {
public:
    virtual void valueChanged(SGPropertyNode* node) {
        static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
        sceneFeatures->setEnablePointSpriteLights(node->getIntValue());
    }
};

class DistanceAttenuationListener : public SGPropertyChangeListener {
public:
    virtual void valueChanged(SGPropertyNode* node) {
        static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
        sceneFeatures->setEnableDistanceAttenuationLights(node->getIntValue());
    }
};

class DirectionalLightsListener : public SGPropertyChangeListener {
public:
    virtual void valueChanged(SGPropertyNode* node) {
        static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
        sceneFeatures->setEnableTriangleDirectionalLights(node->getIntValue());
    }
};

void
FGRenderer::addChangeListener(SGPropertyChangeListener* l, const char* path)
{
    _listeners.push_back(l);
    fgAddChangeListener(l, path);
}

// Initialize various GL/view parameters
//
// Note that this appears to be called *after* FGRenderer::init().
//
void
FGRenderer::preinit( void )
{
    // important that we reset the viewer sceneData here, to ensure the reference
    // time for everything is in sync; otherwise on reset the Viewer and
    // GraphicsWindow clocks are out of sync.
    osgViewer::View* view = getView();
    view->setName("osgViewer");
    _viewerSceneRoot = new osg::Group;
    _viewerSceneRoot->setName("viewerSceneRoot");
    view->setSceneData(_viewerSceneRoot);
    view->setDatabasePager(FGScenery::getPagerSingleton());

    _quickDrawable = nullptr;
    _splash = new SplashScreen;
	_viewerSceneRoot->addChild(_splash);

    if (composite_viewer) {
        // Nothing to do - composite_viewer->addView() will tell view to use
        // composite_viewer's FrameStamp.
    }
    else {
        _frameStamp = new osg::FrameStamp;
        view->setFrameStamp(_frameStamp.get());
    }
    
    // Scene doesn't seem to pass the frame stamp to the update
    // visitor automatically.
    _updateVisitor->setFrameStamp(getFrameStamp());
    getViewerBase()->setUpdateVisitor(_updateVisitor.get());
    fgSetDouble("/sim/startup/splash-alpha", 1.0);

    // hide the menubar if it overlaps the window, so the splash screen
    // is completely visible. We reset this value when the splash screen
    // is fading out.
    fgSetBool("/sim/menubar/overlap-hide", true);
}

void
FGRenderer::init( void )
{
    if (!eventHandler)
        eventHandler = new FGEventHandler();

    sgUserDataInit( globals->get_props() );

    SGPropertyNode* composite_viewer_enabled_prop = fgGetNode("/sim/rendering/composite-viewer-enabled", true);
    // After we've read composite_viewer_enabled_prop here, changing its value
    // will have no affect, so mark it as read-only for clarity.
    composite_viewer_enabled_prop->setAttributes(SGPropertyNode::READ);
    if (composite_viewer_enabled_prop->getBoolValue()) {
        const char* osg_version = osgGetVersion();
        if (simgear::strutils::starts_with(osg_version, "3.4")) {
            SG_LOG( SG_GENERAL, SG_POPUP,
                    "CompositeViewer is enabled and requires OpenSceneGraph-3.6, but\n"
                    " Flightgear has been built with OpenSceneGraph-" << osg_version << ".\n"
                    " There may be problems when opening/closing extra view windows.\n"
                    );
        }
        composite_viewer_enabled = 1;
        SG_LOG(SG_VIEW, SG_ALERT, "Creating osgViewer::CompositeViewer");
        composite_viewer = new osgViewer::CompositeViewer;
        
        // https://stackoverflow.com/questions/15207076/openscenegraph-and-multiple-viewers
        composite_viewer->setReleaseContextAtEndOfFrameHint(false);
        composite_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    }
    else {
        composite_viewer_enabled = 0;
        SG_LOG(SG_VIEW, SG_ALERT, "Not creating osgViewer::CompositeViewer");
    }
    _scenery_loaded   = fgGetNode("/sim/sceneryloaded", true);
    _position_finalized = fgGetNode("/sim/position-finalized", true);

    _panel_hotspots   = fgGetNode("/sim/panel-hotspots", true);
    _virtual_cockpit  = fgGetNode("/sim/virtual-cockpit", true);

    _sim_delta_sec = fgGetNode("/sim/time/delta-sec", true);

    _xsize         = fgGetNode("/sim/startup/xsize", true);
    _ysize         = fgGetNode("/sim/startup/ysize", true);
    _splash_alpha  = fgGetNode("/sim/startup/splash-alpha", true);

    _horizon_effect       = fgGetNode("/sim/rendering/horizon-effect", true);

    _altitude_ft = fgGetNode("/position/altitude-ft", true);

    _cloud_status = fgGetNode("/environment/clouds/status", true);
    _visibility_m = fgGetNode("/environment/visibility-m", true);

    // configure the lighting related parameters and add change listeners.
    bool use_point_sprites = fgGetBool("/sim/rendering/point-sprites", true);
    bool distance_attenuation = fgGetBool("/sim/rendering/distance-attenuation", false);
    bool triangles = fgGetBool("/sim/rendering/triangle-directional-lights", true);
    SGConfigureDirectionalLights(use_point_sprites, distance_attenuation, triangles);

    addChangeListener(new PointSpriteListener,  "/sim/rendering/point-sprites");
    addChangeListener(new DistanceAttenuationListener, "/sim/rendering/distance-attenuation");
    addChangeListener(new DirectionalLightsListener, "/sim/rendering/triangle-directional-lights");

    if (const char* tc = fgGetString("/sim/rendering/texture-compression", NULL)) {
      if (strcmp(tc, "false") == 0 || strcmp(tc, "off") == 0 ||
          strcmp(tc, "0") == 0 || strcmp(tc, "no") == 0 ||
          strcmp(tc, "none") == 0) {
        SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::DoNotUseCompression);
      } else if (strcmp(tc, "arb") == 0) {
        SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::UseARBCompression);
      } else if (strcmp(tc, "dxt1") == 0) {
        SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::UseDXT1Compression);
      } else if (strcmp(tc, "dxt3") == 0) {
        SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::UseDXT3Compression);
      } else if (strcmp(tc, "dxt5") == 0) {
        SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::UseDXT5Compression);
      } else {
        SG_LOG(SG_VIEW, SG_WARN, "Unknown texture compression setting!");
      }
    }
    SGSceneFeatures::instance()->setTextureCompressionPath(globals->get_texture_cache_dir());
// create sky, but can't build until setupView, since we depend
// on other subsystems to be inited, eg Ephemeris
    _sky = new SGSky;

    const SGPath texture_path = globals->get_fg_root() / "Textures" / "Sky";
    for (int i = 0; i < FGEnvironmentMgr::MAX_CLOUD_LAYERS; i++) {
        SGCloudLayer * layer = new SGCloudLayer(texture_path);
        _sky->add_cloud_layer(layer);
    }

    _sky->set_texture_path( texture_path );

    // XXX: Should always be true
    eventHandler->setChangeStatsCameraRenderOrder( true );
}

void FGRenderer::setupRoot()
{
    osg::StateSet* stateSet = _root->getOrCreateStateSet();

    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    stateSet->setAttribute(new osg::Depth(osg::Depth::LESS));
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    stateSet->setAttribute(new osg::BlendFunc);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);

    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);

    // this will be set below
    stateSet->setMode(GL_NORMALIZE, osg::StateAttribute::OFF);

    osg::Material* material = new osg::Material;
    stateSet->setAttribute(material);

    stateSet->setTextureAttribute(0, new osg::TexEnv);
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::OFF);

    osg::Hint* hint = new osg::Hint(GL_FOG_HINT, GL_DONT_CARE);
    hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/fog"));
    stateSet->setAttribute(hint);
    hint = new osg::Hint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
    hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/polygon-smooth"));
    stateSet->setAttribute(hint);
    hint = new osg::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/line-smooth"));
    stateSet->setAttribute(hint);
    hint = new osg::Hint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
    hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/point-smooth"));
    stateSet->setAttribute(hint);
    hint = new osg::Hint(GL_PERSPECTIVE_CORRECTION_HINT, GL_DONT_CARE);
    hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/perspective-correction"));
    stateSet->setAttribute(hint);
}

void
FGRenderer::setupView( void )
{
    osgViewer::View* view = globals->get_renderer()->getView();
    osg::initNotifyLevel();

    // The number of polygon-offset "units" to place between layers.  In
    // principle, one is supposed to be enough.  In practice, I find that
    // my hardware/driver requires many more.
    osg::PolygonOffset::setUnitsMultiplier(1);
    osg::PolygonOffset::setFactorMultiplier(1);

    setupRoot();

    // build the sky
    Ephemeris* ephemerisSub = globals->get_subsystem<Ephemeris>();


    // The sun and moon diameters are scaled down numbers of the
    // actual diameters. This was needed to fit both the sun and the
    // moon within the distance to the far clip plane.
    // Moon diameter:    3,476 kilometers
    // Sun diameter: 1,390,000 kilometers
    osg::ref_ptr<simgear::SGReaderWriterOptions> opt;
    opt = simgear::SGReaderWriterOptions::fromPath(globals->get_fg_root());
    opt->setPropertyNode(globals->get_props());
    _sky->build( 80000.0, 80000.0,
                  463.3, 361.8,
                  *ephemerisSub->data(),
                  fgGetNode("/environment", true),
                  opt.get());

    view->getCamera()
        ->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);


    // need to update the light on every frame
    // OSG LightSource objects are rather confusing. OSG only supports
    // the 10 lights specified by OpenGL itself; if more than one
    // LightSource in the scene graph have the same light number, it's
    // indeterminate which values will be used to render geometry that
    // has that light number enabled. Also, adding children to a
    // LightSource is just a shortcut for setting up a state set that
    // has the corresponding OpenGL light enabled: a LightSource will
    // affect geometry anywhere in the scene graph that has its light
    // number enabled in a state set.
    osg::ref_ptr<LightSource> lightSource = new LightSource;
    lightSource->setName("FGLightSource");
    lightSource->getLight()->setDataVariance(Object::DYNAMIC);
    // relative because of CameraView being just a clever transform node
    lightSource->setReferenceFrame(osg::LightSource::RELATIVE_RF);
    lightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    lightSource->setUpdateCallback(new FGLightSourceUpdateCallback);
    _viewerSceneRoot->addChild(lightSource);

    // we need a white diffuse light for the phase of the moon
    osg::ref_ptr<LightSource> sunLight = new osg::LightSource;
    sunLight->setName("sunLightSource");
    sunLight->getLight()->setDataVariance(Object::DYNAMIC);
    sunLight->getLight()->setLightNum(1);
    sunLight->setUpdateCallback(new FGLightSourceUpdateCallback(true));
    sunLight->setReferenceFrame(osg::LightSource::RELATIVE_RF);
    sunLight->setLocalStateSetModes(osg::StateAttribute::ON);

    // Hang a StateSet above the sky subgraph in order to turn off
    // light 0
    Group* skyGroup = _sky->getPreRoot();
    StateSet* skySS = skyGroup->getOrCreateStateSet();
    skySS->setMode(GL_LIGHT0, StateAttribute::OFF);
    sunLight->addChild(skyGroup);

	_root->addChild(sunLight);

    osg::Group* sceneGroup = globals->get_scenery()->get_scene_graph();
    sceneGroup->setName("rendererScene");
    sceneGroup->setNodeMask(~simgear::BACKGROUND_BIT);
    _root->addChild(sceneGroup);

    // setup state-set for main scenery (including models and aircraft)
    osg::StateSet* stateSet = sceneGroup->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

    // enable disable specular highlights.
    // is the place where we might plug in an other fragment shader ...
    osg::LightModel* lightModel = new osg::LightModel;
    lightModel->setUpdateCallback(new FGLightModelUpdateCallback);
    stateSet->setAttribute(lightModel);

    // switch to enable wireframe
    osg::PolygonMode* polygonMode = new osg::PolygonMode;
    polygonMode->setUpdateCallback(new FGWireFrameModeUpdateCallback);
    stateSet->setAttributeAndModes(polygonMode);

    // scene fog handling
    osg::Fog* fog = new osg::Fog;
    fog->setUpdateCallback(new FGFogUpdateCallback);
    stateSet->setAttributeAndModes(fog);
    stateSet->setUpdateCallback(new FGFogEnableUpdateCallback);

    osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
    if (guiCamera) {
        osg::Geode* hudGeode = new osg::Geode;
        hudGeode->addDrawable(new SGHUDDrawable);
        guiCamera->addChild(hudGeode);

#if defined(HAVE_PUI)
        _puiCamera = new flightgear::PUICamera;
        _puiCamera->init(guiCamera, view);
#endif

#if defined(ENABLE_QQ_UI)
        osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(view);
        if (viewer) {
            std::string rootQMLPath = fgGetString("/sim/gui/qml-root-path");
            auto graphicsWindow = dynamic_cast<osgViewer::GraphicsWindow*>(guiCamera->getGraphicsContext());

            if (!rootQMLPath.empty()) {
                _quickDrawable = new QQuickDrawable;
                _quickDrawable->setup(graphicsWindow, viewer);

                _quickDrawable->setSource(QUrl::fromLocalFile(QString::fromStdString(rootQMLPath)));

                osg::Geode* qqGeode = new osg::Geode;
                qqGeode->addDrawable(_quickDrawable);
                guiCamera->addChild(qqGeode);
            }
        }
#endif
        guiCamera->insertChild(0, FGPanelNode::create2DPanelNode());
    }

    osg::Switch* sw = new osg::Switch;
    sw->setName("scenerySwitch");
    sw->setUpdateCallback(new FGScenerySwitchCallback);
    sw->addChild(_root.get());
    _viewerSceneRoot->addChild(sw);
    // The clouds are attached directly to the scene graph root
    // because, in theory, they don't want the same default state set
    // as the rest of the scene. This may not be true in practice.
	_viewerSceneRoot->addChild(_sky->getCloudRoot());
	_viewerSceneRoot->addChild(FGCreateRedoutNode());

    // Attach empty program to the scene root so that shader programs
    // don't leak into state sets (effects) that shouldn't have one.
    stateSet = _viewerSceneRoot->getOrCreateStateSet();
    stateSet->setAttributeAndModes(new osg::Program, osg::StateAttribute::ON);
}

// Update all Visuals (redraws anything graphics related)
void
FGRenderer::update( ) {
    if (!_position_finalized || !_scenery_loaded->getBoolValue())
    {
        _splash_alpha->setDoubleValue(1.0);

        if (!MaximumTextureSize) {
            osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
            if (guiCamera) {
                GraphicsContext *gc = guiCamera->getGraphicsContext();
                osg::GLExtensions* gl2ext = gc->getState()->get<osg::GLExtensions>();
                if (gl2ext) {
                    MaximumTextureSize = gl2ext->maxTextureSize;
                    SGSceneFeatures::instance()->setMaxTextureSize(MaximumTextureSize);
                    SG_LOG(SG_VIEW, SG_INFO, "FGRenderer:: Maximum texture size " << MaximumTextureSize);
                }
            }
        }
        return;
    }

    if (_splash_alpha->getDoubleValue()>0.0)
    {
        // Fade out the splash screen
        const double fade_time = 0.5;
        const double fade_steps_per_sec = 10;
        double delay_time = SGMiscd::min(fade_time/fade_steps_per_sec,
                                         (SGTimeStamp::now() - _splash_time).toSecs());
        _splash_time = SGTimeStamp::now();
        double sAlpha = _splash_alpha->getDoubleValue();
        sAlpha -= SGMiscd::max(0.0,delay_time/fade_time);
        FGScenerySwitchCallback::scenery_enabled = (sAlpha<1.0);

        if (sAlpha <= 0.0) {
            flightgear::addSentryBreadcrumb("splash-screen fade out complete", "info");
        }

        _splash_alpha->setDoubleValue((sAlpha < 0) ? 0.0 : sAlpha);

        syncPausePopupState();
        fgSetBool("/sim/menubar/overlap-hide", false);
    }

    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));

    // update fog params
    double actual_visibility;
    if (_cloud_status->getBoolValue()) {
        actual_visibility = _sky->get_visibility();
    } else {
        actual_visibility = _visibility_m->getDoubleValue();
    }

    // idle_state is now 1000 meaning we've finished all our
    // initializations and are running the main loop, so this will
    // now work without seg faulting the system.

    flightgear::View *current__view = globals->get_current_view();
    // Force update of center dependent values ...
    current__view->set_dirty();

    assert(composite_viewer_enabled != -1);
    std::vector<osg::Camera*> cameras;
    if (composite_viewer) {
        assert(!viewer);
        unsigned n = composite_viewer->getNumViews();
        for (unsigned i=0; i<n; ++i) {
            osgViewer::View* view = composite_viewer->getView(i);
            osg::Camera* camera = view->getCamera();
            cameras.push_back(camera);
        }
    }
    else {
        cameras.push_back(viewer->getCamera());
    }
    for (osg::Camera* camera: cameras) {
        osg::Vec4 clear_color = _altitude_ft->getDoubleValue() < 250000
                              ? toOsg(l->adj_fog_color())
                              // skydome ends at ~262000ft (default rendering)
                              // ~328000 ft (ALS) and would produce a strange
                              // looking greyish space -> black looks much
                              // better :-)
                              : osg::Vec4(0, 0, 0, 1);
        camera->setClearColor(clear_color);

        updateSky();

        // need to call the update visitor once
        getFrameStamp()->setCalendarTime(*globals->get_time_params()->getGmt());
        _updateVisitor->setViewData(current__view->getViewPosition(),
                                    current__view->getViewOrientation());
        //_updateVisitor->setViewData(eye2, center3);
        SGVec3f direction(l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2]);
        _updateVisitor->setLight(direction, l->scene_ambient(),
                                 l->scene_diffuse(), l->scene_specular(),
                                 l->adj_fog_color(),
                                 l->get_sun_angle()*SGD_RADIANS_TO_DEGREES);
        _updateVisitor->setVisibility(actual_visibility);
        simgear::GroundLightManager::instance()->update(_updateVisitor.get());
        osg::Node::NodeMask cullMask = ~simgear::LIGHTS_BITS & ~simgear::PICK_BIT;
        cullMask |= simgear::GroundLightManager::instance()
            ->getLightNodeMask(_updateVisitor.get());
        if (_panel_hotspots->getBoolValue())
            cullMask |= simgear::PICK_BIT;
        camera->setCullMask(cullMask);
        camera->setCullMaskLeft(cullMask);
        camera->setCullMaskRight(cullMask);
    }
}

void
FGRenderer::updateSky()
{
    // update fog params if visibility has changed
    double visibility_meters = _visibility_m->getDoubleValue();
    _sky->set_visibility(visibility_meters);

    double altitude_m = _altitude_ft->getDoubleValue() * SG_FEET_TO_METER;
    _sky->modify_vis( altitude_m, 0.0 /* time factor, now unused */);

    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));

    // The sun and moon distances are scaled down versions
    // of the actual distance to get both the moon and the sun
    // within the range of the far clip plane.
    // Moon distance:    384,467 kilometers
    // Sun distance: 150,000,000 kilometers

    double sun_horiz_eff, moon_horiz_eff;
    if (_horizon_effect->getBoolValue()) {
        sun_horiz_eff
        = 0.67 + pow(osg::clampAbove(0.5 + cos(l->get_sun_angle()),
                                     0.0),
                     0.33) / 3.0;
        moon_horiz_eff
        = 0.67 + pow(osg::clampAbove(0.5 + cos(l->get_moon_angle()),
                                     0.0),
                     0.33)/3.0;
    } else {
        sun_horiz_eff = moon_horiz_eff = 1.0;
    }



    SGSkyState sstate;
    sstate.pos       = globals->get_current_view()->getViewPosition();
    sstate.pos_geod  = globals->get_current_view()->getPosition();
    sstate.ori       = globals->get_current_view()->getViewOrientation();
    sstate.spin      = l->get_sun_rotation();
    sstate.gst       = globals->get_time_params()->getGst();
    sstate.sun_dist  = 50000.0 * sun_horiz_eff;
    sstate.moon_dist = 40000.0 * moon_horiz_eff;
    sstate.sun_angle = l->get_sun_angle();

    SGSkyColor scolor;
    scolor.sky_color   = SGVec3f(l->sky_color().data());
    scolor.adj_sky_color = SGVec3f(l->adj_sky_color().data());
    scolor.fog_color   = SGVec3f(l->adj_fog_color().data());
    scolor.cloud_color = SGVec3f(l->cloud_color().data());
    scolor.sun_angle   = l->get_sun_angle();
    scolor.moon_angle  = l->get_moon_angle();

    Ephemeris* ephemerisSub = globals->get_subsystem<Ephemeris>();
    double delta_time_sec = _sim_delta_sec->getDoubleValue();
    _sky->reposition( sstate, *ephemerisSub->data(), delta_time_sec );
    _sky->repaint( scolor, *ephemerisSub->data() );
}

void
FGRenderer::resize( int width, int height )
{
    int curWidth = _xsize->getIntValue(),
        curHeight = _ysize->getIntValue();
    SG_LOG(SG_VIEW, SG_DEBUG, "FGRenderer::resize: new size " << width << " x " << height);
    if ((curHeight != height) || (curWidth != width)) {
    // must guard setting these, or PLIB-PUI fails with too many live interfaces
        _xsize->setIntValue(width);
        _ysize->setIntValue(height);
    }

    // update splash node if present
    _splash->resize(width, height);
#if defined(ENABLE_QQ_UI)
    if (_quickDrawable) {
        _quickDrawable->resize(width, height);
    }
#endif
}

typedef osgUtil::LineSegmentIntersector::Intersection Intersection;
SGVec2d uvFromIntersection(const Intersection& hit)
{
  // Taken from http://trac.openscenegraph.org/projects/osg/browser/OpenSceneGraph/trunk/examples/osgmovie/osgmovie.cpp

  osg::Drawable* drawable = hit.drawable.get();
  osg::Geometry* geometry = drawable ? drawable->asGeometry() : 0;
  osg::Vec3Array* vertices =
    geometry ? dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray()) : 0;

  if( !vertices )
  {
    SG_LOG(SG_INPUT, SG_WARN, "Unable to get vertices for intersection.");
    return SGVec2d(-9999,-9999);
  }

  // get the vertex indices.
  const Intersection::IndexList& indices = hit.indexList;
  const Intersection::RatioList& ratios = hit.ratioList;

  if( indices.size() != 3 || ratios.size() != 3 )
  {
    SG_LOG( SG_INPUT,
            SG_WARN,
            "Intersection has insufficient indices to work with." );
    return SGVec2d(-9999,-9999);
  }

  unsigned int i1 = indices[0];
  unsigned int i2 = indices[1];
  unsigned int i3 = indices[2];

  float r1 = ratios[0];
  float r2 = ratios[1];
  float r3 = ratios[2];

  osg::Array* texcoords =
    (geometry->getNumTexCoordArrays() > 0) ? geometry->getTexCoordArray(0) : 0;
  osg::Vec2Array* texcoords_Vec2Array =
    dynamic_cast<osg::Vec2Array*>(texcoords);

  if( !texcoords_Vec2Array )
  {
    SG_LOG(SG_INPUT, SG_WARN, "Unable to get texcoords for intersection.");
    return SGVec2d(-9999,-9999);
  }

  // we have tex coord array so now we can compute the final tex coord at the
  // point of intersection.
  osg::Vec2 tc1 = (*texcoords_Vec2Array)[i1];
  osg::Vec2 tc2 = (*texcoords_Vec2Array)[i2];
  osg::Vec2 tc3 = (*texcoords_Vec2Array)[i3];

  return toSG( osg::Vec2d(tc1 * r1 + tc2 * r2 + tc3 * r3) );
}

PickList FGRenderer::pick(const osg::Vec2& windowPos)
{
    PickList result;

    typedef osgUtil::LineSegmentIntersector::Intersections Intersections;
    Intersections intersections;

    if (!computeIntersections(CameraGroup::getDefault(), windowPos, intersections))
        return result;
    for (Intersections::iterator hit = intersections.begin(),
             e = intersections.end();
         hit != e;
         ++hit) {
        const osg::NodePath& np = hit->nodePath;
        osg::NodePath::const_reverse_iterator npi;

        for (npi = np.rbegin(); npi != np.rend(); ++npi) {
            SGSceneUserData* ud = SGSceneUserData::getSceneUserData(*npi);
            if (!ud || (ud->getNumPickCallbacks() == 0))
                continue;

            for (unsigned i = 0; i < ud->getNumPickCallbacks(); ++i) {
                SGPickCallback* pickCallback = ud->getPickCallback(i);
                if (!pickCallback)
                    continue;
                SGSceneryPick sceneryPick;
                sceneryPick.info.local = toSG(hit->getLocalIntersectPoint());
                sceneryPick.info.wgs84 = toSG(hit->getWorldIntersectPoint());

                if( pickCallback->needsUV() )
                  sceneryPick.info.uv = uvFromIntersection(*hit);

                sceneryPick.callback = pickCallback;
                result.push_back(sceneryPick);
            } // of installed pick callbacks iteration
        } // of reverse node path walk
    }

    return result;
}

osgViewer::ViewerBase* FGRenderer::getViewerBase()
{
    if (composite_viewer) {
        return composite_viewer.get();
    }
    else {
        return viewer.get();
    }
}

osgViewer::View* FGRenderer::getView()
{
    /* Would like to assert that FGRenderer::init() has always been called
    before we are called, with:
        assert(composite_viewer_enabled != -1);
    But this fails if user specifies -h, when we are called by
    FGGlobals::~FGGlobals().
    */
    if (composite_viewer) {
        assert(composite_viewer->getNumViews());
        return composite_viewer->getView(0);
    }
    else {
        return viewer.get();
    }
}

const osgViewer::View* FGRenderer::getView() const
{
    FGRenderer* this_ = const_cast<FGRenderer*>(this);
    return this_->getView();
}

void
FGRenderer::setView(osgViewer::View* view)
{
    assert(composite_viewer_enabled != -1);
    if (composite_viewer) {
        if (composite_viewer->getNumViews() == 0) {
            SG_LOG(SG_VIEW, SG_ALERT, "adding view to composite_viewer.");
            composite_viewer->stopThreading();
            composite_viewer->addView(view);
            composite_viewer->startThreading();
        }
    }
    else {
        osgViewer::Viewer* viewer_ = dynamic_cast<osgViewer::Viewer*>(view);
        assert(viewer_);
        viewer = viewer_;
    }
}

osg::FrameStamp*
FGRenderer::getFrameStamp()
{
    assert(composite_viewer_enabled != -1);
    if (composite_viewer) {
        assert(!viewer);
        return composite_viewer->getFrameStamp();
    }
    else {
        assert(viewer);
        return viewer->getFrameStamp();
    }
}

void
FGRenderer::setEventHandler(FGEventHandler* eventHandler_)
{
    eventHandler = eventHandler_;
}

void
FGRenderer::addCamera(osg::Camera* camera, bool useSceneData)
{
    _viewerSceneRoot->addChild(camera);
}

void
FGRenderer::removeCamera(osg::Camera* camera)
{
    _viewerSceneRoot->removeChild(camera);
}

void
FGRenderer::setPlanes( double zNear, double zFar )
{
//	_planes->set( osg::Vec3f( - zFar, - zFar * zNear, zFar - zNear ) );
}

bool
fgDumpSceneGraphToFile(const char* filename)
{
    osgViewer::View* view = globals->get_renderer()->getView();
    return osgDB::writeNodeFile(*view->getSceneData(), filename);
}

bool
fgDumpTerrainBranchToFile(const char* filename)
{
    return osgDB::writeNodeFile( *globals->get_scenery()->get_terrain_branch(),
                                 filename );
}

// For debugging
bool
fgDumpNodeToFile(osg::Node* node, const char* filename)
{
    return osgDB::writeNodeFile(*node, filename);
}

namespace flightgear
{
using namespace osg;

class VisibleSceneInfoVistor : public NodeVisitor, CullStack
{
public:
    VisibleSceneInfoVistor()
        : NodeVisitor(CULL_VISITOR, TRAVERSE_ACTIVE_CHILDREN)
    {
        setCullingMode(CullSettings::SMALL_FEATURE_CULLING
                       | CullSettings::VIEW_FRUSTUM_CULLING);
        setComputeNearFarMode(CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    }

    VisibleSceneInfoVistor(const VisibleSceneInfoVistor& rhs)
    {
    }

    META_NodeVisitor("flightgear","VisibleSceneInfoVistor")

    typedef std::map<const std::string,int> InfoMap;

    void getNodeInfo(Node* node)
    {
        const char* typeName = typeid(*node).name();
        classInfo[typeName]++;
        const std::string& nodeName = node->getName();
        if (!nodeName.empty())
            nodeInfo[nodeName]++;
    }

    void dumpInfo()
    {
        using namespace std;
        typedef vector<InfoMap::iterator> FreqVector;
        cout << "class info:\n";
        FreqVector classes;
        for (InfoMap::iterator itr = classInfo.begin(), end = classInfo.end();
             itr != end;
             ++itr)
            classes.push_back(itr);
        sort(classes.begin(), classes.end(), freqComp);
        for (FreqVector::iterator itr = classes.begin(), end = classes.end();
             itr != end;
             ++itr) {
            cout << (*itr)->first << " " << (*itr)->second << "\n";
        }
        cout << "\nnode info:\n";
        FreqVector nodes;
        for (InfoMap::iterator itr = nodeInfo.begin(), end = nodeInfo.end();
             itr != end;
             ++itr)
            nodes.push_back(itr);

        sort (nodes.begin(), nodes.end(), freqComp);
        for (FreqVector::iterator itr = nodes.begin(), end = nodes.end();
             itr != end;
             ++itr) {
            cout << (*itr)->first << " " << (*itr)->second << "\n";
        }
        cout << endl;
    }

    void doTraversal(Camera* camera, Node* root, Viewport* viewport)
    {
        ref_ptr<RefMatrix> projection
            = createOrReuseMatrix(camera->getProjectionMatrix());
        ref_ptr<RefMatrix> mv = createOrReuseMatrix(camera->getViewMatrix());
        if (!viewport)
            viewport = camera->getViewport();
        if (viewport)
            pushViewport(viewport);
        pushProjectionMatrix(projection.get());
        pushModelViewMatrix(mv.get(), Transform::ABSOLUTE_RF);
        root->accept(*this);
        popModelViewMatrix();
        popProjectionMatrix();
        if (viewport)
            popViewport();
        dumpInfo();
    }

    void apply(Node& node)
    {
        if (isCulled(node))
            return;
        pushCurrentMask();
        getNodeInfo(&node);
        traverse(node);
        popCurrentMask();
    }
    void apply(Group& node)
    {
        if (isCulled(node))
            return;
        pushCurrentMask();
        getNodeInfo(&node);
        traverse(node);
        popCurrentMask();
    }

    void apply(Transform& node)
    {
        if (isCulled(node))
            return;
        pushCurrentMask();
        ref_ptr<RefMatrix> matrix = createOrReuseMatrix(*getModelViewMatrix());
        node.computeLocalToWorldMatrix(*matrix,this);
        pushModelViewMatrix(matrix.get(), node.getReferenceFrame());
        getNodeInfo(&node);
        traverse(node);
        popModelViewMatrix();
        popCurrentMask();
    }

    void apply(Camera& camera)
    {
        // Save current cull settings
        CullSettings saved_cull_settings(*this);

        // set cull settings from this Camera
        setCullSettings(camera);
        // inherit the settings from above
        inheritCullSettings(saved_cull_settings, camera.getInheritanceMask());

        // set the cull mask.
        unsigned int savedTraversalMask = getTraversalMask();
        bool mustSetCullMask = (camera.getInheritanceMask()
                                & osg::CullSettings::CULL_MASK) == 0;
        if (mustSetCullMask)
            setTraversalMask(camera.getCullMask());

        osg::RefMatrix* projection = 0;
        osg::RefMatrix* modelview = 0;

        if (camera.getReferenceFrame()==osg::Transform::RELATIVE_RF) {
            if (camera.getTransformOrder()==osg::Camera::POST_MULTIPLY) {
                projection = createOrReuseMatrix(*getProjectionMatrix()
                                                 *camera.getProjectionMatrix());
                modelview = createOrReuseMatrix(*getModelViewMatrix()
                                                * camera.getViewMatrix());
            }
            else {              // pre multiply
                projection = createOrReuseMatrix(camera.getProjectionMatrix()
                                                 * (*getProjectionMatrix()));
                modelview = createOrReuseMatrix(camera.getViewMatrix()
                                                * (*getModelViewMatrix()));
            }
        } else {
            // an absolute reference frame
            projection = createOrReuseMatrix(camera.getProjectionMatrix());
            modelview = createOrReuseMatrix(camera.getViewMatrix());
        }
        if (camera.getViewport())
            pushViewport(camera.getViewport());

        pushProjectionMatrix(projection);
        pushModelViewMatrix(modelview, camera.getReferenceFrame());

        traverse(camera);

        // restore the previous model view matrix.
        popModelViewMatrix();

        // restore the previous model view matrix.
        popProjectionMatrix();

        if (camera.getViewport()) popViewport();

        // restore the previous traversal mask settings
        if (mustSetCullMask)
            setTraversalMask(savedTraversalMask);

        // restore the previous cull settings
        setCullSettings(saved_cull_settings);
    }

protected:
    // sort in reverse
    static bool freqComp(const InfoMap::iterator& lhs, const InfoMap::iterator& rhs)
    {
        return lhs->second > rhs->second;
    }
    InfoMap classInfo;
    InfoMap nodeInfo;
};

bool printVisibleSceneInfo(FGRenderer* renderer)
{
    osgViewer::View* view = renderer->getView();
    VisibleSceneInfoVistor vsv;
    Viewport* vp = 0;
    if (!view->getCamera()->getViewport() && view->getNumSlaves() > 0) {
        const osg::View::Slave& slave = view->getSlave(0);
        vp = slave._camera->getViewport();
    }
    vsv.doTraversal(view->getCamera(), view->getSceneData(), vp);
    return true;
}

}
// end of renderer.cxx
