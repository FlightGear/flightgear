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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <osg/ref_ptr>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CameraNode>
#include <osg/CameraView>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/Fog>
#include <osg/Group>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/LightSource>
#include <osg/NodeCallback>
#include <osg/Notify>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/ShadeModel>
#include <osg/TexEnv>

#include <osgUtil/SceneView>
#include <osgUtil/UpdateVisitor>
#include <osgUtil/IntersectVisitor>

#include <osg/io_utils>
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <sstream>

#include <simgear/math/SGMath.hxx>
#include <simgear/screen/extensions.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/tgdb/pt_lights.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/sg_random.h>
#ifdef FG_JPEG_SERVER
#include <simgear/screen/jpgfactory.hxx>
#endif

#include <simgear/environment/visual_enviro.hxx>

#include <Scenery/tileentry.hxx>
#include <Time/light.hxx>
#include <Time/light.hxx>
#include <Aircraft/aircraft.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/cockpit.hxx>
#include <Cockpit/hud.hxx>
#include <Model/panelnode.hxx>
#include <Model/modelmgr.hxx>
#include <Model/acmodel.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <ATC/ATCdisplay.hxx>
#include <GUI/new_gui.hxx>
#include <Instrumentation/instrument_mgr.hxx>
#include <Instrumentation/HUD/HUD.hxx>

#include "splash.hxx"
#include "renderer.hxx"
#include "main.hxx"

class SGPuDrawable : public osg::Drawable {
public:
  SGPuDrawable()
  {
    // Dynamic stuff, do not store geometry
    setUseDisplayList(false);

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
  virtual void drawImplementation(osg::State& state) const
  {
    state.pushStateSet(getStateSet());
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);

    if((fgGetBool("/sim/atc/enabled"))
       || (fgGetBool("/sim/ai-traffic/enabled")))
      globals->get_ATC_display()->update(delta_time_sec, state);

    puDisplay();

    // Fade out the splash screen over the first three seconds.
    double t = globals->get_sim_time_sec();
    if (t <= 2.5) {
      glPushAttrib(GL_ALL_ATTRIB_BITS);
      glPushClientAttrib(~0u);

      fgSplashUpdate((2.5 - t) / 2.5);

      glPopClientAttrib();
      glPopAttrib();
    }

    state.popStateSet();
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
  virtual void drawImplementation(osg::State& state) const
  {
    state.pushStateSet(getStateSet());
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(~0u);

    fgCockpitUpdate(&state);

    FGInstrumentMgr *instr = static_cast<FGInstrumentMgr*>(globals->get_subsystem("instrumentation"));
    HUD *hud = static_cast<HUD*>(instr->get_subsystem("hud"));
    hud->draw(state);

    // update the panel subsystem
    if ( globals->get_current_panel() != NULL )
        globals->get_current_panel()->update(state);
    // We don't need a state here - can be safely removed when we can pick
    // correctly
    fgUpdate3DPanels();

    glPopClientAttrib();
    glPopAttrib();

    state.popStateSet();
  }

  virtual osg::Object* cloneType() const { return new SGHUDAndPanelDrawable; }
  virtual osg::Object* clone(const osg::CopyOp&) const { return new SGHUDAndPanelDrawable; }
  
private:
};

class FGLightSourceUpdateCallback : public osg::NodeCallback {
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::LightSource*>(node));
    osg::LightSource* lightSource = static_cast<osg::LightSource*>(node);
    osg::Light* light = lightSource->getLight();
    
    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));
    light->setAmbient(l->scene_ambient().osg());
    light->setDiffuse(l->scene_diffuse().osg());
    light->setSpecular(l->scene_specular().osg());
    SGVec4f position(l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2], 0);
    light->setPosition(position.osg());
    SGVec3f direction(l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2]);
    light->setDirection(direction.osg());
    light->setSpotExponent(0);
    light->setSpotCutoff(180);
    light->setConstantAttenuation(1);
    light->setLinearAttenuation(0);
    light->setQuadraticAttenuation(0);

    traverse(node, nv);
  }
};

class FGWireFrameModeUpdateCallback : public osg::StateAttribute::Callback {
public:
  FGWireFrameModeUpdateCallback() :
    mWireframe(fgGetNode("/sim/rendering/wireframe"))
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
  SGSharedPtr<SGPropertyNode> mWireframe;
};

class FGLightModelUpdateCallback : public osg::StateAttribute::Callback {
public:
  FGLightModelUpdateCallback() :
    mHighlights(fgGetNode("/sim/rendering/specular-highlight"))
  { }
  virtual void operator()(osg::StateAttribute* stateAttribute,
                          osg::NodeVisitor*)
  {
    assert(dynamic_cast<osg::LightModel*>(stateAttribute));
    osg::LightModel* lightModel;
    lightModel = static_cast<osg::LightModel*>(stateAttribute);

#if 0
    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));
    lightModel->setAmbientIntensity(l->scene_ambient().osg());
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
  SGSharedPtr<SGPropertyNode> mHighlights;
};

class FGFogEnableUpdateCallback : public osg::StateSet::Callback {
public:
  FGFogEnableUpdateCallback() :
    mFogEnabled(fgGetNode("/sim/rendering/fog"))
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
  SGSharedPtr<SGPropertyNode> mFogEnabled;
};

// fog constants.  I'm a little nervous about putting actual code out
// here but it seems to work (?)
static const double m_log01 = -log( 0.01 );
static const double sqrt_m_log01 = sqrt( m_log01 );
static GLfloat fog_exp_density;
static GLfloat fog_exp2_density;
static GLfloat rwy_exp2_punch_through;
static GLfloat taxi_exp2_punch_through;
static GLfloat ground_exp2_punch_through;

// Sky structures
SGSky *thesky;

static osg::ref_ptr<osgUtil::SceneView> sceneView = new osgUtil::SceneView;
static osg::ref_ptr<osg::FrameStamp> mFrameStamp = new osg::FrameStamp;
static osg::ref_ptr<SGUpdateVisitor> mUpdateVisitor= new SGUpdateVisitor;

static osg::ref_ptr<osg::Group> mRoot = new osg::Group;

static osg::ref_ptr<osg::CameraView> mCameraView = new osg::CameraView;
static osg::ref_ptr<osg::CameraNode> mBackGroundCamera = new osg::CameraNode;
osg::ref_ptr<osg::CameraNode> mSceneCamera = new osg::CameraNode;

static osg::ref_ptr<osg::Fog> mFog = new osg::Fog;
static osg::ref_ptr<osg::Fog> mRunwayLightingFog = new osg::Fog;
static osg::ref_ptr<osg::Fog> mTaxiLightingFog = new osg::Fog;
static osg::ref_ptr<osg::Fog> mGroundLightingFog = new osg::Fog;

FGRenderer::FGRenderer()
{
#ifdef FG_JPEG_SERVER
   jpgRenderFrame = FGRenderer::update;
#endif
}

FGRenderer::~FGRenderer()
{
#ifdef FG_JPEG_SERVER
   jpgRenderFrame = NULL;
#endif
}

// Initialize various GL/view parameters
void
FGRenderer::init( void ) {

    osg::initNotifyLevel();

    // The number of polygon-offset "units" to place between layers.  In
    // principle, one is supposed to be enough.  In practice, I find that
    // my hardware/driver requires many more.
    osg::PolygonOffset::setUnitsMultiplier(1);
    osg::PolygonOffset::setFactorMultiplier(1);

    // Go full screen if requested ...
    if ( fgGetBool("/sim/startup/fullscreen") )
        fgOSFullScreen();

    if ( (!strcmp(fgGetString("/sim/rendering/fog"), "disabled")) || 
         (!fgGetBool("/sim/rendering/shading"))) {
        // if fastest fog requested, or if flat shading force fastest
        glHint ( GL_FOG_HINT, GL_FASTEST );
    } else if ( !strcmp(fgGetString("/sim/rendering/fog"), "nicest") ) {
        glHint ( GL_FOG_HINT, GL_DONT_CARE );
    }

    glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);

    sceneView->setDefaults(osgUtil::SceneView::COMPILE_GLOBJECTS_AT_INIT);

    mFog->setMode(osg::Fog::EXP2);
    mRunwayLightingFog->setMode(osg::Fog::EXP2);
    mTaxiLightingFog->setMode(osg::Fog::EXP2);
    mGroundLightingFog->setMode(osg::Fog::EXP2);

    sceneView->setFrameStamp(mFrameStamp.get());

    mUpdateVisitor = new SGUpdateVisitor;
    sceneView->setUpdateVisitor(mUpdateVisitor.get());

    sceneView->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    sceneView->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    osg::StateSet* stateSet = mRoot->getOrCreateStateSet();

    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    stateSet->setAttribute(new osg::Depth(osg::Depth::LEQUAL));
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


//     stateSet->setAttribute(new osg::CullFace(osg::CullFace::BACK));
//     stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);


    // this is the topmost scenegraph node for osg
    mBackGroundCamera->addChild(thesky->getPreRoot());
    mBackGroundCamera->setClearMask(0);

    GLbitfield inheritanceMask = osg::CullSettings::ALL_VARIABLES;
    inheritanceMask &= ~osg::CullSettings::COMPUTE_NEAR_FAR_MODE;
    inheritanceMask &= ~osg::CullSettings::NEAR_FAR_RATIO;
    inheritanceMask &= ~osg::CullSettings::CULLING_MODE;
    mBackGroundCamera->setInheritanceMask(inheritanceMask);
    mBackGroundCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    mBackGroundCamera->setCullingMode(osg::CullSettings::NO_CULLING);
    mBackGroundCamera->setRenderOrder(osg::CameraNode::NESTED_RENDER);

    stateSet = mBackGroundCamera->getOrCreateStateSet();
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);


    mSceneCamera->setClearMask(0);
    inheritanceMask = osg::CullSettings::ALL_VARIABLES;
    inheritanceMask &= ~osg::CullSettings::COMPUTE_NEAR_FAR_MODE;
    inheritanceMask &= ~osg::CullSettings::CULLING_MODE;
    mSceneCamera->setInheritanceMask(inheritanceMask);
    mSceneCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    mSceneCamera->setCullingMode(osg::CullSettings::DEFAULT_CULLING);
    mSceneCamera->setRenderOrder(osg::CameraNode::NESTED_RENDER);
    mSceneCamera->addChild(globals->get_scenery()->get_scene_graph());
    mSceneCamera->addChild(thesky->getCloudRoot());

    stateSet = mSceneCamera->getOrCreateStateSet();
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

    // need to update the light on every frame
    osg::LightSource* lightSource = new osg::LightSource;
    lightSource->setUpdateCallback(new FGLightSourceUpdateCallback);
    // relative because of CameraView being just a clever transform node
    lightSource->setReferenceFrame(osg::LightSource::RELATIVE_RF);
    lightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    mRoot->addChild(lightSource);

    lightSource->addChild(mBackGroundCamera.get());
    lightSource->addChild(mSceneCamera.get());


    stateSet = globals->get_scenery()->get_scene_graph()->getOrCreateStateSet();
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
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
    stateSet->setAttributeAndModes(mFog.get());
    stateSet->setUpdateCallback(new FGFogEnableUpdateCallback);

    // plug in the GUI
    osg::CameraNode* guiCamera = new osg::CameraNode;
    guiCamera->setRenderOrder(osg::CameraNode::POST_RENDER);
    guiCamera->setClearMask(0);
    inheritanceMask = osg::CullSettings::ALL_VARIABLES;
    inheritanceMask &= ~osg::CullSettings::COMPUTE_NEAR_FAR_MODE;
    inheritanceMask &= ~osg::CullSettings::CULLING_MODE;
    guiCamera->setInheritanceMask(inheritanceMask);
    guiCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    guiCamera->setCullingMode(osg::CullSettings::NO_CULLING);
    mRoot->addChild(guiCamera);
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(new SGPuDrawable);
    geode->addDrawable(new SGHUDAndPanelDrawable);
    guiCamera->addChild(geode);

    // this one contains all lights, here we set the light states we did
    // in the plib case with plain OpenGL
    osg::Group* lightGroup = new osg::Group;
    mSceneCamera->addChild(lightGroup);
    lightGroup->addChild(globals->get_scenery()->get_gnd_lights_root());
    lightGroup->addChild(globals->get_scenery()->get_vasi_lights_root());
    lightGroup->addChild(globals->get_scenery()->get_rwy_lights_root());
    lightGroup->addChild(globals->get_scenery()->get_taxi_lights_root());

    stateSet = globals->get_scenery()->get_gnd_lights_root()->getOrCreateStateSet();
    stateSet->setAttributeAndModes(mFog.get());
    stateSet->setUpdateCallback(new FGFogEnableUpdateCallback);
    stateSet = globals->get_scenery()->get_vasi_lights_root()->getOrCreateStateSet();
    stateSet->setAttributeAndModes(mRunwayLightingFog.get());
    stateSet->setUpdateCallback(new FGFogEnableUpdateCallback);
    stateSet = globals->get_scenery()->get_rwy_lights_root()->getOrCreateStateSet();
    stateSet->setAttributeAndModes(mRunwayLightingFog.get());
    stateSet->setUpdateCallback(new FGFogEnableUpdateCallback);
    stateSet = globals->get_scenery()->get_taxi_lights_root()->getOrCreateStateSet();
    stateSet->setAttributeAndModes(mTaxiLightingFog.get());
    stateSet->setUpdateCallback(new FGFogEnableUpdateCallback);

    mCameraView->addChild(mRoot.get());
    sceneView->setSceneData(mCameraView.get());

//  sceneView->getState()->setCheckForGLErrors(osg::State::ONCE_PER_ATTRIBUTE);
}


// Update all Visuals (redraws anything graphics related)
void
FGRenderer::update( bool refresh_camera_settings ) {
    bool scenery_loaded = fgGetBool("sim/sceneryloaded")
                          || fgGetBool("sim/sceneryloaded-override");

    if ( idle_state < 1000 || !scenery_loaded ) {
        if (sceneView.valid() && sceneView->getState()) {
            sceneView->getState()->setActiveTextureUnit(0);
            sceneView->getState()->setClientActiveTextureUnit(0);
        }
        // still initializing, draw the splash screen
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushClientAttrib(~0u);

        fgSplashUpdate(1.0);

        glPopClientAttrib();
        glPopAttrib();

        // Keep resetting sim time while the sim is initializing
        globals->set_sim_time_sec( 0.0 );
        return;
    }

    bool skyblend = fgGetBool("/sim/rendering/skyblend");
    bool use_point_sprites = fgGetBool("/sim/rendering/point-sprites");
    bool enhanced_lighting = fgGetBool("/sim/rendering/enhanced-lighting");
    bool distance_attenuation
        = fgGetBool("/sim/rendering/distance-attenuation");
    // OSGFIXME
    SGConfigureDirectionalLights( use_point_sprites, enhanced_lighting,
                                  distance_attenuation );

    static const SGPropertyNode *groundlevel_nearplane
        = fgGetNode("/sim/current-view/ground-level-nearplane-m");

    FGLight *l = static_cast<FGLight*>(globals->get_subsystem("lighting"));

    // update fog params
    double actual_visibility;
    if (fgGetBool("/environment/clouds/status")) {
        actual_visibility = thesky->get_visibility();
    } else {
        actual_visibility = fgGetDouble("/environment/visibility-m");
    }

    static double last_visibility = -9999;
    if ( actual_visibility != last_visibility ) {
        last_visibility = actual_visibility;

        fog_exp_density = m_log01 / actual_visibility;
        fog_exp2_density = sqrt_m_log01 / actual_visibility;
        ground_exp2_punch_through = sqrt_m_log01 / (actual_visibility * 1.5);
        if ( actual_visibility < 8000 ) {
            rwy_exp2_punch_through = sqrt_m_log01 / (actual_visibility * 2.5);
            taxi_exp2_punch_through = sqrt_m_log01 / (actual_visibility * 1.5);
        } else {
            rwy_exp2_punch_through = sqrt_m_log01 / ( 8000 * 2.5 );
            taxi_exp2_punch_through = sqrt_m_log01 / ( 8000 * 1.5 );
        }
        mFog->setDensity(fog_exp2_density);
        mRunwayLightingFog->setDensity(rwy_exp2_punch_through);
        mTaxiLightingFog->setDensity(taxi_exp2_punch_through);
        mGroundLightingFog->setDensity(ground_exp2_punch_through);
    }

    // idle_state is now 1000 meaning we've finished all our
    // initializations and are running the main loop, so this will
    // now work without seg faulting the system.

    FGViewer *current__view = globals->get_current_view();
    // Force update of center dependent values ...
    current__view->set_dirty();

    if ( refresh_camera_settings ) {
        // update view port
        resize( fgGetInt("/sim/startup/xsize"),
                fgGetInt("/sim/startup/ysize") );

        SGVec3d center = globals->get_scenery()->get_center();
        SGVec3d position = current__view->getViewPosition();
        SGQuatd attitude = current__view->getViewOrientation();
        SGVec3d osgPosition = attitude.transform(center - position);
        mCameraView->setPosition(osgPosition.osg());
        mCameraView->setAttitude(inverse(attitude).osg());
    }

    if ( skyblend ) {
        if ( fgGetBool("/sim/rendering/textures") ) {
            SGVec4f clearColor(l->adj_fog_color());
            sceneView->getCamera()->setClearColor(clearColor.osg());
        }
    } else {
        SGVec4f clearColor(l->sky_color());
        sceneView->getCamera()->setClearColor(clearColor.osg());
    }

    // update fog params if visibility has changed
    double visibility_meters = fgGetDouble("/environment/visibility-m");
    thesky->set_visibility(visibility_meters);

    thesky->modify_vis( cur_fdm_state->get_Altitude() * SG_FEET_TO_METER,
                        ( global_multi_loop * fgGetInt("/sim/speed-up") )
                        / (double)fgGetInt("/sim/model-hz") );

    // update the sky dome
    if ( skyblend ) {

        // The sun and moon distances are scaled down versions
        // of the actual distance to get both the moon and the sun
        // within the range of the far clip plane.
        // Moon distance:    384,467 kilometers
        // Sun distance: 150,000,000 kilometers

        double sun_horiz_eff, moon_horiz_eff;
        if (fgGetBool("/sim/rendering/horizon-effect")) {
           sun_horiz_eff = 0.67+pow(0.5+cos(l->get_sun_angle())*2/2, 0.33)/3;
           moon_horiz_eff = 0.67+pow(0.5+cos(l->get_moon_angle())*2/2, 0.33)/3;
        } else {
           sun_horiz_eff = moon_horiz_eff = 1.0;
        }

        static SGSkyState sstate;

        sstate.view_pos  = current__view->get_view_pos();
        sstate.zero_elev = current__view->get_zero_elev();
        sstate.view_up   = current__view->get_world_up();
        sstate.lon       = current__view->getLongitude_deg()
                            * SGD_DEGREES_TO_RADIANS;
        sstate.lat       = current__view->getLatitude_deg()
                            * SGD_DEGREES_TO_RADIANS;
        sstate.alt       = current__view->getAltitudeASL_ft()
                            * SG_FEET_TO_METER;
        sstate.spin      = l->get_sun_rotation();
        sstate.gst       = globals->get_time_params()->getGst();
        sstate.sun_ra    = globals->get_ephem()->getSunRightAscension();
        sstate.sun_dec   = globals->get_ephem()->getSunDeclination();
        sstate.sun_dist  = 50000.0 * sun_horiz_eff;
        sstate.moon_ra   = globals->get_ephem()->getMoonRightAscension();
        sstate.moon_dec  = globals->get_ephem()->getMoonDeclination();
        sstate.moon_dist = 40000.0 * moon_horiz_eff;
        sstate.sun_angle = l->get_sun_angle();


        /*
         SG_LOG( SG_GENERAL, SG_BULK, "thesky->repaint() sky_color = "
         << l->sky_color()[0] << " "
         << l->sky_color()[1] << " "
         << l->sky_color()[2] << " "
         << l->sky_color()[3] );
        SG_LOG( SG_GENERAL, SG_BULK, "    fog = "
         << l->fog_color()[0] << " "
         << l->fog_color()[1] << " "
         << l->fog_color()[2] << " "
         << l->fog_color()[3] );
        SG_LOG( SG_GENERAL, SG_BULK,
                "    sun_angle = " << l->sun_angle
         << "    moon_angle = " << l->moon_angle );
        */

        static SGSkyColor scolor;

        scolor.sky_color   = SGVec3f(l->sky_color().data());
        scolor.fog_color   = SGVec3f(l->adj_fog_color().data());
        scolor.cloud_color = SGVec3f(l->cloud_color().data());
        scolor.sun_angle   = l->get_sun_angle();
        scolor.moon_angle  = l->get_moon_angle();
        scolor.nplanets    = globals->get_ephem()->getNumPlanets();
        scolor.nstars      = globals->get_ephem()->getNumStars();
        scolor.planet_data = globals->get_ephem()->getPlanets();
        scolor.star_data   = globals->get_ephem()->getStars();

        thesky->reposition( sstate, delta_time_sec );
        thesky->repaint( scolor );

        /*
        SG_LOG( SG_GENERAL, SG_BULK,
                "thesky->reposition( view_pos = " << view_pos[0] << " "
         << view_pos[1] << " " << view_pos[2] );
        SG_LOG( SG_GENERAL, SG_BULK,
                "    zero_elev = " << zero_elev[0] << " "
         << zero_elev[1] << " " << zero_elev[2]
         << " lon = " << cur_fdm_state->get_Longitude()
         << " lat = " << cur_fdm_state->get_Latitude() );
        SG_LOG( SG_GENERAL, SG_BULK,
                "    sun_rot = " << l->get_sun_rotation
         << " gst = " << SGTime::cur_time_params->getGst() );
        SG_LOG( SG_GENERAL, SG_BULK,
             "    sun ra = " << globals->get_ephem()->getSunRightAscension()
          << " sun dec = " << globals->get_ephem()->getSunDeclination()
          << " moon ra = " << globals->get_ephem()->getMoonRightAscension()
          << " moon dec = " << globals->get_ephem()->getMoonDeclination() );
        */

        //OSGFIXME
//         shadows->setupShadows(
//           current__view->getLongitude_deg(),
//           current__view->getLatitude_deg(),
//           globals->get_time_params()->getGst(),
//           globals->get_ephem()->getSunRightAscension(),
//           globals->get_ephem()->getSunDeclination(),
//           l->get_sun_angle());

    }

    if ( strcmp(fgGetString("/sim/rendering/fog"), "disabled") ) {
        SGVec4f color(l->adj_fog_color());
        mFog->setColor(color.osg());
        mRunwayLightingFog->setColor(color.osg());
        mTaxiLightingFog->setColor(color.osg());
        mGroundLightingFog->setColor(color.osg());
    }


//     sgEnviro.setLight(l->adj_fog_color());

    // texture parameters
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

    double agl = current__view->getAltitudeASL_ft()*SG_FEET_TO_METER
      - current__view->getSGLocation()->get_cur_elev_m();

    float scene_nearplane, scene_farplane;
    if ( agl > 10.0 ) {
        scene_nearplane = 10.0f;
        scene_farplane = 120000.0f;
    } else {
        scene_nearplane = groundlevel_nearplane->getDoubleValue();
        scene_farplane = 120000.0f;
    }

    setNearFar( scene_nearplane, scene_farplane );

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
       // TODO:find the real view speed, not the AC one
//     sgEnviro.drawPrecipitation(
//         fgGetDouble("/environment/metar/rain-norm", 0.0),
//         fgGetDouble("/environment/metar/snow-norm", 0.0),
//         fgGetDouble("/environment/metar/hail-norm", 0.0),
//         current__view->getPitch_deg() + current__view->getPitchOffset_deg(),
//         current__view->getRoll_deg() + current__view->getRollOffset_deg(),
//         - current__view->getHeadingOffset_deg(),
//                current_view_origin_airspeed_horiz_kt
//                );

    // OSGFIXME
//     if( is_internal )
//         shadows->endOfFrame();

    // need to call the update visitor once
    globals->get_aircraft_model()->select( true );
    FGTileMgr::set_tile_filter( true );
    mFrameStamp->setReferenceTime(globals->get_sim_time_sec());
    mFrameStamp->setFrameNumber(1+mFrameStamp->getFrameNumber());
    mFrameStamp->setCalendarTime(*globals->get_time_params()->getGmt());
    mUpdateVisitor->setViewData(current__view->getViewPosition(),
                                current__view->getViewOrientation());
    mUpdateVisitor->setSceneryCenter(globals->get_scenery()->get_center());
    SGVec3f direction(l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2]);
    mUpdateVisitor->setLight(direction, l->scene_ambient(),
                             l->scene_diffuse(), l->scene_specular());
    mUpdateVisitor->setVisibility(actual_visibility);

    if (fgGetBool("/sim/panel-hotspots"))
      sceneView->setCullMask(~0u);
    else
      sceneView->setCullMask(~SG_NODEMASK_PICK_BIT);

    sceneView->update();
    sceneView->cull();
    sceneView->draw();
}



// options.cxx needs to see this for toggle_panel()
// Handle new window size or exposure
void
FGRenderer::resize( int width, int height ) {
    int view_h;

    if ( (!fgGetBool("/sim/virtual-cockpit"))
         && fgPanelVisible() && idle_state == 1000 ) {
        view_h = (int)(height * (globals->get_current_panel()->getViewHeight() -
                             globals->get_current_panel()->getYOffset()) / 768.0);
    } else {
        view_h = height;
    }

    sceneView->getViewport()->setViewport(0, height - view_h, width, view_h);

    static int lastwidth = 0;
    static int lastheight = 0;
    if (width != lastwidth)
        fgSetInt("/sim/startup/xsize", lastwidth = width);
    if (height != lastheight)
        fgSetInt("/sim/startup/ysize", lastheight = height);

    guiInitMouse(width, height);

    // for all views
    FGViewMgr *viewmgr = globals->get_viewmgr();
    if (viewmgr) {
      for ( int i = 0; i < viewmgr->size(); ++i ) {
        viewmgr->get_view(i)->
          set_aspect_ratio((float)view_h / (float)width);
      }

      setFOV( viewmgr->get_current_view()->get_h_fov(),
              viewmgr->get_current_view()->get_v_fov() );
    }
}


// These are wrapper functions around ssgSetNearFar() and ssgSetFOV()
// which will post process and rewrite the resulting frustum if we
// want to do asymmetric view frustums.

static void fgHackFrustum() {

    // specify a percent of the configured view frustum to actually
    // display.  This is a bit of a hack to achieve asymmetric view
    // frustums.  For instance, if you want to display two monitors
    // side by side, you could specify each with a double fov, a 0.5
    // aspect ratio multiplier, and then the left side monitor would
    // have a left_pct = 0.0, a right_pct = 0.5, a bottom_pct = 0.0,
    // and a top_pct = 1.0.  The right side monitor would have a
    // left_pct = 0.5 and a right_pct = 1.0.

    static SGPropertyNode *left_pct
        = fgGetNode("/sim/current-view/frustum-left-pct");
    static SGPropertyNode *right_pct
        = fgGetNode("/sim/current-view/frustum-right-pct");
    static SGPropertyNode *bottom_pct
        = fgGetNode("/sim/current-view/frustum-bottom-pct");
    static SGPropertyNode *top_pct
        = fgGetNode("/sim/current-view/frustum-top-pct");

    double left, right;
    double bottom, top;
    double zNear, zFar;
    sceneView->getProjectionMatrixAsFrustum(left, right, bottom, top, zNear, zFar);
    // cout << " l = " << f->getLeft()
    //      << " r = " << f->getRight()
    //      << " b = " << f->getBot()
    //      << " t = " << f->getTop()
    //      << " n = " << f->getNear()
    //      << " f = " << f->getFar()
    //      << endl;

    double width = right - left;
    double height = top - bottom;

    double l, r, t, b;

    if ( left_pct != NULL ) {
        l = left + width * left_pct->getDoubleValue();
    } else {
        l = left;
    }

    if ( right_pct != NULL ) {
        r = left + width * right_pct->getDoubleValue();
    } else {
        r = right;
    }

    if ( bottom_pct != NULL ) {
        b = bottom + height * bottom_pct->getDoubleValue();
    } else {
        b = bottom;
    }

    if ( top_pct != NULL ) {
        t = bottom + height * top_pct->getDoubleValue();
    } else {
        t = top;
    }

    sceneView->setProjectionMatrixAsFrustum(l, r, b, t, zNear, zFar);
}


// we need some static storage space for these values.  However, we
// can't store it in a renderer class object because the functions
// that manipulate these are static.  They are static so they can
// interface to the display callback system.  There's probably a
// better way, there has to be a better way, but I'm not seeing it
// right now.
static float fov_width = 55.0;
static float fov_height = 42.0;
static float fov_near = 1.0;
static float fov_far = 1000.0;


/** FlightGear code should use this routine to set the FOV rather than
 *  calling the ssg routine directly
 */
void FGRenderer::setFOV( float w, float h ) {
    fov_width = w;
    fov_height = h;

    sceneView->setProjectionMatrixAsPerspective(fov_height,
                                                fov_width/fov_height,
                                                fov_near, fov_far);
    // fully specify the view frustum before hacking it (so we don't
    // accumulate hacked effects
    fgHackFrustum();
//     sgEnviro.setFOV( w, h );
}


/** FlightGear code should use this routine to set the Near/Far clip
 *  planes rather than calling the ssg routine directly
 */
void FGRenderer::setNearFar( float n, float f ) {
// OSGFIXME: we have currently too much z-buffer fights
n = 0.2;
    fov_near = n;
    fov_far = f;

    sceneView->setProjectionMatrixAsPerspective(fov_height,
                                                fov_width/fov_height,
                                                fov_near, fov_far);

    sceneView->getCamera()->setNearFarRatio(fov_near/fov_far);
    mSceneCamera->setNearFarRatio(fov_near/fov_far);

    // fully specify the view frustum before hacking it (so we don't
    // accumulate hacked effects
    fgHackFrustum();
}

bool
FGRenderer::pick( unsigned x, unsigned y,
                  std::vector<SGSceneryPick>& pickList )
{
  // wipe out the return ...
  pickList.resize(0);

  // we can get called early ...
  if (!sceneView.valid())
    return false;

  osg::Node* sceneData = globals->get_scenery()->get_scene_graph();
  if (!sceneData)
    return false;
  osg::Viewport* viewport = sceneView->getViewport();
  if (!viewport)
    return false;

  // good old scenery center
  SGVec3d center = globals->get_scenery()->get_center();

  // don't know why, but the update has partly happened somehow,
  // so update the scenery part of the viewer
  FGViewer *current_view = globals->get_current_view();
  // Force update of center dependent values ...
  current_view->set_dirty();
  SGVec3d position = current_view->getViewPosition();
  SGQuatd attitude = current_view->getViewOrientation();
  SGVec3d osgPosition = attitude.transform(center - position);
  mCameraView->setPosition(osgPosition.osg());
  mCameraView->setAttitude(inverse(attitude).osg());

  osg::Matrix projection(sceneView->getProjectionMatrix());
  osg::Matrix modelview(sceneView->getViewMatrix());

  osg::NodePathList nodePath = sceneData->getParentalNodePaths();
  // modify the view matrix so that it accounts for this nodePath's
  // accumulated transform
  if (!nodePath.empty())
    modelview.preMult(computeLocalToWorld(nodePath.front()));

  // swap the y values ...
  y = viewport->height() - y;
  // set up the pick visitor
  osgUtil::PickVisitor pickVisitor(viewport, projection, modelview, x, y);
  sceneData->accept(pickVisitor);
  if (!pickVisitor.hits())
    return false;

  // collect all interaction callbacks on the pick ray.
  // They get stored in the pickCallbacks list where they are sorted back
  // to front and croasest to finest wrt the scenery node they are attached to
  osgUtil::PickVisitor::LineSegmentHitListMap::const_iterator mi;
  for (mi = pickVisitor.getSegHitList().begin();
       mi != pickVisitor.getSegHitList().end();
       ++mi) {
    osgUtil::IntersectVisitor::HitList::const_iterator hi;
    for (hi = mi->second.begin(); hi != mi->second.end(); ++hi) {
      // ok, go back the nodes and ask for intersection callbacks,
      // execute them in top down order
      const osg::NodePath& np = hi->getNodePath();
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
          /// note that this is done totally in doubles instead of
          /// just using getWorldIntersectionPoint
          osg::Vec3d localPt = hi->getLocalIntersectPoint();
          sceneryPick.info.local = SGVec3d(localPt);
          if (hi->getMatrix())
            sceneryPick.info.wgs84 = SGVec3d(localPt*(*hi->getMatrix()));
          else
            sceneryPick.info.wgs84 = SGVec3d(localPt);
          sceneryPick.info.wgs84 += globals->get_scenery()->get_center();
          sceneryPick.callback = pickCallback;
          pickList.push_back(sceneryPick);
        }
      }
    }
  }

  return !pickList.empty();
}

// end of renderer.cxx
    
