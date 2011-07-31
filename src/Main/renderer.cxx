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

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/material/matlib.hxx>
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

#include "splash.hxx"
#include "renderer.hxx"
#include "main.hxx"
#include "CameraGroup.hxx"
#include "FGEventHandler.hxx"
#include <Main/viewer.hxx>
#include <Main/viewmgr.hxx>

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

class SGHUDAndPanelDrawable : public osg::Drawable {
public:
  SGHUDAndPanelDrawable()
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

    // update the panel subsystem
    if ( globals->get_current_panel() != NULL )
        globals->get_current_panel()->update(state);
    // We don't need a state here - can be safely removed when we can pick
    // correctly
    fgUpdate3DPanels();

    glPopClientAttrib();
    glPopAttrib();

  }

  virtual osg::Object* cloneType() const { return new SGHUDAndPanelDrawable; }
  virtual osg::Object* clone(const osg::CopyOp&) const { return new SGHUDAndPanelDrawable; }
  
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

// Sky structures
SGSky *thesky;

static osg::ref_ptr<osg::FrameStamp> mFrameStamp = new osg::FrameStamp;
static osg::ref_ptr<SGUpdateVisitor> mUpdateVisitor= new SGUpdateVisitor;

static osg::ref_ptr<osg::Group> mRealRoot = new osg::Group;

static osg::ref_ptr<osg::Group> mRoot = new osg::Group;

FGRenderer::FGRenderer()
{
#ifdef FG_JPEG_SERVER
   jpgRenderFrame = FGRenderer::update;
#endif
   eventHandler = new FGEventHandler;
}

FGRenderer::~FGRenderer()
{
#ifdef FG_JPEG_SERVER
   jpgRenderFrame = NULL;
#endif
}

// Initialize various GL/view parameters
// XXX This should be called "preinit" or something, as it initializes
// critical parts of the scene graph in addition to the splash screen.
void
FGRenderer::splashinit( void ) {
    osgViewer::Viewer* viewer = globals->get_renderer()->getViewer();
    mRealRoot = dynamic_cast<osg::Group*>(viewer->getSceneData());
    mRealRoot->addChild(fgCreateSplashNode());
    mFrameStamp = viewer->getFrameStamp();
    // Scene doesn't seem to pass the frame stamp to the update
    // visitor automatically.
    mUpdateVisitor->setFrameStamp(mFrameStamp.get());
    viewer->setUpdateVisitor(mUpdateVisitor.get());
    fgSetDouble("/sim/startup/splash-alpha", 1.0);
}

void
FGRenderer::init( void )
{
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
    skyGroup->addChild(thesky->getPreRoot());
    sunLight->addChild(skyGroup);
    mRoot->addChild(sceneGroup);
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
        geode->addDrawable(new SGHUDAndPanelDrawable);
        guiCamera->addChild(geode);
    }
    osg::Switch* sw = new osg::Switch;
    sw->setUpdateCallback(new FGScenerySwitchCallback);
    sw->addChild(mRoot.get());
    mRealRoot->addChild(sw);
    // The clouds are attached directly to the scene graph root
    // because, in theory, they don't want the same default state set
    // as the rest of the scene. This may not be true in practice.
    mRealRoot->addChild(thesky->getCloudRoot());
    mRealRoot->addChild(FGCreateRedoutNode());
    // Attach empty program to the scene root so that shader programs
    // don't leak into state sets (effects) that shouldn't have one.
    stateSet = mRealRoot->getOrCreateStateSet();
    stateSet->setAttributeAndModes(new osg::Program, osg::StateAttribute::ON);
}

void
FGRenderer::update()
{
    globals->get_renderer()->update(true);
}

// Update all Visuals (redraws anything graphics related)
void
FGRenderer::update( bool refresh_camera_settings ) {
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
        const double fade_time = 0.8;
        const double fade_steps_per_sec = 20;
        double delay_time = SGMiscd::min(fade_time/fade_steps_per_sec,
                                         (SGTimeStamp::now() - _splash_time).toSecs());
        _splash_time = SGTimeStamp::now();
        double sAlpha = _splash_alpha->getDoubleValue();
        sAlpha -= SGMiscd::max(0.0,delay_time/fade_time);
        FGScenerySwitchCallback::scenery_enabled = (sAlpha<1.0);
        _splash_alpha->setDoubleValue(sAlpha);
    }

#if 0 // OSGFIXME
    // OSGFIXME: features no longer available or no longer run-time configurable
    {
        bool use_point_sprites = _point_sprites->getBoolValue();
        bool enhanced_lighting = _enhanced_lighting->getBoolValue();
        bool distance_attenuation = _distance_attenuation->getBoolValue();
    
        SGConfigureDirectionalLights( use_point_sprites, enhanced_lighting,
                                      distance_attenuation );
    }
#endif

    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));

    // update fog params
    double actual_visibility;
    if (_cloud_status->getBoolValue()) {
        actual_visibility = thesky->get_visibility();
    } else {
        actual_visibility = _visibility_m->getDoubleValue();
    }

    // idle_state is now 1000 meaning we've finished all our
    // initializations and are running the main loop, so this will
    // now work without seg faulting the system.

    FGViewer *current__view = globals->get_current_view();
    // Force update of center dependent values ...
    current__view->set_dirty();

    if ( refresh_camera_settings ) {
        // update view port
        resize( _xsize->getIntValue(),
                _ysize->getIntValue() );
    }
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
    thesky->set_visibility(visibility_meters);

    double altitude_m = _altitude_ft->getDoubleValue() * SG_FEET_TO_METER;
    thesky->modify_vis( altitude_m, 0.0 /* time factor, now unused */);

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
        thesky->reposition( sstate, *globals->get_ephem(), delta_time_sec );
        thesky->repaint( scolor, *globals->get_ephem() );

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
}



// options.cxx needs to see this for toggle_panel()
// Handle new window size or exposure
void
FGRenderer::resize( int width, int height ) {

// the following breaks aspect-ratio of the main 3D scenery window when 2D panels are moved
// in y direction - causing issues for aircraft with 2D panels (/sim/virtual_cockpit=false).
// Disabling for now. Seems this useful for the pre-OSG time only.
//    if ( (!_virtual_cockpit->getBoolValue())
//         && fgPanelVisible() && idle_state == 1000 ) {
//        view_h = (int)(height * (globals->get_current_panel()->getViewHeight() -
//                             globals->get_current_panel()->getYOffset()) / 768.0);
//    }

    int curWidth = _xsize->getIntValue(),
        curHeight = _ysize->getIntValue();
    if ((curHeight != height) || (curWidth != width)) {
    // must guard setting these, or PLIB-PUI fails with too many live interfaces
        _xsize->setIntValue(width);
        _ysize->setIntValue(height);
    }
    
    // must set view aspect ratio each frame, or initial values are wrong.
    // should probably be fixed 'smarter' during view setup.
    double aspect = height / (double) width;

    // for all views
    FGViewMgr *viewmgr = globals->get_viewmgr();
    if (viewmgr) {
        for ( int i = 0; i < viewmgr->size(); ++i ) {
            viewmgr->get_view(i)->set_aspect_ratio(aspect);
        }
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
    
