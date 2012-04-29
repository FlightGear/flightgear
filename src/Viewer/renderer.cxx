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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

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

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/tgdb/GroundLightManager.hxx>
#include <simgear/scene/tgdb/pt_lights.hxx>
#include <simgear/structure/OSGUtils.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/sg_random.h>
#ifdef FG_JPEG_SERVER
#include <simgear/screen/jpgfactory.hxx>
#endif

#include <Time/light.hxx>
#include <Time/light.hxx>
#include <Cockpit/panel.hxx>

#include <Model/panelnode.hxx>
#include <Model/modelmgr.hxx>
#include <Model/acmodel.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/redout.hxx>
#include <GUI/new_gui.hxx>
#include <Instrumentation/HUD/HUD.hxx>
#include <Environment/precipitation_mgr.hxx>
#include <Environment/environment_mgr.hxx>

//#include <Main/main.hxx>
#include "viewer.hxx"
#include "viewmgr.hxx"
#include "splash.hxx"
#include "renderer.hxx"
#include "CameraGroup.hxx"
#include "FGEventHandler.hxx"

using namespace osg;
using namespace simgear;
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


class SGPuDrawable : public osg::Drawable {
public:
  SGPuDrawable()
  {
    // Dynamic stuff, do not store geometry
    setUseDisplayList(false);
    setDataVariance(Object::DYNAMIC);

    osg::StateSet* stateSet = getOrCreateStateSet();
    stateSet->setRenderBinDetails(1001, "RenderBin");
    // speed optimization?
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    // We can do translucent menus, so why not. :-)
    stateSet->setAttribute(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::OFF);

    stateSet->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::MODULATE));

    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
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

    puDisplay();

    glPopClientAttrib();
    glPopAttrib();
  }

  virtual osg::Object* cloneType() const { return new SGPuDrawable; }
  virtual osg::Object* clone(const osg::CopyOp&) const { return new SGPuDrawable; }
  
private:
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
      
    HUD *hud = static_cast<HUD*>(globals->get_subsystem("hud"));
    hud->draw(state);

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

static osg::ref_ptr<osg::FrameStamp> mFrameStamp = new osg::FrameStamp;
static osg::ref_ptr<SGUpdateVisitor> mUpdateVisitor= new SGUpdateVisitor;

static osg::ref_ptr<osg::Group> mRealRoot = new osg::Group;
static osg::ref_ptr<osg::Group> mDeferredRealRoot = new osg::Group;

static osg::ref_ptr<osg::Group> mRoot = new osg::Group;

static osg::ref_ptr<osg::Switch> panelSwitch;
                                    
                                    
// update callback for the switch node controlling the 2D panel
class FGPanelSwitchCallback : public osg::NodeCallback {
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        assert(dynamic_cast<osg::Switch*>(node));
        osg::Switch* sw = static_cast<osg::Switch*>(node);
        
        bool enabled = fgPanelVisible();
        sw->setValue(0, enabled);
        if (!enabled)
            return;
        traverse(node, nv);
    }
};

#ifdef FG_JPEG_SERVER
static void updateRenderer()
{
    globals->get_renderer()->update();
}
#endif

FGRenderer::FGRenderer() :
    _sky(NULL),
    _ambientFactor( new osg::Uniform( "fg_SunAmbientColor", osg::Vec4f() ) ),
    _sunDiffuse( new osg::Uniform( "fg_SunDiffuseColor", osg::Vec4f() ) ),
    _sunSpecular( new osg::Uniform( "fg_SunSpecularColor", osg::Vec4f() ) ),
    _sunDirection( new osg::Uniform( "fg_SunDirection", osg::Vec3f() ) ),
    _planes( new osg::Uniform( "fg_Planes", osg::Vec3f() ) ),
    _fogColor( new osg::Uniform( "fg_FogColor", osg::Vec4f(1.0, 1.0, 1.0, 1.0) ) ),
    _fogDensity( new osg::Uniform( "fg_FogDensity", 0.0001f ) ),
    _shadowNumber( new osg::Uniform( "fg_ShadowNumber", (int)4 ) ),
    _shadowDistances( new osg::Uniform( "fg_ShadowDistances", osg::Vec4f(5.0, 50.0, 500.0, 5000.0 ) ) )
{
#ifdef FG_JPEG_SERVER
   jpgRenderFrame = updateRenderer;
#endif
   eventHandler = new FGEventHandler;

   _numCascades = 4;
   _cascadeFar[0] = 5.f;
   _cascadeFar[1] = 50.f;
   _cascadeFar[2] = 500.f;
   _cascadeFar[3] = 5000.f;
}

FGRenderer::~FGRenderer()
{
#ifdef FG_JPEG_SERVER
   jpgRenderFrame = NULL;
#endif
    delete _sky;
}

// Initialize various GL/view parameters
// XXX This should be called "preinit" or something, as it initializes
// critical parts of the scene graph in addition to the splash screen.
void
FGRenderer::splashinit( void ) {
    osgViewer::Viewer* viewer = getViewer();
    mRealRoot = dynamic_cast<osg::Group*>(viewer->getSceneData());
    ref_ptr<Node> splashNode = fgCreateSplashNode();
    if (_classicalRenderer) {
        mRealRoot->addChild(splashNode.get());
    } else {
        for (   CameraGroup::CameraIterator ii = CameraGroup::getDefault()->camerasBegin();
                ii != CameraGroup::getDefault()->camerasEnd();
                ++ii )
        {
            CameraInfo* info = ii->get();
            Camera* camera = info->getCamera(DISPLAY_CAMERA);
            if (camera == 0) continue;

            camera->addChild(splashNode.get());
        }
    }
    mFrameStamp = viewer->getFrameStamp();
    // Scene doesn't seem to pass the frame stamp to the update
    // visitor automatically.
    mUpdateVisitor->setFrameStamp(mFrameStamp.get());
    viewer->setUpdateVisitor(mUpdateVisitor.get());
    fgSetDouble("/sim/startup/splash-alpha", 1.0);
}

class ShadowMapSizeListener : public SGPropertyChangeListener {
public:
    virtual void valueChanged(SGPropertyNode* node) {
        globals->get_renderer()->updateShadowMapSize(node->getIntValue());
    }
};

class ShadowEnabledListener : public SGPropertyChangeListener {
public:
    virtual void valueChanged(SGPropertyNode* node) {
        globals->get_renderer()->enableShadows(node->getBoolValue());
    }
};

class ShadowNumListener : public SGPropertyChangeListener {
public:
    virtual void valueChanged(SGPropertyNode* node) {
        globals->get_renderer()->updateCascadeNumber(node->getIntValue());
    }
};

class ShadowRangeListener : public SGPropertyChangeListener {
public:
    virtual void valueChanged(SGPropertyNode* node) {
        globals->get_renderer()->updateCascadeFar(node->getIndex(), node->getFloatValue());
    }
};

void
FGRenderer::init( void )
{
    _classicalRenderer = !fgGetBool("/sim/rendering/rembrandt", false);
    _shadowMapSize    = fgGetInt( "/sim/rendering/shadows/map-size", 4096 );
    fgAddChangeListener( new ShadowMapSizeListener, "/sim/rendering/shadows/map-size" );
    fgAddChangeListener( new ShadowEnabledListener, "/sim/rendering/shadows/enabled" );
    ShadowRangeListener* srl = new ShadowRangeListener;
    fgAddChangeListener(srl, "/sim/rendering/shadows/cascade-far-m[0]");
    fgAddChangeListener(srl, "/sim/rendering/shadows/cascade-far-m[1]");
    fgAddChangeListener(srl, "/sim/rendering/shadows/cascade-far-m[2]");
    fgAddChangeListener(srl, "/sim/rendering/shadows/cascade-far-m[3]");
    fgAddChangeListener(new ShadowNumListener, "/sim/rendering/shadows/num-cascades");
    _numCascades = fgGetInt("/sim/rendering/shadows/num-cascades", 4);
    _cascadeFar[0] = fgGetFloat("/sim/rendering/shadows/cascade-far-m[0]", 5.0f);
    _cascadeFar[1] = fgGetFloat("/sim/rendering/shadows/cascade-far-m[1]", 50.0f);
    _cascadeFar[2] = fgGetFloat("/sim/rendering/shadows/cascade-far-m[2]", 500.0f);
    _cascadeFar[3] = fgGetFloat("/sim/rendering/shadows/cascade-far-m[3]", 5000.0f);
    updateCascadeNumber(_numCascades);
    updateCascadeFar(0, _cascadeFar[0]);
    updateCascadeFar(1, _cascadeFar[1]);
    updateCascadeFar(2, _cascadeFar[2]);
    updateCascadeFar(3, _cascadeFar[3]);

    _scenery_loaded   = fgGetNode("/sim/sceneryloaded", true);
    _scenery_override = fgGetNode("/sim/sceneryloaded-override", true);
    _panel_hotspots   = fgGetNode("/sim/panel-hotspots", true);
    _virtual_cockpit  = fgGetNode("/sim/virtual-cockpit", true);

    _sim_delta_sec = fgGetNode("/sim/time/delta-sec", true);

    _xsize         = fgGetNode("/sim/startup/xsize", true);
    _ysize         = fgGetNode("/sim/startup/ysize", true);
    _splash_alpha  = fgGetNode("/sim/startup/splash-alpha", true);

    _skyblend             = fgGetNode("/sim/rendering/skyblend", true);
    _point_sprites        = fgGetNode("/sim/rendering/point-sprites", true);
    _enhanced_lighting    = fgGetNode("/sim/rendering/enhanced-lighting", true);
    _distance_attenuation = fgGetNode("/sim/rendering/distance-attenuation", true);
    _horizon_effect       = fgGetNode("/sim/rendering/horizon-effect", true);
    _textures             = fgGetNode("/sim/rendering/textures", true);

    _altitude_ft = fgGetNode("/position/altitude-ft", true);

    _cloud_status = fgGetNode("/environment/clouds/status", true);
    _visibility_m = fgGetNode("/environment/visibility-m", true);
    
    bool use_point_sprites = _point_sprites->getBoolValue();
    bool enhanced_lighting = _enhanced_lighting->getBoolValue();
    bool distance_attenuation = _distance_attenuation->getBoolValue();

    SGConfigureDirectionalLights( use_point_sprites, enhanced_lighting,
                                  distance_attenuation );

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
    
// create sky, but can't build until setupView, since we depend
// on other subsystems to be inited, eg Ephemeris    
    _sky = new SGSky;
    
    SGPath texture_path(globals->get_fg_root());
    texture_path.append("Textures");
    texture_path.append("Sky");
    for (int i = 0; i < FGEnvironmentMgr::MAX_CLOUD_LAYERS; i++) {
        SGCloudLayer * layer = new SGCloudLayer(texture_path.str());
        _sky->add_cloud_layer(layer);
    }
    
    _sky->texture_path( texture_path.str() );

    if (!_classicalRenderer) {
        eventHandler->setChangeStatsCameraRenderOrder( true );
    }
}

void installCullVisitor(Camera* camera)
{
    osgViewer::Renderer* renderer
        = static_cast<osgViewer::Renderer*>(camera->getRenderer());
    for (int i = 0; i < 2; ++i) {
        osgUtil::SceneView* sceneView = renderer->getSceneView(i);
#if SG_OSG_VERSION_LESS_THAN(3,0,0)
        sceneView->setCullVisitor(new simgear::EffectCullVisitor);
#else
        osg::ref_ptr<osgUtil::CullVisitor::Identifier> identifier;
        identifier = sceneView->getCullVisitor()->getIdentifier();
        sceneView->setCullVisitor(new simgear::EffectCullVisitor);
        sceneView->getCullVisitor()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorLeft()->getIdentifier();
        sceneView->setCullVisitorLeft(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorLeft()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorRight()->getIdentifier();
        sceneView->setCullVisitorRight(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorRight()->setIdentifier(identifier.get());
#endif
    }
}

flightgear::CameraInfo*
FGRenderer::buildRenderingPipeline(flightgear::CameraGroup* cgroup, unsigned flags, Camera* camera,
                                   const Matrix& view,
                                   const Matrix& projection,
								   osg::GraphicsContext* gc,
                                   bool useMasterSceneData)
{
	flightgear::CameraInfo* info = 0;
	if (!_classicalRenderer && (flags & (CameraGroup::GUI | CameraGroup::ORTHO)) == 0)
		info = buildDeferredPipeline( cgroup, flags, camera, view, projection, gc );

	if (info) {
		return info;
	} else {
		if ((flags & (CameraGroup::GUI | CameraGroup::ORTHO)) == 0)
			_classicalRenderer = true;
		return buildClassicalPipeline( cgroup, flags, camera, view, projection, useMasterSceneData );
	}
}

flightgear::CameraInfo*
FGRenderer::buildClassicalPipeline(flightgear::CameraGroup* cgroup, unsigned flags, osg::Camera* camera,
                                const osg::Matrix& view,
                                const osg::Matrix& projection,
                                bool useMasterSceneData)
{
    CameraInfo* info = new CameraInfo(flags);
    // The camera group will always update the camera
    camera->setReferenceFrame(Transform::ABSOLUTE_RF);

    Camera* farCamera = 0;
    if ((flags & (CameraGroup::GUI | CameraGroup::ORTHO)) == 0) {
        farCamera = new Camera;
        farCamera->setAllowEventFocus(camera->getAllowEventFocus());
        farCamera->setGraphicsContext(camera->getGraphicsContext());
        farCamera->setCullingMode(camera->getCullingMode());
        farCamera->setInheritanceMask(camera->getInheritanceMask());
        farCamera->setReferenceFrame(Transform::ABSOLUTE_RF);
        // Each camera's viewport is written when the window is
        // resized; if the the viewport isn't copied here, it gets updated
        // twice and ends up with the wrong value.
        farCamera->setViewport(simgear::clone(camera->getViewport()));
        farCamera->setDrawBuffer(camera->getDrawBuffer());
        farCamera->setReadBuffer(camera->getReadBuffer());
        farCamera->setRenderTargetImplementation(
            camera->getRenderTargetImplementation());
        const Camera::BufferAttachmentMap& bufferMap
            = camera->getBufferAttachmentMap();
        if (bufferMap.count(Camera::COLOR_BUFFER) != 0) {
            farCamera->attach(
                Camera::COLOR_BUFFER,
                bufferMap.find(Camera::COLOR_BUFFER)->second._texture.get());
        }
        cgroup->getViewer()->addSlave(farCamera, projection, view, useMasterSceneData);
        installCullVisitor(farCamera);
		int farSlaveIndex = cgroup->getViewer()->getNumSlaves() - 1;
		info->addCamera( FAR_CAMERA, farCamera, farSlaveIndex );
        farCamera->setRenderOrder(Camera::POST_RENDER, farSlaveIndex);
        camera->setCullMask(camera->getCullMask() & ~simgear::BACKGROUND_BIT);
        camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    }
    cgroup->getViewer()->addSlave(camera, projection, view, useMasterSceneData);
    installCullVisitor(camera);
    int slaveIndex = cgroup->getViewer()->getNumSlaves() - 1;
	info->addCamera( MAIN_CAMERA, camera, slaveIndex );
    camera->setRenderOrder(Camera::POST_RENDER, slaveIndex);
    cgroup->addCamera(info);
    return info;
}

class FGDeferredRenderingCameraCullCallback : public osg::NodeCallback {
public:
    FGDeferredRenderingCameraCullCallback( flightgear::CameraKind k, CameraInfo* i ) : kind( k ), info( i ) {}
    virtual void operator()( osg::Node *n, osg::NodeVisitor *nv) {
    simgear::EffectCullVisitor* cv = dynamic_cast<simgear::EffectCullVisitor*>(nv);
    osg::Camera* camera = static_cast<osg::Camera*>(n);

    cv->clearBufferList();
    cv->addBuffer(simgear::Effect::DEPTH_BUFFER, info->getBuffer( flightgear::RenderBufferInfo::DEPTH_BUFFER ) );
    cv->addBuffer(simgear::Effect::NORMAL_BUFFER, info->getBuffer( flightgear::RenderBufferInfo::NORMAL_BUFFER ) );
    cv->addBuffer(simgear::Effect::DIFFUSE_BUFFER, info->getBuffer( flightgear::RenderBufferInfo::DIFFUSE_BUFFER ) );
    cv->addBuffer(simgear::Effect::SPEC_EMIS_BUFFER, info->getBuffer( flightgear::RenderBufferInfo::SPEC_EMIS_BUFFER ) );
    cv->addBuffer(simgear::Effect::LIGHTING_BUFFER, info->getBuffer( flightgear::RenderBufferInfo::LIGHTING_BUFFER ) );
    cv->addBuffer(simgear::Effect::SHADOW_BUFFER, info->getBuffer( flightgear::RenderBufferInfo::SHADOW_BUFFER ) );
    // cv->addBuffer(simgear::Effect::AO_BUFFER, info->gBuffer->aoBuffer[2]);

    if ( !info->getRenderStageInfo(kind).fullscreen )
        info->setMatrices( camera );

    cv->traverse( *camera );

    if ( kind == flightgear::GEOMETRY_CAMERA ) {
        // Save transparent bins to render later
        osgUtil::RenderStage* renderStage = cv->getRenderStage();
        osgUtil::RenderBin::RenderBinList& rbl = renderStage->getRenderBinList();
        for (osgUtil::RenderBin::RenderBinList::iterator rbi = rbl.begin(); rbi != rbl.end(); ) {
            if (rbi->second->getSortMode() == osgUtil::RenderBin::SORT_BACK_TO_FRONT) {
                info->savedTransparentBins.insert( std::make_pair( rbi->first, rbi->second ) );
                rbl.erase( rbi++ );
            } else {
                ++rbi;
            }
        }
    } else if ( kind == flightgear::LIGHTING_CAMERA ) {
        osg::ref_ptr<osg::Camera> mainShadowCamera = info->getCamera( SHADOW_CAMERA );
            if (mainShadowCamera.valid()) {
                osg::Switch* grp = mainShadowCamera->getChild(0)->asSwitch();
                for (int i = 0; i < 4; ++i ) {
                    if (!grp->getValue(i))
                        continue;
                    osg::TexGen* shadowTexGen = info->shadowTexGen[i];
                    shadowTexGen->setMode(osg::TexGen::EYE_LINEAR);

                    osg::Camera* cascadeCam = static_cast<osg::Camera*>( grp->getChild(i) );
                    // compute the matrix which takes a vertex from view coords into tex coords
                    shadowTexGen->setPlanesFromMatrix(  cascadeCam->getProjectionMatrix() *
                                                        osg::Matrix::translate(1.0,1.0,1.0) *
                                                        osg::Matrix::scale(0.5f,0.5f,0.5f) );

                    osg::RefMatrix * refMatrix = new osg::RefMatrix( cascadeCam->getInverseViewMatrix() * *cv->getModelViewMatrix() );

                    cv->getRenderStage()->getPositionalStateContainer()->addPositionedTextureAttribute( i+1, refMatrix, shadowTexGen );
                }
            }
            // Render saved transparent render bins
            osgUtil::RenderStage* renderStage = cv->getRenderStage();
            osgUtil::RenderBin::RenderBinList& rbl = renderStage->getRenderBinList();
            for (osgUtil::RenderBin::RenderBinList::iterator rbi = info->savedTransparentBins.begin(); rbi != info->savedTransparentBins.end(); ++rbi ){
                rbl.insert( std::make_pair( rbi->first + 10000, rbi->second ) );
            }
            info->savedTransparentBins.clear();
        }
    }

private:
    flightgear::CameraKind kind;
    CameraInfo* info;
};

osg::Texture2D* buildDeferredBuffer(GLint internalFormat, GLenum sourceFormat, GLenum sourceType, osg::Texture::WrapMode wrapMode, bool shadowComparison = false)
{
    osg::Texture2D* tex = new osg::Texture2D;
    tex->setResizeNonPowerOfTwoHint( false );
    tex->setInternalFormat( internalFormat );
    tex->setShadowComparison(shadowComparison);
    if (shadowComparison) {
        tex->setShadowTextureMode(osg::Texture2D::LUMINANCE);
        tex->setBorderColor(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
    }
    tex->setSourceFormat(sourceFormat);
    tex->setSourceType(sourceType);
    tex->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    tex->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
    tex->setWrap( osg::Texture::WRAP_S, wrapMode );
    tex->setWrap( osg::Texture::WRAP_T, wrapMode );
	return tex;
}

void buildDeferredBuffers( flightgear::CameraInfo* info, int shadowMapSize, bool normal16 )
{
    info->addBuffer(flightgear::RenderBufferInfo::DEPTH_BUFFER, buildDeferredBuffer( GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, osg::Texture::CLAMP_TO_BORDER) );
    if (false)
        info->addBuffer(flightgear::RenderBufferInfo::NORMAL_BUFFER, buildDeferredBuffer( 0x822C /*GL_RG16*/, 0x8227 /*GL_RG*/, GL_UNSIGNED_SHORT, osg::Texture::CLAMP_TO_BORDER) );
    else
        info->addBuffer(flightgear::RenderBufferInfo::NORMAL_BUFFER, buildDeferredBuffer( GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, osg::Texture::CLAMP_TO_BORDER) );

    info->addBuffer(flightgear::RenderBufferInfo::DIFFUSE_BUFFER, buildDeferredBuffer( GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, osg::Texture::CLAMP_TO_BORDER) );
    info->addBuffer(flightgear::RenderBufferInfo::SPEC_EMIS_BUFFER, buildDeferredBuffer( GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, osg::Texture::CLAMP_TO_BORDER) );
    info->addBuffer(flightgear::RenderBufferInfo::LIGHTING_BUFFER, buildDeferredBuffer( GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, osg::Texture::CLAMP_TO_BORDER) );
    info->addBuffer(flightgear::RenderBufferInfo::SHADOW_BUFFER, buildDeferredBuffer( GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, osg::Texture::CLAMP_TO_BORDER, true), 0.0f );
    info->getBuffer(RenderBufferInfo::SHADOW_BUFFER)->setTextureSize(shadowMapSize,shadowMapSize);
}

void attachBufferToCamera( flightgear::CameraInfo* info, osg::Camera* camera, osg::Camera::BufferComponent c, flightgear::CameraKind ck, flightgear::RenderBufferInfo::Kind bk )
{
    camera->attach( c, info->getBuffer(bk) );
    info->getRenderStageInfo(ck).buffers.insert( std::make_pair( c, bk ) );
}

osg::Camera* FGRenderer::buildDeferredGeometryCamera( flightgear::CameraInfo* info, osg::GraphicsContext* gc )
{
    osg::Camera* camera = new osg::Camera;
    info->addCamera(flightgear::GEOMETRY_CAMERA, camera );

    camera->setCullMask( ~simgear::MODELLIGHT_BIT );
    camera->setName( "GeometryC" );
    camera->setGraphicsContext( gc );
    camera->setCullCallback( new FGDeferredRenderingCameraCullCallback( flightgear::GEOMETRY_CAMERA, info ) );
    camera->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    camera->setClearColor( osg::Vec4( 0., 0., 0., 0. ) );
    camera->setClearDepth( 1.0 );
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    camera->setViewport( new osg::Viewport );
    attachBufferToCamera( info, camera, osg::Camera::DEPTH_BUFFER, flightgear::GEOMETRY_CAMERA, flightgear::RenderBufferInfo::DEPTH_BUFFER );
    attachBufferToCamera( info, camera, osg::Camera::COLOR_BUFFER0, flightgear::GEOMETRY_CAMERA, flightgear::RenderBufferInfo::NORMAL_BUFFER );
    attachBufferToCamera( info, camera, osg::Camera::COLOR_BUFFER1, flightgear::GEOMETRY_CAMERA, flightgear::RenderBufferInfo::DIFFUSE_BUFFER );
    attachBufferToCamera( info, camera, osg::Camera::COLOR_BUFFER2, flightgear::GEOMETRY_CAMERA, flightgear::RenderBufferInfo::SPEC_EMIS_BUFFER );
    camera->setDrawBuffer(GL_FRONT);
    camera->setReadBuffer(GL_FRONT);

    camera->addChild( mDeferredRealRoot.get() );

    return camera;
}

static void setShadowCascadeStateSet( osg::Camera* cam ) {
    osg::StateSet* ss = cam->getOrCreateStateSet();
    ss->setAttribute( new osg::PolygonOffset( 1.1f, 5.0f ), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
    ss->setMode( GL_POLYGON_OFFSET_FILL, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
    ss->setRenderBinDetails( 0, "RenderBin", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS );
    ss->setAttributeAndModes( new osg::AlphaFunc( osg::AlphaFunc::GREATER, 0.05 ), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
    ss->setAttributeAndModes( new osg::ColorMask( false, false, false, false ), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
    ss->setAttributeAndModes( new osg::CullFace( osg::CullFace::FRONT ), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
    osg::Program* program = new osg::Program;
    ss->setAttribute( program, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON );
    ss->setMode( GL_LIGHTING, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF );
    ss->setMode( GL_BLEND, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF );
    //ss->setTextureMode( 0, GL_TEXTURE_2D, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON );
    ss->setTextureMode( 0, GL_TEXTURE_3D, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF );
    ss->setTextureMode( 1, GL_TEXTURE_2D, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF );
    ss->setTextureMode( 1, GL_TEXTURE_3D, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF );
    ss->setTextureMode( 2, GL_TEXTURE_2D, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF );
    ss->setTextureMode( 2, GL_TEXTURE_3D, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF );
}

static osg::Camera* createShadowCascadeCamera( int no, int cascadeSize ) {
    osg::Camera* cascadeCam = new osg::Camera;
    setShadowCascadeStateSet( cascadeCam );

    std::ostringstream oss;
    oss << "CascadeCamera" << (no + 1);
    cascadeCam->setName( oss.str() );
    cascadeCam->setClearMask(0);
    cascadeCam->setCullMask(~( simgear::MODELLIGHT_BIT /* | simgear::NO_SHADOW_BIT */ ) );
    cascadeCam->setCullingMode( cascadeCam->getCullingMode() & ~osg::CullSettings::SMALL_FEATURE_CULLING );
    cascadeCam->setAllowEventFocus(false);
    cascadeCam->setReferenceFrame(osg::Transform::ABSOLUTE_RF_INHERIT_VIEWPOINT);
    cascadeCam->setRenderOrder(osg::Camera::NESTED_RENDER);
    cascadeCam->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    cascadeCam->setViewport( int( no / 2 ) * cascadeSize, (no & 1) * cascadeSize, cascadeSize, cascadeSize );
    return cascadeCam;
}

osg::Camera* FGRenderer::buildDeferredShadowCamera( flightgear::CameraInfo* info, osg::GraphicsContext* gc )
{
    osg::Camera* mainShadowCamera = new osg::Camera;
    info->addCamera(flightgear::SHADOW_CAMERA, mainShadowCamera, 0.0f );

    mainShadowCamera->setName( "ShadowC" );
    mainShadowCamera->setClearMask( GL_DEPTH_BUFFER_BIT );
    mainShadowCamera->setClearDepth( 1.0 );
    mainShadowCamera->setAllowEventFocus(false);
    mainShadowCamera->setGraphicsContext(gc);
    mainShadowCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    attachBufferToCamera( info, mainShadowCamera, osg::Camera::DEPTH_BUFFER, flightgear::SHADOW_CAMERA, flightgear::RenderBufferInfo::SHADOW_BUFFER );
    mainShadowCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    mainShadowCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    mainShadowCamera->setProjectionMatrix(osg::Matrix::identity());
    mainShadowCamera->setCullingMode( osg::CullSettings::NO_CULLING );
    mainShadowCamera->setViewport( 0, 0, _shadowMapSize, _shadowMapSize );
    mainShadowCamera->setDrawBuffer(GL_FRONT);
    mainShadowCamera->setReadBuffer(GL_FRONT);

    osg::Switch* shadowSwitch = new osg::Switch;
    mainShadowCamera->addChild( shadowSwitch );

    for (int i = 0; i < 4; ++i ) {
        osg::Camera* cascadeCam = createShadowCascadeCamera( i, _shadowMapSize/2 );
        cascadeCam->addChild( mDeferredRealRoot.get() );
        shadowSwitch->addChild( cascadeCam );
        info->shadowTexGen[i] = new osg::TexGen;
    }
    if (fgGetBool("/sim/rendering/shadows/enabled", true))
        shadowSwitch->setAllChildrenOn();
    else
        shadowSwitch->setAllChildrenOff();

    return mainShadowCamera;
}

void FGRenderer::updateShadowCascade(const CameraInfo* info, osg::Camera* camera, osg::Group* grp, int idx, double left, double right, double bottom, double top, double zNear, double f1, double f2)
{
    osg::Camera* cascade = static_cast<osg::Camera*>( grp->getChild(idx) );
    osg::Matrixd &viewMatrix = cascade->getViewMatrix();
    osg::Matrixd &projectionMatrix = cascade->getProjectionMatrix();

    osg::BoundingSphere bs;
    bs.expandBy(osg::Vec3(left,bottom,-zNear) * f1);
    bs.expandBy(osg::Vec3(right,top,-zNear) * f2);
    bs.expandBy(osg::Vec3(left,bottom,-zNear) * f2);
    bs.expandBy(osg::Vec3(right,top,-zNear) * f1);

    osg::Vec4 aim = osg::Vec4(bs.center(), 1.0) * camera->getInverseViewMatrix();

    projectionMatrix.makeOrtho( -bs.radius(), bs.radius(), -bs.radius(), bs.radius(), 1., 15000.0 );
    osg::Vec3 position( aim.x(), aim.y(), aim.z() );
    viewMatrix.makeLookAt( position + (getSunDirection() * 10000.0), position, position );
}

osg::Vec3 FGRenderer::getSunDirection() const
{
    osg::Vec3 val;
    _sunDirection->get( val );
    return val;
}

void FGRenderer::updateShadowCamera(const flightgear::CameraInfo* info, const osg::Vec3d& position)
{
    ref_ptr<Camera> mainShadowCamera = info->getCamera( SHADOW_CAMERA );
    if (mainShadowCamera.valid()) {
        ref_ptr<Switch> shadowSwitch = mainShadowCamera->getChild( 0 )->asSwitch();
        osg::Vec3d up = position,
            dir = getSunDirection();
        up.normalize();
        dir.normalize();
        // cos(100 deg) == -0.17
        if (up * dir < -0.17 || !fgGetBool("/sim/rendering/shadows/enabled", true)) {
            shadowSwitch->setAllChildrenOff();
        } else {
            double left,right,bottom,top,zNear,zFar;
            ref_ptr<Camera> camera = info->getCamera(GEOMETRY_CAMERA);
            camera->getProjectionMatrix().getFrustum(left,right,bottom,top,zNear,zFar);

            shadowSwitch->setAllChildrenOn();
            if (_numCascades == 1) {
                osg::Camera* cascadeCam = static_cast<osg::Camera*>( shadowSwitch->getChild(0) );
                cascadeCam->setViewport( 0, 0, _shadowMapSize, _shadowMapSize );
            } else {
                for (int no = 0; no < 4; ++no) {
                    osg::Camera* cascadeCam = static_cast<osg::Camera*>( shadowSwitch->getChild(no) );
                    cascadeCam->setViewport( int( no / 2 ) * (_shadowMapSize/2), (no & 1) * (_shadowMapSize/2), (_shadowMapSize/2), (_shadowMapSize/2) );
                }
            }
            updateShadowCascade(info, camera, shadowSwitch, 0, left, right, bottom, top, zNear, 1.0, _cascadeFar[0]/zNear);
            if (_numCascades > 1) {
                shadowSwitch->setValue(1, true);
                updateShadowCascade(info, camera, shadowSwitch, 1, left, right, bottom, top, zNear, _cascadeFar[0]/zNear, _cascadeFar[1]/zNear);
            } else {
                shadowSwitch->setValue(1, false);
            }
            if (_numCascades > 2) {
                shadowSwitch->setValue(2, true);
                updateShadowCascade(info, camera, shadowSwitch, 2, left, right, bottom, top, zNear, _cascadeFar[1]/zNear, _cascadeFar[2]/zNear);
            } else {
                shadowSwitch->setValue(2, false);
            }
            if (_numCascades > 3) {
                shadowSwitch->setValue(3, true);
                updateShadowCascade(info, camera, shadowSwitch, 3, left, right, bottom, top, zNear, _cascadeFar[2]/zNear, _cascadeFar[3]/zNear);
            } else {
                shadowSwitch->setValue(3, false);
            }
            {
            osg::Matrixd &viewMatrix = mainShadowCamera->getViewMatrix();

            osg::Vec4 aim = osg::Vec4( 0.0, 0.0, 0.0, 1.0 ) * camera->getInverseViewMatrix();

            osg::Vec3 position( aim.x(), aim.y(), aim.z() );
            viewMatrix.makeLookAt( position, position + (getSunDirection() * 10000.0), position );
            }
        }
    }
}

void FGRenderer::updateShadowMapSize(int mapSize)
{
    if ( ((~( mapSize-1 )) & mapSize) != mapSize ) {
        SG_LOG( SG_VIEW, SG_ALERT, "Map size is not a power of two" );
        return;
    }
    for (   CameraGroup::CameraIterator ii = CameraGroup::getDefault()->camerasBegin();
            ii != CameraGroup::getDefault()->camerasEnd();
            ++ii )
    {
        CameraInfo* info = ii->get();
        Camera* camera = info->getCamera(SHADOW_CAMERA);
        if (camera == 0) continue;

        Texture2D* tex = info->getBuffer(RenderBufferInfo::SHADOW_BUFFER);
        if (tex == 0) continue;

        tex->setTextureSize( mapSize, mapSize );
        tex->dirtyTextureObject();

        Viewport* vp = camera->getViewport();
        vp->width() = mapSize;
        vp->height() = mapSize;

        osgViewer::Renderer* renderer
            = static_cast<osgViewer::Renderer*>(camera->getRenderer());
        for (int i = 0; i < 2; ++i) {
            osgUtil::SceneView* sceneView = renderer->getSceneView(i);
            sceneView->getRenderStage()->setFrameBufferObject(0);
            sceneView->getRenderStage()->setCameraRequiresSetUp(true);
            if (sceneView->getRenderStageLeft()) {
                sceneView->getRenderStageLeft()->setFrameBufferObject(0);
                sceneView->getRenderStageLeft()->setCameraRequiresSetUp(true);
            }
            if (sceneView->getRenderStageRight()) {
                sceneView->getRenderStageRight()->setFrameBufferObject(0);
                sceneView->getRenderStageRight()->setCameraRequiresSetUp(true);
            }
        }

        int cascadeSize = mapSize / 2;
        Group* grp = camera->getChild(0)->asGroup();
        for (int i = 0; i < 4; ++i ) {
            Camera* cascadeCam = static_cast<Camera*>( grp->getChild(i) );
            cascadeCam->setViewport( int( i / 2 ) * cascadeSize, (i & 1) * cascadeSize, cascadeSize, cascadeSize );
        }

        _shadowMapSize = mapSize;
    }
}

void FGRenderer::enableShadows(bool enabled)
{
    for (   CameraGroup::CameraIterator ii = CameraGroup::getDefault()->camerasBegin();
            ii != CameraGroup::getDefault()->camerasEnd();
            ++ii )
    {
        CameraInfo* info = ii->get();
        Camera* camera = info->getCamera(SHADOW_CAMERA);
        if (camera == 0) continue;

        osg::Switch* shadowSwitch = camera->getChild(0)->asSwitch();
        if (enabled)
            shadowSwitch->setAllChildrenOn();
        else
            shadowSwitch->setAllChildrenOff();
    }
}

void FGRenderer::updateCascadeFar(int index, float far_m)
{
    if (index < 0 || index > 3)
        return;
    _cascadeFar[index] = far_m;
    _shadowDistances->set( osg::Vec4f(_cascadeFar[0], _cascadeFar[1], _cascadeFar[2], _cascadeFar[3]) );
}

void FGRenderer::updateCascadeNumber(size_t num)
{
    if (num < 1 || num > 4)
        return;
    _numCascades = num;
    _shadowNumber->set( (int)_numCascades );
}


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

const char *ambient_vert_src = ""
    "#line " TOSTRING(__LINE__) " 1\n"
    "void main() {\n"
    "    gl_Position = gl_Vertex;\n"
    "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "}\n";

const char *ambient_frag_src = ""
    "#line " TOSTRING(__LINE__) " 1\n"
    "uniform sampler2D color_tex;\n"
//    "uniform sampler2D ao_tex;\n"
    "uniform sampler2D normal_tex;\n"
    "uniform sampler2D spec_emis_tex;\n"
    "uniform vec4 fg_SunAmbientColor;\n"
    "void main() {\n"
    "    vec2 coords = gl_TexCoord[0].xy;\n"
    "    float initialized = texture2D( spec_emis_tex, coords ).a;\n"
    "    if ( initialized < 0.1 )\n"
    "        discard;\n"
    "    vec3 tcolor = texture2D( color_tex, coords ).rgb;\n"
//    "    float ao = texture2D( ao_tex, coords ).r;\n"
//    "    gl_FragColor = vec4(tcolor*fg_SunAmbientColor.rgb*ao, 1.0);\n"
    "    gl_FragColor = vec4(tcolor*fg_SunAmbientColor.rgb, 1.0);\n"
    "}\n";

const char *sunlight_vert_src = ""
    "#line " TOSTRING(__LINE__) " 1\n"
//  "uniform mat4 fg_ViewMatrixInverse;\n"
    "uniform mat4 fg_ProjectionMatrixInverse;\n"
    "varying vec3 ray;\n"
    "void main() {\n"
    "    gl_Position = gl_Vertex;\n"
    "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
//  "    ray = (fg_ViewMatrixInverse * vec4((fg_ProjectionMatrixInverse * gl_Vertex).xyz, 0.0)).xyz;\n"
    "    ray = (fg_ProjectionMatrixInverse * gl_Vertex).xyz;\n"
    "}\n";

const char *sunlight_frag_src = ""
#if 0
    "#version 130\n"
#endif
    "#line " TOSTRING(__LINE__) " 1\n"
    "uniform mat4 fg_ViewMatrix;\n"
    "uniform sampler2D depth_tex;\n"
    "uniform sampler2D normal_tex;\n"
    "uniform sampler2D color_tex;\n"
    "uniform sampler2D spec_emis_tex;\n"
    "uniform sampler2DShadow shadow_tex;\n"
    "uniform vec4 fg_SunDiffuseColor;\n"
    "uniform vec4 fg_SunSpecularColor;\n"
    "uniform vec3 fg_SunDirection;\n"
    "uniform vec3 fg_Planes;\n"
    "varying vec3 ray;\n"
    "vec4 DynamicShadow( in vec4 ecPosition, out vec4 tint )\n"
    "{\n"
    "    vec4 coords;\n"
    "    vec2 shift = vec2( 0.0 );\n"
    "    int index = 4;\n"
    "    if (ecPosition.z > -5.0) {\n"
    "        index = 1;\n"
    "        tint = vec4(0.0,1.0,0.0,1.0);\n"
    "    } else if (ecPosition.z > -50.0) {\n"
    "        index = 2;\n"
    "        shift = vec2( 0.0, 0.5 );\n"
    "        tint = vec4(0.0,0.0,1.0,1.0);\n"
    "    } else if (ecPosition.z > -512.0) {\n"
    "        index = 3;\n"
    "        shift = vec2( 0.5, 0.0 );\n"
    "        tint = vec4(1.0,1.0,0.0,1.0);\n"
    "    } else if (ecPosition.z > -10000.0) {\n"
    "        shift = vec2( 0.5, 0.5 );\n"
    "        tint = vec4(1.0,0.0,0.0,1.0);\n"
    "    } else {\n"
    "        return vec4(1.1,1.1,0.0,1.0);\n" // outside, clamp to border
    "    }\n"
    "    coords.s = dot( ecPosition, gl_EyePlaneS[index] );\n"
    "    coords.t = dot( ecPosition, gl_EyePlaneT[index] );\n"
    "    coords.p = dot( ecPosition, gl_EyePlaneR[index] );\n"
    "    coords.q = dot( ecPosition, gl_EyePlaneQ[index] );\n"
    "    coords.st *= .5;\n"
    "    coords.st += shift;\n"
    "    return coords;\n"
    "}\n"
    "void main() {\n"
    "    vec2 coords = gl_TexCoord[0].xy;\n"
    "    vec4 spec_emis = texture2D( spec_emis_tex, coords );\n"
    "    if ( spec_emis.a < 0.1 )\n"
    "        discard;\n"
    "    vec3 normal;\n"
    "    normal.xy = texture2D( normal_tex, coords ).rg * 2.0 - vec2(1.0,1.0);\n"
    "    normal.z = sqrt( 1.0 - dot( normal.xy, normal.xy ) );\n"
    "    float len = length(normal);\n"
    "    normal /= len;\n"
    "    vec3 viewDir = normalize(ray);\n"
    "    float depth = texture2D( depth_tex, coords ).r;\n"
    "    vec3 pos;\n"
    "    pos.z = - fg_Planes.y / (fg_Planes.x + depth * fg_Planes.z);\n"
    "    pos.xy = viewDir.xy / viewDir.z * pos.z;\n"

    "    vec4 tint;\n"
#if 0
    "    float shadow = 1.0;\n"
#elif 1
    "    float shadow = shadow2DProj( shadow_tex, DynamicShadow( vec4( pos, 1.0 ), tint ) ).r;\n"
#else
    "    float kernel[9] = float[]( 36/256.0, 24/256.0, 6/256.0,\n"
    "                           24/256.0, 16/256.0, 4/256.0,\n"
    "                           6/256.0,  4/256.0, 1/256.0 );\n"
    "    float shadow = 0;\n"
    "    for( int x = -2; x <= 2; ++x )\n"
    "      for( int y = -2; y <= 2; ++y )\n"
    "        shadow += kernel[abs(x)*3 + abs(y)] * shadow2DProj( shadow_tex, DynamicShadow( vec4(pos + vec3(0.05 * x, 0.05 * y, 0), 1.0), tint ) ).r;\n"
#endif
    "    vec3 lightDir = (fg_ViewMatrix * vec4( fg_SunDirection, 0.0 )).xyz;\n"
    "    lightDir = normalize( lightDir );\n"
    "    vec3 color = texture2D( color_tex, coords ).rgb;\n"
    "    vec3 Idiff = clamp( dot( lightDir, normal ), 0.0, 1.0 ) * color * fg_SunDiffuseColor.rgb;\n"
    "    vec3 halfDir = lightDir - viewDir;\n"
    "    len = length( halfDir );\n"
    "    vec3 Ispec = vec3(0.0);\n"
    "    vec3 Iemis = spec_emis.z * color;\n"
    "    if (len > 0.0001) {\n"
    "        halfDir /= len;\n"
    "        Ispec = pow( clamp( dot( halfDir, normal ), 0.0, 1.0 ), spec_emis.y * 255.0 ) * spec_emis.x * fg_SunSpecularColor.rgb;\n"
    "    }\n"
    "    gl_FragColor = vec4(mix(vec3(0.0), Idiff + Ispec, shadow) + Iemis, 1.0);\n"
//    "    gl_FragColor = mix(tint, vec4(mix(vec3(0.0), Idiff + Ispec, shadow) + Iemis, 1.0), 0.92);\n"
    "}\n";

const char *fog_vert_src = ""
    "#line " TOSTRING(__LINE__) " 1\n"
    "uniform mat4 fg_ProjectionMatrixInverse;\n"
    "varying vec3 ray;\n"
    "void main() {\n"
    "    gl_Position = gl_Vertex;\n"
    "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "    ray = (fg_ProjectionMatrixInverse * gl_Vertex).xyz;\n"
    "}\n";

const char *fog_frag_src = ""
    "#line " TOSTRING(__LINE__) " 1\n"
    "uniform sampler2D depth_tex;\n"
    "uniform sampler2D normal_tex;\n"
    "uniform sampler2D color_tex;\n"
    "uniform sampler2D spec_emis_tex;\n"
    "uniform vec4 fg_FogColor;\n"
    "uniform float fg_FogDensity;\n"
    "uniform vec3 fg_Planes;\n"
    "varying vec3 ray;\n"
    "void main() {\n"
    "    vec2 coords = gl_TexCoord[0].xy;\n"
    "    float initialized = texture2D( spec_emis_tex, coords ).a;\n"
    "    if ( initialized < 0.1 )\n"
    "        discard;\n"
    "    vec3 normal;\n"
    "    normal.xy = texture2D( normal_tex, coords ).rg * 2.0 - vec2(1.0,1.0);\n"
    "    normal.z = sqrt( 1.0 - dot( normal.xy, normal.xy ) );\n"
    "    float len = length(normal);\n"
    "    normal /= len;\n"
    "    vec3 viewDir = normalize(ray);\n"
    "    float depth = texture2D( depth_tex, coords ).r;\n"
    "    vec3 pos;\n"
    "    pos.z = - fg_Planes.y / (fg_Planes.x + depth * fg_Planes.z);\n"
    "    pos.xy = viewDir.xy / viewDir.z * pos.z;\n"

    "    float fogFactor = 0.0;\n"
    "    const float LOG2 = 1.442695;\n"
    "    fogFactor = exp2(-fg_FogDensity * fg_FogDensity * pos.z * pos.z * LOG2);\n"
    "    fogFactor = clamp(fogFactor, 0.0, 1.0);\n"

    "    gl_FragColor = vec4(fg_FogColor.rgb, 1.0 - fogFactor);\n"
    "}\n";

osg::Camera* FGRenderer::buildDeferredLightingCamera( flightgear::CameraInfo* info, osg::GraphicsContext* gc )
{
    osg::Camera* camera = new osg::Camera;
    info->addCamera(flightgear::LIGHTING_CAMERA, camera );

    camera->setCullCallback( new FGDeferredRenderingCameraCullCallback( flightgear::LIGHTING_CAMERA, info ) );
    camera->setAllowEventFocus(false);
    camera->setGraphicsContext(gc);
    camera->setViewport(new Viewport);
    camera->setName("LightingC");
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setRenderOrder(osg::Camera::POST_RENDER, 50);
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    camera->setViewport( new osg::Viewport );
    attachBufferToCamera( info, camera, osg::Camera::DEPTH_BUFFER, flightgear::LIGHTING_CAMERA, flightgear::RenderBufferInfo::DEPTH_BUFFER );
    attachBufferToCamera( info, camera, osg::Camera::COLOR_BUFFER, flightgear::LIGHTING_CAMERA, flightgear::RenderBufferInfo::LIGHTING_BUFFER );
    camera->setDrawBuffer(GL_FRONT);
    camera->setReadBuffer(GL_FRONT);
    camera->setClearColor( osg::Vec4( 0., 0., 0., 1. ) );
    camera->setClearMask( GL_COLOR_BUFFER_BIT );
    osg::StateSet* ss = camera->getOrCreateStateSet();
    ss->setAttribute( new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false) );

    osg::Group* lightingGroup = new osg::Group;

    osg::Camera* quadCam1 = new osg::Camera;
    quadCam1->setName( "QuadCamera1" );
    quadCam1->setClearMask(0);
    quadCam1->setAllowEventFocus(false);
    quadCam1->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    quadCam1->setRenderOrder(osg::Camera::NESTED_RENDER);
    quadCam1->setViewMatrix(osg::Matrix::identity());
    quadCam1->setProjectionMatrixAsOrtho2D(-1,1,-1,1);
    quadCam1->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    ss = quadCam1->getOrCreateStateSet();
    ss->addUniform( _ambientFactor );
    ss->addUniform( info->projInverse );
    ss->addUniform( info->viewInverse );
    ss->addUniform( info->view );
    ss->addUniform( _sunDiffuse );
    ss->addUniform( _sunSpecular );
    ss->addUniform( _sunDirection );
    ss->addUniform( _planes );
    ss->addUniform( _shadowNumber );
    ss->addUniform( _shadowDistances );

    osg::Geometry* g = osg::createTexturedQuadGeometry( osg::Vec3(-1.,-1.,0.), osg::Vec3(2.,0.,0.), osg::Vec3(0.,2.,0.) );
    g->setUseDisplayList(false);
    simgear::EffectGeode* eg = new simgear::EffectGeode;
    simgear::Effect* effect = simgear::makeEffect("Effects/ambient", true);
    if (effect) {
        eg->setEffect( effect );
    } else {
        SG_LOG( SG_VIEW, SG_ALERT, "=> Using default, builtin, Effects/ambient" );
        ss = eg->getOrCreateStateSet();
        ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
        ss->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );
        ss->setTextureAttributeAndModes( 0, info->getBuffer( flightgear::RenderBufferInfo::DEPTH_BUFFER ) );
        ss->setTextureAttributeAndModes( 1, info->getBuffer( flightgear::RenderBufferInfo::NORMAL_BUFFER ) );
        ss->setTextureAttributeAndModes( 2, info->getBuffer( flightgear::RenderBufferInfo::DIFFUSE_BUFFER ) );
        ss->setTextureAttributeAndModes( 3, info->getBuffer( flightgear::RenderBufferInfo::SPEC_EMIS_BUFFER ) );
        //ss->setTextureAttributeAndModes( 4, info->gBuffer->aoBuffer[2] );
        ss->addUniform( new osg::Uniform( "depth_tex", 0 ) );
        ss->addUniform( new osg::Uniform( "normal_tex", 1 ) );
        ss->addUniform( new osg::Uniform( "color_tex", 2 ) );
        ss->addUniform( new osg::Uniform( "spec_emis_tex", 3 ) );
        //ss->addUniform( new osg::Uniform( "ao_tex", 4 ) );
        ss->setRenderBinDetails( 0, "RenderBin" );
        osg::Program* program = new osg::Program;
        program->addShader( new osg::Shader( osg::Shader::VERTEX, ambient_vert_src ) );
        program->addShader( new osg::Shader( osg::Shader::FRAGMENT, ambient_frag_src ) );
        ss->setAttributeAndModes( program );
    }

    g->setName( "AmbientQuad" );
    eg->setName("AmbientQuad");
    eg->setCullingActive(false);
    eg->addDrawable(g);
    quadCam1->addChild( eg );

    g = osg::createTexturedQuadGeometry( osg::Vec3(-1.,-1.,0.), osg::Vec3(2.,0.,0.), osg::Vec3(0.,2.,0.) );
    g->setUseDisplayList(false);
    g->setName( "SunlightQuad" );
    eg = new simgear::EffectGeode;
    effect = simgear::makeEffect("Effects/sunlight", true);
    if (effect) {
        eg->setEffect( effect );
    } else {
        SG_LOG( SG_VIEW, SG_ALERT, "=> Using default, builtin, Effects/sunlight" );
        ss = eg->getOrCreateStateSet();
        ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
        ss->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );
        ss->setAttributeAndModes( new osg::BlendFunc( osg::BlendFunc::ONE, osg::BlendFunc::ONE ) );
        ss->setTextureAttribute( 0, info->getBuffer( flightgear::RenderBufferInfo::DEPTH_BUFFER ) );
        ss->setTextureAttribute( 1, info->getBuffer( flightgear::RenderBufferInfo::NORMAL_BUFFER ) );
        ss->setTextureAttribute( 2, info->getBuffer( flightgear::RenderBufferInfo::DIFFUSE_BUFFER ) );
        ss->setTextureAttribute( 3, info->getBuffer( flightgear::RenderBufferInfo::SPEC_EMIS_BUFFER ) );
        ss->setTextureAttribute( 4, info->getBuffer( flightgear::RenderBufferInfo::SHADOW_BUFFER ) );
        ss->addUniform( new osg::Uniform( "depth_tex", 0 ) );
        ss->addUniform( new osg::Uniform( "normal_tex", 1 ) );
        ss->addUniform( new osg::Uniform( "color_tex", 2 ) );
        ss->addUniform( new osg::Uniform( "spec_emis_tex", 3 ) );
        ss->addUniform( new osg::Uniform( "shadow_tex", 4 ) );
        ss->setRenderBinDetails( 1, "RenderBin" );
        osg::Program* program = new osg::Program;
        program->addShader( new osg::Shader( osg::Shader::VERTEX, sunlight_vert_src ) );
        program->addShader( new osg::Shader( osg::Shader::FRAGMENT, sunlight_frag_src ) );
        ss->setAttributeAndModes( program );
    }
    eg->setName("SunlightQuad");
    eg->setCullingActive(false);
    eg->addDrawable(g);
    quadCam1->addChild( eg );

    osg::Camera* lightCam = new osg::Camera;
    ss = lightCam->getOrCreateStateSet();
    ss->addUniform( _planes );
    ss->addUniform( info->bufferSize );
    lightCam->setName( "LightCamera" );
    lightCam->setClearMask(0);
    lightCam->setAllowEventFocus(false);
    lightCam->setReferenceFrame(osg::Transform::RELATIVE_RF);
    lightCam->setRenderOrder(osg::Camera::NESTED_RENDER,1);
    lightCam->setViewMatrix(osg::Matrix::identity());
    lightCam->setProjectionMatrix(osg::Matrix::identity());
    lightCam->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    lightCam->setCullMask( simgear::MODELLIGHT_BIT );
    lightCam->setInheritanceMask( osg::CullSettings::ALL_VARIABLES & ~osg::CullSettings::CULL_MASK );
    lightCam->addChild( mDeferredRealRoot.get() );


    osg::Camera* quadCam2 = new osg::Camera;
    quadCam2->setName( "QuadCamera1" );
    quadCam2->setClearMask(0);
    quadCam2->setAllowEventFocus(false);
    quadCam2->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    quadCam2->setRenderOrder(osg::Camera::NESTED_RENDER,2);
    quadCam2->setViewMatrix(osg::Matrix::identity());
    quadCam2->setProjectionMatrixAsOrtho2D(-1,1,-1,1);
    quadCam2->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    ss = quadCam2->getOrCreateStateSet();
    ss->addUniform( _ambientFactor );
    ss->addUniform( info->projInverse );
    ss->addUniform( info->viewInverse );
    ss->addUniform( info->view );
    ss->addUniform( _sunDiffuse );
    ss->addUniform( _sunSpecular );
    ss->addUniform( _sunDirection );
    ss->addUniform( _fogColor );
    ss->addUniform( _fogDensity );
    ss->addUniform( _planes );

    g = osg::createTexturedQuadGeometry( osg::Vec3(-1.,-1.,0.), osg::Vec3(2.,0.,0.), osg::Vec3(0.,2.,0.) );
    g->setUseDisplayList(false);
    g->setName( "FogQuad" );
    eg = new simgear::EffectGeode;
    effect = simgear::makeEffect("Effects/fog", true);
    if (effect) {
        eg->setEffect( effect );
    } else {
        SG_LOG( SG_VIEW, SG_ALERT, "=> Using default, builtin, Effects/fog" );
        ss = eg->getOrCreateStateSet();
        ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
        ss->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );
        ss->setAttributeAndModes( new osg::BlendFunc( osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA ) );
        ss->setTextureAttributeAndModes( 0, info->getBuffer( flightgear::RenderBufferInfo::DEPTH_BUFFER ) );
        ss->setTextureAttributeAndModes( 1, info->getBuffer( flightgear::RenderBufferInfo::NORMAL_BUFFER ) );
        ss->setTextureAttributeAndModes( 2, info->getBuffer( flightgear::RenderBufferInfo::DIFFUSE_BUFFER ) );
        ss->setTextureAttributeAndModes( 3, info->getBuffer( flightgear::RenderBufferInfo::SPEC_EMIS_BUFFER ) );
        ss->addUniform( new osg::Uniform( "depth_tex", 0 ) );
        ss->addUniform( new osg::Uniform( "normal_tex", 1 ) );
        ss->addUniform( new osg::Uniform( "color_tex", 2 ) );
        ss->addUniform( new osg::Uniform( "spec_emis_tex", 3 ) );
        ss->setRenderBinDetails( 10000, "RenderBin" );
        osg::Program* program = new osg::Program;
        program->addShader( new osg::Shader( osg::Shader::VERTEX, fog_vert_src ) );
        program->addShader( new osg::Shader( osg::Shader::FRAGMENT, fog_frag_src ) );
        ss->setAttributeAndModes( program );
    }
    eg->setName("FogQuad");
    eg->setCullingActive(false);
    eg->addDrawable(g);
    quadCam2->addChild( eg );

    lightingGroup->addChild( _sky->getPreRoot() );
    lightingGroup->addChild( _sky->getCloudRoot() );
    lightingGroup->addChild( quadCam1 );
    lightingGroup->addChild( lightCam );
    lightingGroup->addChild( quadCam2 );

    camera->addChild( lightingGroup );

    return camera;
}

flightgear::CameraInfo*
FGRenderer::buildDeferredPipeline(flightgear::CameraGroup* cgroup, unsigned flags, osg::Camera* camera,
                                    const osg::Matrix& view,
                                    const osg::Matrix& projection,
                                    osg::GraphicsContext* gc)
{
    CameraInfo* info = new CameraInfo(flags);
	buildDeferredBuffers( info, _shadowMapSize, !fgGetBool("/sim/rendering/no-16bit-buffer", false ) );

    osg::Camera* geometryCamera = buildDeferredGeometryCamera( info, gc );
    cgroup->getViewer()->addSlave(geometryCamera, false);
    installCullVisitor(geometryCamera);
    int slaveIndex = cgroup->getViewer()->getNumSlaves() - 1;
    info->getRenderStageInfo(GEOMETRY_CAMERA).slaveIndex = slaveIndex;
    
    Camera* shadowCamera = buildDeferredShadowCamera( info, gc );
    cgroup->getViewer()->addSlave(shadowCamera, false);
    installCullVisitor(shadowCamera);
    slaveIndex = cgroup->getViewer()->getNumSlaves() - 1;
    info->getRenderStageInfo(SHADOW_CAMERA).slaveIndex = slaveIndex;

    osg::Camera* lightingCamera = buildDeferredLightingCamera( info, gc );
    cgroup->getViewer()->addSlave(lightingCamera, false);
    installCullVisitor(lightingCamera);
    slaveIndex = cgroup->getViewer()->getNumSlaves() - 1;
    info->getRenderStageInfo(LIGHTING_CAMERA).slaveIndex = slaveIndex;

    camera->setName( "DisplayC" );
    camera->setCullCallback( new FGDeferredRenderingCameraCullCallback( flightgear::DISPLAY_CAMERA, info ) );
    camera->setReferenceFrame(Transform::ABSOLUTE_RF);
    camera->setAllowEventFocus(false);
    osg::Geometry* g = osg::createTexturedQuadGeometry( osg::Vec3(-1.,-1.,0.), osg::Vec3(2.,0.,0.), osg::Vec3(0.,2.,0.) );
    g->setUseDisplayList(false); //DEBUG
    simgear::EffectGeode* eg = new simgear::EffectGeode;
    simgear::Effect* effect = simgear::makeEffect("Effects/display", true);
    if (!effect) {
        SG_LOG(SG_VIEW, SG_ALERT, "Effects/display not found");
        return 0;
    }
    eg->setEffect(effect);
    eg->setCullingActive(false);
    eg->addDrawable(g);
    camera->setViewMatrix(osg::Matrix::identity());
    camera->setProjectionMatrixAsOrtho2D(-1,1,-1,1);
    camera->addChild(eg);

    cgroup->getViewer()->addSlave(camera, false);
    installCullVisitor(camera);
    slaveIndex = cgroup->getViewer()->getNumSlaves() - 1;
    info->addCamera( DISPLAY_CAMERA, camera, slaveIndex, true );
    camera->setRenderOrder(Camera::POST_RENDER, 99+slaveIndex); //FIXME
    cgroup->addCamera(info);
    return info;
}


void
FGRenderer::setupView( void )
{
    osgViewer::Viewer* viewer = globals->get_renderer()->getViewer();
    osg::initNotifyLevel();

    // The number of polygon-offset "units" to place between layers.  In
    // principle, one is supposed to be enough.  In practice, I find that
    // my hardware/driver requires many more.
    osg::PolygonOffset::setUnitsMultiplier(1);
    osg::PolygonOffset::setFactorMultiplier(1);

    // Go full screen if requested ...
    if ( fgGetBool("/sim/startup/fullscreen") )
        fgOSFullScreen();

// build the sky    
    // The sun and moon diameters are scaled down numbers of the
    // actual diameters. This was needed to fit both the sun and the
    // moon within the distance to the far clip plane.
    // Moon diameter:    3,476 kilometers
    // Sun diameter: 1,390,000 kilometers
    _sky->build( 80000.0, 80000.0,
                  463.3, 361.8,
                  *globals->get_ephem(),
                  fgGetNode("/environment", true));
    
    viewer->getCamera()
        ->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    
    osg::StateSet* stateSet = mRoot->getOrCreateStateSet();

    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    stateSet->setAttribute(new osg::Depth(osg::Depth::LESS));
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    stateSet->setAttribute(new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01));
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
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

    osg::Group* sceneGroup = new osg::Group;
    sceneGroup->addChild(globals->get_scenery()->get_scene_graph());
    sceneGroup->setNodeMask(~simgear::BACKGROUND_BIT);

    //sceneGroup->addChild(thesky->getCloudRoot());

    stateSet = sceneGroup->getOrCreateStateSet();
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

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
    LightSource* lightSource = new LightSource;
    lightSource->getLight()->setDataVariance(Object::DYNAMIC);
    // relative because of CameraView being just a clever transform node
    lightSource->setReferenceFrame(osg::LightSource::RELATIVE_RF);
    lightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    lightSource->setUpdateCallback(new FGLightSourceUpdateCallback);
    mRealRoot->addChild(lightSource);
    // we need a white diffuse light for the phase of the moon
    osg::LightSource* sunLight = new osg::LightSource;
    sunLight->getLight()->setDataVariance(Object::DYNAMIC);
    sunLight->getLight()->setLightNum(1);
    sunLight->setUpdateCallback(new FGLightSourceUpdateCallback(true));
    sunLight->setReferenceFrame(osg::LightSource::RELATIVE_RF);
    sunLight->setLocalStateSetModes(osg::StateAttribute::ON);
    
    // Hang a StateSet above the sky subgraph in order to turn off
    // light 0
    Group* skyGroup = new Group;
    StateSet* skySS = skyGroup->getOrCreateStateSet();
    skySS->setMode(GL_LIGHT0, StateAttribute::OFF);
    skyGroup->addChild(_sky->getPreRoot());
    sunLight->addChild(skyGroup);
    mRoot->addChild(sceneGroup);
    if ( _classicalRenderer )
        mRoot->addChild(sunLight);
    
    // Clouds are added to the scene graph later
    stateSet = globals->get_scenery()->get_scene_graph()->getOrCreateStateSet();
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
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

    // plug in the GUI
    osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
    if (guiCamera) {
        
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(new SGPuDrawable);
        geode->addDrawable(new SGHUDDrawable);
        guiCamera->addChild(geode);
      
        panelSwitch = new osg::Switch;
        osg::StateSet* stateSet = panelSwitch->getOrCreateStateSet();
        stateSet->setRenderBinDetails(1000, "RenderBin");
        
        // speed optimization?
        stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        stateSet->setAttribute(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        
        
        panelSwitch->setUpdateCallback(new FGPanelSwitchCallback);
        panelChanged();
        
        guiCamera->addChild(panelSwitch.get());
    }
    
    osg::Switch* sw = new osg::Switch;
    sw->setUpdateCallback(new FGScenerySwitchCallback);
    sw->addChild(mRoot.get());
    mRealRoot->addChild(sw);
    // The clouds are attached directly to the scene graph root
    // because, in theory, they don't want the same default state set
    // as the rest of the scene. This may not be true in practice.
	if ( _classicalRenderer ) {
		mRealRoot->addChild(_sky->getCloudRoot());
		mRealRoot->addChild(FGCreateRedoutNode());
	}
    // Attach empty program to the scene root so that shader programs
    // don't leak into state sets (effects) that shouldn't have one.
    stateSet = mRealRoot->getOrCreateStateSet();
    stateSet->setAttributeAndModes(new osg::Program, osg::StateAttribute::ON);

	mDeferredRealRoot->addChild( mRealRoot.get() );
}

void FGRenderer::panelChanged()
{
    if (!panelSwitch) {
        return;
    }
    
    osg::Node* n = FGPanelNode::createNode(globals->get_current_panel());
    if (panelSwitch->getNumChildren()) {
        panelSwitch->setChild(0, n);
    } else {
        panelSwitch->addChild(n);
    }
}
                                    
// Update all Visuals (redraws anything graphics related)
void
FGRenderer::update( ) {
    if (!(_scenery_loaded->getBoolValue() || 
           _scenery_override->getBoolValue()))
    {
        _splash_alpha->setDoubleValue(1.0);
        return;
    }
    osgViewer::Viewer* viewer = globals->get_renderer()->getViewer();

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
        _splash_alpha->setDoubleValue((sAlpha < 0) ? 0.0 : sAlpha);
    }

    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));
	if (!_classicalRenderer ) {
		_ambientFactor->set( toOsg(l->scene_ambient()) );
		_sunDiffuse->set( toOsg(l->scene_diffuse()) );
		_sunSpecular->set( toOsg(l->scene_specular()) );
		_sunDirection->set( osg::Vec3f(l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2]) );
	}

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

    FGViewer *current__view = globals->get_current_view();
    // Force update of center dependent values ...
    current__view->set_dirty();
  
    osg::Camera *camera = viewer->getCamera();

    bool skyblend = _skyblend->getBoolValue();
    if ( skyblend ) {
	
        if ( _textures->getBoolValue() ) {
            SGVec4f clearColor(l->adj_fog_color());
            camera->setClearColor(toOsg(clearColor));
        }
    } else {
        SGVec4f clearColor(l->sky_color());
        camera->setClearColor(toOsg(clearColor));
    }

    // update fog params if visibility has changed
    double visibility_meters = _visibility_m->getDoubleValue();
    _sky->set_visibility(visibility_meters);

    double altitude_m = _altitude_ft->getDoubleValue() * SG_FEET_TO_METER;
    _sky->modify_vis( altitude_m, 0.0 /* time factor, now unused */);

    // update the sky dome
    if ( skyblend ) {

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
        sstate.pos       = current__view->getViewPosition();
        sstate.pos_geod  = current__view->getPosition();
        sstate.ori       = current__view->getViewOrientation();
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
  
        double delta_time_sec = _sim_delta_sec->getDoubleValue();
        _sky->reposition( sstate, *globals->get_ephem(), delta_time_sec );
        _sky->repaint( scolor, *globals->get_ephem() );

            //OSGFIXME
//         shadows->setupShadows(
//           current__view->getLongitude_deg(),
//           current__view->getLatitude_deg(),
//           globals->get_time_params()->getGst(),
//           globals->get_ephem()->getSunRightAscension(),
//           globals->get_ephem()->getSunDeclination(),
//           l->get_sun_angle());

    }

//     sgEnviro.setLight(l->adj_fog_color());
//     sgEnviro.startOfFrame(current__view->get_view_pos(), 
//         current__view->get_world_up(),
//         current__view->getLongitude_deg(),
//         current__view->getLatitude_deg(),
//         current__view->getAltitudeASL_ft() * SG_FEET_TO_METER,
//         delta_time_sec);

    // OSGFIXME
//     sgEnviro.drawLightning();

//        double current_view_origin_airspeed_horiz_kt =
//         fgGetDouble("/velocities/airspeed-kt", 0.0)
//                        * cos( fgGetDouble("/orientation/pitch-deg", 0.0)
//                                * SGD_DEGREES_TO_RADIANS);

    // OSGFIXME
//     if( is_internal )
//         shadows->endOfFrame();

    // need to call the update visitor once
    mFrameStamp->setCalendarTime(*globals->get_time_params()->getGmt());
    mUpdateVisitor->setViewData(current__view->getViewPosition(),
                                current__view->getViewOrientation());
    SGVec3f direction(l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2]);
    mUpdateVisitor->setLight(direction, l->scene_ambient(),
                             l->scene_diffuse(), l->scene_specular(),
                             l->adj_fog_color(),
                             l->get_sun_angle()*SGD_RADIANS_TO_DEGREES);
    mUpdateVisitor->setVisibility(actual_visibility);
    simgear::GroundLightManager::instance()->update(mUpdateVisitor.get());
    osg::Node::NodeMask cullMask = ~simgear::LIGHTS_BITS & ~simgear::PICK_BIT;
    cullMask |= simgear::GroundLightManager::instance()
        ->getLightNodeMask(mUpdateVisitor.get());
    if (_panel_hotspots->getBoolValue())
        cullMask |= simgear::PICK_BIT;
    CameraGroup::getDefault()->setCameraCullMasks(cullMask);
	if ( !_classicalRenderer ) {
		_fogColor->set( toOsg( l->adj_fog_color() ) );
		_fogDensity->set( float( mUpdateVisitor->getFogExp2Density() ) );
	}
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
}

bool
FGRenderer::pick(std::vector<SGSceneryPick>& pickList,
                 const osgGA::GUIEventAdapter* ea)
{
    // wipe out the return ...
    pickList.clear();
    typedef osgUtil::LineSegmentIntersector::Intersections Intersections;
    Intersections intersections;

    if (!computeIntersections(CameraGroup::getDefault(), ea, intersections))
        return false;
    for (Intersections::iterator hit = intersections.begin(),
             e = intersections.end();
         hit != e;
         ++hit) {
        const osg::NodePath& np = hit->nodePath;
        osg::NodePath::const_reverse_iterator npi;
        for (npi = np.rbegin(); npi != np.rend(); ++npi) {
            SGSceneUserData* ud = SGSceneUserData::getSceneUserData(*npi);
            if (!ud)
                continue;
            for (unsigned i = 0; i < ud->getNumPickCallbacks(); ++i) {
                SGPickCallback* pickCallback = ud->getPickCallback(i);
                if (!pickCallback)
                    continue;
                SGSceneryPick sceneryPick;
                sceneryPick.info.local = toSG(hit->getLocalIntersectPoint());
                sceneryPick.info.wgs84 = toSG(hit->getWorldIntersectPoint());
                sceneryPick.callback = pickCallback;
                pickList.push_back(sceneryPick);
            }
        }
    }
    return !pickList.empty();
}

void
FGRenderer::setViewer(osgViewer::Viewer* viewer_)
{
    viewer = viewer_;
}

void
FGRenderer::setEventHandler(FGEventHandler* eventHandler_)
{
    eventHandler = eventHandler_;
}

void
FGRenderer::addCamera(osg::Camera* camera, bool useSceneData)
{
    mRealRoot->addChild(camera);
}

void
FGRenderer::removeCamera(osg::Camera* camera)
{
    mRealRoot->removeChild(camera);
}
                                    
void
FGRenderer::setPlanes( double zNear, double zFar )
{
	_planes->set( osg::Vec3f( - zFar, - zFar * zNear, zFar - zNear ) );
}

bool
fgDumpSceneGraphToFile(const char* filename)
{
    return osgDB::writeNodeFile(*mRealRoot.get(), filename);
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
    osgViewer::Viewer* viewer = renderer->getViewer();
    VisibleSceneInfoVistor vsv;
    Viewport* vp = 0;
    if (!viewer->getCamera()->getViewport() && viewer->getNumSlaves() > 0) {
        const View::Slave& slave = viewer->getSlave(0);
        vp = slave._camera->getViewport();
    }
    vsv.doTraversal(viewer->getCamera(), viewer->getSceneData(), vp);
    return true;
}

}
// end of renderer.cxx
    
