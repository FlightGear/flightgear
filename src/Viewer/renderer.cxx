/*
 * SPDX-FileName: renderer.cxx
 * SPDX-FileComment: Written by Curtis Olson, started May 1997.
 * SPDX-FileCopyrightText: Copyright (C) 1997 - 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>
#include <simgear/compiler.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
#include <typeinfo>

#include <osg/Camera>
#include <osg/CullFace>
#include <osg/CullStack>
#include <osg/GraphicsContext>
#include <osg/Group>
#include <osg/Hint>
#include <osg/Math>
#include <osg/NodeCallback>
#include <osg/Notify>
#include <osg/PolygonMode>
#include <osg/Program>
#include <osgUtil/LineSegmentIntersector>
#include <osgDB/WriteFile>

#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/scene/tgdb/GroundLightManager.hxx>
#include <simgear/scene/tgdb/pt_lights.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/util/OsgUtils.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/sentryIntegration.hxx>
#include <Model/modelmgr.hxx>
#include <Model/acmodel.hxx>
#include <Scenery/scenery.hxx>
#include <GUI/new_gui.hxx>
#include <GUI/gui.h>
#include <GUI/Highlight.hxx>
#include <Time/light.hxx>

#include <Environment/precipitation_mgr.hxx>
#include <Environment/environment_mgr.hxx>
#include <Environment/ephemeris.hxx>

#include "CameraGroup.hxx"
#include "FGEventHandler.hxx"
#include "splash.hxx"
#include "view.hxx"
#include "viewmgr.hxx"
#include "WindowSystemAdapter.hxx"

#include "renderer.hxx"

#if defined(ENABLE_QQ_UI)
#include <GUI/QQuickDrawable.hxx>
#endif

using namespace flightgear;

// Operation for querying OpenGL parameters. This must be done in a
// valid OpenGL context, potentially in another thread.
class QueryGLParametersOperation : public GraphicsContextOperation {
public:
    QueryGLParametersOperation() : GraphicsContextOperation(std::string("Query OpenGL Parameters"))
    {
    }

    void run(osg::GraphicsContext* gc)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);

        SGPropertyNode* p_rendering = fgGetNode("/sim/rendering/gl-info", true);
        auto query_gl_string =
            [p_rendering](const std::string& prop_name, GLenum name) {
                const char* value = (const char*)glGetString(name);
                // glGetString may return a null string
                std::string str(value ? value : "");
                p_rendering->setStringValue(prop_name, str);
                SG_LOG(SG_GL, SG_INFO, "  " << prop_name << ": " << str);
            };

        auto query_gl_int =
            [p_rendering](const std::string& prop_name, GLenum name) {
                GLint value = 0;
                glGetIntegerv(name, &value);
                p_rendering->setIntValue(prop_name, value);
                SG_LOG(SG_GL, SG_INFO, "  " << prop_name << ": " << value);
            };

        SG_LOG(SG_GL, SG_INFO, "OpenGL context info:");
        query_gl_string("gl-vendor", GL_VENDOR);
        query_gl_string("gl-renderer", GL_RENDERER);
        query_gl_string("gl-version", GL_VERSION);
        query_gl_string("gl-shading-language-version", GL_SHADING_LANGUAGE_VERSION);
        query_gl_int("gl-max-texture-size", GL_MAX_TEXTURE_SIZE);
        query_gl_int("gl-max-texture-units", GL_MAX_TEXTURE_UNITS);
    }

private:
    OpenThreads::Mutex _mutex;
};

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

class FGHintUpdateCallback : public osg::StateAttribute::Callback {
public:
    FGHintUpdateCallback(const char* configNode) :
        mConfigNode(fgGetNode(configNode, true))
    {
    }
    void operator()(osg::StateAttribute* stateAttribute, osg::NodeVisitor*) override
    {
        assert(dynamic_cast<osg::Hint*>(stateAttribute));
        osg::Hint* hint = static_cast<osg::Hint*>(stateAttribute);
        std::string value = mConfigNode->getStringValue();
        if (value.empty())
            hint->setMode(GL_DONT_CARE);
        else if (value == "nicest")
            hint->setMode(GL_NICEST);
        else if (value == "fastest")
            hint->setMode(GL_FASTEST);
        else
            hint->setMode(GL_DONT_CARE);
    }
private:
    SGPropertyNode_ptr mConfigNode;
};

class FGWireFrameModeUpdateCallback : public osg::StateAttribute::Callback {
public:
    FGWireFrameModeUpdateCallback() :
        mWireframe(fgGetNode("/sim/rendering/wireframe", true))
    {
    }
    void operator()(osg::StateAttribute* stateAttribute, osg::NodeVisitor*) override
    {
        assert(dynamic_cast<osg::PolygonMode*>(stateAttribute));
        osg::PolygonMode* polygonMode = static_cast<osg::PolygonMode*>(stateAttribute);
        if (mWireframe->getBoolValue()) {
            polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                                 osg::PolygonMode::LINE);
        } else {
            polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                                 osg::PolygonMode::FILL);
        }
    }
private:
    SGPropertyNode_ptr mWireframe;
};

// update callback for the switch node guarding that splash
class FGScenerySwitchCallback : public osg::NodeCallback {
public:
    void operator()(osg::Node* node, osg::NodeVisitor* nv) override
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

FGRenderer::FGRenderer()
{
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
    if (_sky) {
        delete _sky;
        _sky = nullptr;
    }
}

void
FGRenderer::init()
{
    osg::initNotifyLevel();

    osg::DisplaySettings* display_settings = osg::DisplaySettings::instance();
    assert(display_settings);
    // Don't let OSG do automatic shader generation
    display_settings->setShaderHint(osg::DisplaySettings::SHADER_NONE, false);

    // Create the update visitor
    _update_visitor = new SGUpdateVisitor;

    if (!_event_handler)
        _event_handler = new FGEventHandler;
    _event_handler->setChangeStatsCameraRenderOrder(true);

    sgUserDataInit(globals->get_props());

    if (_composite_viewer) {
        // reinit.
    } else {
        _composite_viewer = new osgViewer::CompositeViewer;
        std::string affinity = fgGetString("/sim/thread-cpu-affinity");
        bool osg_affinity_flag = true;
        if (affinity == "") {}
        else if (affinity == "none") {
            osg_affinity_flag = false;
        }
        else if (affinity == "osg") {
            /* This is handled elsewhere. */
        }
        else {
            SG_LOG(SG_VIEW, SG_ALERT, "Unrecognised value for /sim/thread-cpu-affinity: " << affinity);
        }
        _composite_viewer->setUseConfigureAffinity(osg_affinity_flag);
    }
        
    // https://stackoverflow.com/questions/15207076/openscenegraph-and-multiple-viewers
    _composite_viewer->setReleaseContextAtEndOfFrameHint(false);
    _composite_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    _scenery_loaded     = fgGetNode("/sim/sceneryloaded", true);
    _position_finalized = fgGetNode("/sim/position-finalized", true);
    _panel_hotspots     = fgGetNode("/sim/panel-hotspots", true);

    _sim_delta_sec      = fgGetNode("/sim/time/delta-sec", true);

    _xsize              = fgGetNode("/sim/startup/xsize", true);
    _ysize              = fgGetNode("/sim/startup/ysize", true);
    _xpos               = fgGetNode("/sim/startup/xpos", true);
    _ypos               = fgGetNode("/sim/startup/ypos", true);
    _splash_alpha       = fgGetNode("/sim/startup/splash-alpha", true);

    _altitude_ft        = fgGetNode("/position/altitude-ft", true);

    _cloud_status       = fgGetNode("/environment/clouds/status", true);
    _visibility_m       = fgGetNode("/environment/visibility-m", true);

    // configure the lighting related parameters and add change listeners.
    bool use_point_sprites = fgGetBool("/sim/rendering/point-sprites", true);
    bool distance_attenuation = fgGetBool("/sim/rendering/distance-attenuation", false);
    bool triangles = fgGetBool("/sim/rendering/triangle-directional-lights", true);
    SGConfigureDirectionalLights(use_point_sprites, distance_attenuation, triangles);

    addChangeListener(new PointSpriteListener,  "/sim/rendering/point-sprites");
    addChangeListener(new DistanceAttenuationListener, "/sim/rendering/distance-attenuation");
    addChangeListener(new DirectionalLightsListener, "/sim/rendering/triangle-directional-lights");

    // Setup texture compression
    std::string tc = fgGetString("/sim/rendering/texture-compression");
    if (!tc.empty()) {
        if (tc == "false" || tc == "off" ||
            tc == "0" || tc == "no" ||
            tc == "none"
            ) {
            SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::DoNotUseCompression);
        } else if (tc == "arb") {
            SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::UseARBCompression);
        } else if (tc == "dxt1") {
            SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::UseDXT1Compression);
        } else if (tc == "dxt3") {
            SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::UseDXT3Compression);
        } else if (tc == "dxt5") {
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
}

void
FGRenderer::postinit()
{
    // important that we reset the viewer sceneData here, to ensure the reference
    // time for everything is in sync; otherwise on reset the Viewer and
    // GraphicsWindow clocks are out of sync.
    osgViewer::View* view = getView();
    _scene_root = new osg::Group;
    _scene_root->setName("viewerSceneRoot");
    view->setSceneData(_scene_root);
    view->setDatabasePager(FGScenery::getPagerSingleton());

    // Scene doesn't seem to pass the frame stamp to the update
    // visitor automatically.
    _update_visitor->setFrameStamp(getFrameStamp());
    getViewerBase()->setUpdateVisitor(_update_visitor.get());

    fgSetDouble("/sim/startup/splash-alpha", 1.0);
    // hide the menubar if it overlaps the window, so the splash screen
    // is completely visible. We reset this value when the splash screen
    // is fading out.
    fgSetBool("/sim/menubar/overlap-hide", true);
}

void
FGRenderer::setupView()
{
    // Do not automatically compute near far values
    getView()->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // Attach empty program to the scene root so that shader programs
    // don't leak into state sets (effects) that shouldn't have one.
    _scene_root->getOrCreateStateSet()->setAttributeAndModes(
        new osg::Program, osg::StateAttribute::ON);

    // Specify implementation-specific hints
    // osg::Hint* hint = new osg::Hint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_DONT_CARE);
    // hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/hints/fragment-shader-derivative"));
    // stateSet->setAttribute(hint);
    // hint = new osg::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    // hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/hints/line-smooth"));
    // stateSet->setAttribute(hint);
    // hint = new osg::Hint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
    // hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/hints/polygon-smooth"));
    // stateSet->setAttribute(hint);
    // hint = new osg::Hint(GL_TEXTURE_COMPRESSION_HINT, GL_DONT_CARE);
    // hint->setUpdateCallback(new FGHintUpdateCallback("/sim/rendering/hints/texture-compression"));
    // stateSet->setAttribute(hint);

    // Build the sky
    // The sun and moon radius are scaled down numbers of the actual
    // diameters. This is needed to fit both the sun and the moon
    // within the distance to the far clip plane.
    // 
    // Their distance to the observer is specified in FGRendered::updateSky() and
    // set to 40000 for the Moon semi-mayor axis, 50000 for the mean
    // Earth-Sun distance (1AU). The location of the stars is
    // specified below in the call to sky->build() (80000).
    //
    // Mean Moon radius: 1,737.4 kilometers
    // Moon Semi-mayor axis: 384,399 km
    // => Rendered Moon radius = 1,737.4 / 384,399 * 40000 = 232.5
    //
    // Photosphere Sun radius: 695,700 kilometers
    // 1UA = 149,597,870.700 km
    // => Rendered Sun radius = 695,700/149,597,870.700 * 50000 = 180.8
    //
    auto ephemerisSub = globals->get_subsystem<Ephemeris>();
    osg::ref_ptr<simgear::SGReaderWriterOptions> opt;
    opt = simgear::SGReaderWriterOptions::fromPath(globals->get_fg_root());
    opt->setPropertyNode(globals->get_props());
    _sky->build(80000.0, 80000.0,
                232.5, 180.8,
                *ephemerisSub->data(),
                fgGetNode("/environment", true),
                opt.get());

    // Add the sky to the root
    _scene_root->addChild(_sky->getPreRoot());
    // Add the clouds as well
    _scene_root->addChild(_sky->getCloudRoot());

    // Add the main scenery (including models and aircraft) to the root with
    // a switch to enable/disable it on demand.
    osg::Group* scenery_group = globals->get_scenery()->get_scene_graph();
    scenery_group->setName("Scenery group");
    scenery_group->setNodeMask(~simgear::BACKGROUND_BIT);
    osg::Switch* scenery_switch = new osg::Switch;
    scenery_switch->setName("Scenery switch");
    scenery_switch->setUpdateCallback(new FGScenerySwitchCallback);
    scenery_switch->addChild(scenery_group);
    _scene_root->addChild(scenery_switch);

    // Switch to enable wireframe mode on the scenery group
    osg::PolygonMode* polygonMode = new osg::PolygonMode;
    polygonMode->setUpdateCallback(new FGWireFrameModeUpdateCallback);
    scenery_group->getOrCreateStateSet()->setAttributeAndModes(polygonMode);

    osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
    if (guiCamera) {
#if defined(ENABLE_QQ_UI)
        osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(view);
        if (viewer) {
            std::string rootQMLPath = fgGetString("/sim/gui/qml-root-path");
            auto graphicsWindow = dynamic_cast<osgViewer::GraphicsWindow*>(guiCamera->getGraphicsContext());

            if (!rootQMLPath.empty()) {
                _quickDrawable = new QQuickDrawable;
                _quickDrawable->setup(graphicsWindow, viewer);
                _quickDrawable->setSourcePath(rootQMLPath);

                osg::Geode* qqGeode = new osg::Geode;
                qqGeode->addDrawable(_quickDrawable);
                guiCamera->addChild(qqGeode);
            }
        }
#endif
    }
}

bool
FGRenderer::runInitOperation()
{
    static osg::ref_ptr<QueryGLParametersOperation> genOp;
    static bool didInit = false;

    if (didInit) {
        return true;
    }

    if (!genOp.valid()) {
        genOp = new QueryGLParametersOperation;
        WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
        wsa->windows[0]->gc->add(genOp.get());
        return false; // not ready yet
    } else {
        if (!genOp->isFinished())
            return false;

        genOp = nullptr;
        didInit = true;
        // we're done
        return true;
    }
}

void
FGRenderer::update()
{
    if (!_position_finalized || !_scenery_loaded->getBoolValue()) {
        _splash_alpha->setDoubleValue(1.0);

        if (!_maximum_texture_size) {
            osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
            if (guiCamera) {
                osg::GraphicsContext *gc = guiCamera->getGraphicsContext();
                osg::GLExtensions* gl2ext = gc->getState()->get<osg::GLExtensions>();
                if (gl2ext) {
                    _maximum_texture_size = gl2ext->maxTextureSize;
                    SGSceneFeatures::instance()->setMaxTextureSize(_maximum_texture_size);
                }
            }
        }
        return;
    }

    if (_splash_alpha->getDoubleValue() > 0.0) {
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

    auto l = globals->get_subsystem<FGLight>();

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

    // Update the sky
    updateSky();

    // need to call the update visitor once
    getFrameStamp()->setCalendarTime(*globals->get_time_params()->getGmt());
    _update_visitor->setViewData(current__view->getViewPosition(),
                                current__view->getViewOrientation());

    SGVec3f sundirection(l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2]);
    SGVec3f moondirection(l->moon_vec()[0], l->moon_vec()[1], l->moon_vec()[2]);

    _update_visitor->setLight(sundirection, moondirection,
                             l->get_sun_angle()*SGD_RADIANS_TO_DEGREES);
    _update_visitor->setVisibility(actual_visibility);

    osg::Node::NodeMask cullMask = ~simgear::LIGHTS_BITS & ~simgear::PICK_BIT;
    cullMask |= simgear::GroundLightManager::instance()
        ->getLightNodeMask(_update_visitor.get());
    if (_panel_hotspots->getBoolValue())
        cullMask |= simgear::PICK_BIT;
    CameraGroup::getDefault()->setCameraCullMasks(cullMask);
}

void
FGRenderer::updateSky()
{
    // update fog params if visibility has changed
    double visibility_meters = _visibility_m->getDoubleValue();
    _sky->set_visibility(visibility_meters);

    double altitude_m = _altitude_ft->getDoubleValue() * SG_FEET_TO_METER;
    _sky->modify_vis( altitude_m, 0.0 /* time factor, now unused */);

    auto l = globals->get_subsystem<FGLight>();

    // The sun and moon distances are scaled down versions of the
    // actual distance. See FGRenderer::setupView() for more details.

    SGSkyState sstate;
    sstate.pos       = globals->get_current_view()->getViewPosition();
    sstate.pos_geod  = globals->get_current_view()->getPosition();
    sstate.ori       = globals->get_current_view()->getViewOrientation();
    sstate.spin      = l->get_sun_rotation();
    sstate.gst       = globals->get_time_params()->getGst();
    sstate.sun_dist  = 50000.0;
    sstate.moon_dist_bare = 40000.0;
    sstate.moon_dist_factor = 1.0;
    sstate.sun_angle = l->get_sun_angle();

    SGSkyColor scolor;
    scolor.sun_angle   = l->get_sun_angle();
    scolor.moon_angle  = l->get_moon_angle();
    scolor.altitude_m =  altitude_m;
    
    auto ephemerisSub = globals->get_subsystem<Ephemeris>();
    double delta_time_sec = _sim_delta_sec->getDoubleValue();
    _sky->reposition( sstate, *ephemerisSub->data(), delta_time_sec );
    _sky->repaint( scolor, *ephemerisSub->data() );
}

void
FGRenderer::resize(int width, int height, int x, int y)
{
    SG_LOG(SG_VIEW, SG_DEBUG, "FGRenderer::resize: new size " << width << " x " << height);
    // must guard setting these, or PLIB-PUI fails with too many live interfaces
    if (width != _xsize->getIntValue()) _xsize->setIntValue(width);
    if (height != _ysize->getIntValue()) _ysize->setIntValue(height);
    if (x != _xpos->getIntValue()) _xpos->setIntValue(x);
    if (y != _ypos->getIntValue()) _ypos->setIntValue(y);

    // update splash node if present
    _splash->resize(width, height);
#if defined(ENABLE_QQ_UI)
    if (_quickDrawable) {
        _quickDrawable->resize(width, height);
    }
#endif
}

void
FGRenderer::resize(int width, int height)
{
    resize(width, height, _xpos->getIntValue(), _ypos->getIntValue());
}

namespace {

typedef osgUtil::LineSegmentIntersector::Intersection Intersection;

SGVec2d uvFromIntersection(const Intersection& hit)
{
    // Taken from http://trac.openscenegraph.org/projects/osg/browser/OpenSceneGraph/trunk/examples/osgmovie/osgmovie.cpp

    osg::Drawable* drawable = hit.drawable.get();
    osg::Geometry* geometry = drawable ? drawable->asGeometry() : nullptr;
    osg::Vec3Array* vertices = geometry ?
        dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray()) : nullptr;

    if (!vertices) {
        SG_LOG(SG_INPUT, SG_WARN, "Unable to get vertices for intersection.");
        return SGVec2d(-9999,-9999);
    }

    // get the vertex indices.
    const Intersection::IndexList& indices = hit.indexList;
    const Intersection::RatioList& ratios = hit.ratioList;

    if (indices.size() != 3 || ratios.size() != 3) {
        SG_LOG(SG_INPUT, SG_WARN, "Intersection has insufficient indices to work with.");
        return SGVec2d(-9999,-9999);
    }

    unsigned int i1 = indices[0];
    unsigned int i2 = indices[1];
    unsigned int i3 = indices[2];

    float r1 = ratios[0];
    float r2 = ratios[1];
    float r3 = ratios[2];

    osg::Array* texcoords = (geometry->getNumTexCoordArrays() > 0) ?
        geometry->getTexCoordArray(0) : nullptr;
    osg::Vec2Array* texcoords_Vec2Array = dynamic_cast<osg::Vec2Array*>(texcoords);

    if (!texcoords_Vec2Array) {
        SG_LOG(SG_INPUT, SG_WARN, "Unable to get texcoords for intersection.");
        return SGVec2d(-9999,-9999);
    }

    // we have tex coord array so now we can compute the final tex coord at the
    // point of intersection.
    osg::Vec2 tc1 = (*texcoords_Vec2Array)[i1];
    osg::Vec2 tc2 = (*texcoords_Vec2Array)[i2];
    osg::Vec2 tc3 = (*texcoords_Vec2Array)[i3];

    return toSG(osg::Vec2d(tc1 * r1 + tc2 * r2 + tc3 * r3));
}

} // anonymous namespace

FGRenderer::PickList FGRenderer::pick(const osg::Vec2& windowPos)
{
    PickList result;
    osgUtil::LineSegmentIntersector::Intersections intersections;

    if (!computeIntersections(CameraGroup::getDefault(), windowPos, intersections))
        return result; // return empty list
    
    // We attempt to highlight nodes until Highlight::highlight_nodes()
    // succeeds and returns +ve, or highlighting is disabled and it returns -1.
    auto highlight = globals->get_subsystem<Highlight>();
    int highlight_num_props = 0;
    
    for (const auto& hit : intersections) {
        const osg::NodePath& np = hit.nodePath;
        osg::NodePath::const_reverse_iterator npi;

        for (npi = np.rbegin(); npi != np.rend(); ++npi) {
            if (!highlight_num_props && highlight) {
                highlight_num_props = highlight->highlightNodes(*npi);
            }
            SGSceneUserData* ud = SGSceneUserData::getSceneUserData(*npi);
            if (!ud || (ud->getNumPickCallbacks() == 0))
                continue;

            for (unsigned i = 0; i < ud->getNumPickCallbacks(); ++i) {
                SGPickCallback* pickCallback = ud->getPickCallback(i);
                if (!pickCallback)
                    continue;
                SGSceneryPick sceneryPick;
                sceneryPick.info.local = toSG(hit.getLocalIntersectPoint());
                sceneryPick.info.wgs84 = toSG(hit.getWorldIntersectPoint());

                if( pickCallback->needsUV() )
                    sceneryPick.info.uv = uvFromIntersection(hit);

                sceneryPick.callback = pickCallback;
                result.push_back(sceneryPick);
            } // of installed pick callbacks iteration
        } // of reverse node path walk
    }

    return result;
}

void
FGRenderer::addCanvasCamera(osg::Camera* camera)
{
    assert(camera);

    bool should_restart_threading = getViewerBase()->areThreadsRunning();
    if (should_restart_threading) {
        getViewerBase()->stopThreading();
    }

    // Use the same graphics context as the GUI camera
    osg::Camera *guiCamera = getGUICamera(CameraGroup::getDefault());
    osg::GraphicsContext *gc = guiCamera->getGraphicsContext();
    camera->setGraphicsContext(gc);

    // Add it as a slave to the viewer
    _composite_viewer->getView(0)->addSlave(camera, false);
    simgear::installEffectCullVisitor(camera);

    if (should_restart_threading) {
        getViewerBase()->startThreading();
    }
}

void
FGRenderer::removeCanvasCamera(osg::Camera* camera)
{
    assert(camera);

    bool should_restart_threading = getViewerBase()->areThreadsRunning();
    if (should_restart_threading) {
        getViewerBase()->stopThreading();
    }

    // Remove all children before removing the slave to prevent the graphics
    // window from automatically cleaning up all associated OpenGL objects.
    camera->removeChildren(0, camera->getNumChildren());

    auto view = _composite_viewer->getView(0);
    unsigned int index = view->findSlaveIndexForCamera(camera);
    if (index < view->getNumSlaves()) {
        view->removeSlave(index);
    } else {
        SG_LOG(SG_GL, SG_WARN, "Attempted to remove unregistered Canvas camera");
    }

    if (should_restart_threading) {
        getViewerBase()->startThreading();
    }
}

osgViewer::ViewerBase*
FGRenderer::getViewerBase() const
{
    return _composite_viewer;
}

osg::ref_ptr<osgViewer::CompositeViewer>
FGRenderer::getCompositeViewer()
{
    return _composite_viewer;
}

void
FGRenderer::setCompositeViewer(osg::ref_ptr<osgViewer::CompositeViewer> composite_viewer)
{
    _composite_viewer = composite_viewer;
}

osg::FrameStamp*
FGRenderer::getFrameStamp() const
{
    assert(_composite_viewer);
    return _composite_viewer->getFrameStamp();
}

osgViewer::View*
FGRenderer::getView()
{
    // Would like to assert that FGRenderer::init() has always been called
    // before we are called, but this fails if user specifies -h, when we are
    // called by FGGlobals::~FGGlobals().
    if (_composite_viewer && _composite_viewer->getNumViews() > 0) {
        assert(_composite_viewer->getNumViews());
        return _composite_viewer->getView(0);
    }
    return nullptr;
}

const osgViewer::View*
FGRenderer::getView() const
{
    FGRenderer* this_ = const_cast<FGRenderer*>(this);
    return this_->getView();
}

void
FGRenderer::setView(osgViewer::View* view)
{
    if (_composite_viewer && _composite_viewer->getNumViews() == 0) {
        SG_LOG(SG_VIEW, SG_DEBUG, "adding view to composite_viewer.");
        _composite_viewer->stopThreading();
        _composite_viewer->addView(view);
        _composite_viewer->startThreading();
    }
}

FGEventHandler*
FGRenderer::getEventHandler()
{
    return _event_handler.get();
}

const FGEventHandler*
FGRenderer::getEventHandler() const
{
    return _event_handler.get();
}

void
FGRenderer::setEventHandler(FGEventHandler* event_handler)
{
    _event_handler = event_handler;
}

SGSky*
FGRenderer::getSky() const
{
    return _sky;
}

SplashScreen*
FGRenderer::getSplash()
{
    if (!_splash)
        _splash = new SplashScreen;
    return _splash;
}

void
FGRenderer::addChangeListener(SGPropertyChangeListener* l, const char* path)
{
    _listeners.push_back(l);
    fgAddChangeListener(l, path);
}

//------------------------------------------------------------------------------

bool
fgDumpSceneGraphToFile(const char* filename)
{
    osgViewer::View* view = globals->get_renderer()->getView();
    return osgDB::writeNodeFile(*view->getSceneData(), filename);
}

bool
fgDumpTerrainBranchToFile(const char* filename)
{
    return osgDB::writeNodeFile(*globals->get_scenery()->get_terrain_branch(),
                                filename);
}

bool
fgDumpNodeToFile(osg::Node* node, const char* filename)
{
    return osgDB::writeNodeFile(*node, filename);
}

namespace {

using namespace osg;

class VisibleSceneInfoVisitor : public NodeVisitor, CullStack {
public:
    VisibleSceneInfoVisitor() :
        NodeVisitor(CULL_VISITOR, TRAVERSE_ACTIVE_CHILDREN)
    {
        setCullingMode(CullSettings::SMALL_FEATURE_CULLING
                       | CullSettings::VIEW_FRUSTUM_CULLING);
        setComputeNearFarMode(CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    }

    VisibleSceneInfoVisitor(const VisibleSceneInfoVisitor& rhs) :
        osg::Object(rhs), NodeVisitor(rhs), CullStack(rhs)
    {
    }

    META_NodeVisitor("flightgear","VisibleSceneInfoVistor")

    typedef std::map<const std::string, int> InfoMap;

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
        auto freqComp = [](const InfoMap::iterator& lhs, const InfoMap::iterator& rhs) {
            return lhs->second > rhs->second;
        };
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
    InfoMap classInfo;
    InfoMap nodeInfo;
};

} // anonymous namespace

bool fgPrintVisibleSceneInfo(FGRenderer* renderer)
{
    osgViewer::View* view = renderer->getView();
    VisibleSceneInfoVisitor vsv;
    osg::Viewport* vp = 0;
    if (!view->getCamera()->getViewport() && view->getNumSlaves() > 0) {
        const osg::View::Slave& slave = view->getSlave(0);
        vp = slave._camera->getViewport();
    }
    vsv.doTraversal(view->getCamera(), view->getSceneData(), vp);
    return true;
}

bool fgPreliminaryGLVersionCheck()
{
    osg::ref_ptr<osg::GraphicsContext::Traits> traits =
        new osg::GraphicsContext::Traits;

    // 1x1 is enough for the check
    traits->x = 0; traits->y = 0;
    traits->width = 1; traits->height = 1;
    // RGBA8
    traits->red = 8; traits->green = 8; traits->blue = 8; traits->alpha = 8;
    // Use an off-screen pbuffer, not an actual window surface. This prevents
    // flashing from opening and closing a window very fast.
    traits->pbuffer = true;

    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = nullptr;
    traits->readDISPLAY();
    traits->setUndefinedScreenDetailsToDefaultScreen();

    // Our minimum is OpenGL 4.1 core
    traits->glContextVersion = "4.1";
    traits->glContextProfileMask = 0x1;

    osg::ref_ptr<osg::GraphicsContext> pbuffer
        = osg::GraphicsContext::createGraphicsContext(traits.get());
    return pbuffer.valid();
}
