/*
 * SPDX-FileName: renderer.hxx
 * SPDX-FileComment: Written by Curtis Olson, started May 1997.
 * SPDX-FileCopyrightText: Copyright (C) 1997 - 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <osg/ref_ptr>
#include <osgViewer/CompositeViewer>

#include <simgear/props/props.hxx>
#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/timing/timestamp.hxx>


namespace osg {
class Camera;
class Group;
class FrameStamp;
}
namespace flightgear {
class FGEventHandler;
}
class SGSky;
class SGUpdateVisitor;
class SplashScreen;

class FGRenderer final {
public:
    FGRenderer();
    ~FGRenderer();

    /**
     * @brief Initialize the renderer.
     * Constructor does nothing. This is the first function that must be called
     * when initializing the renderer.
     */
    void init();

    /**
     * Called after init() was called, the graphics window has been created and
     * the CameraGroup has been initialized.
     */
    void postinit();

    /**
     * @brief Setup the scene graph root.
     * Add the sky and scenery to the scene graph root and initialize some
     * common rendering options.
     */
    void setupView();

    /**
     * @brief Run a graphics operation that retrieves some OpenGL parameters.
     * Should be called until it returns true.
     *
     * @return True when done, false when still busy (call again).
     */
    bool runInitOperation();

    /**
     * @brief Handle a window resize event.
     *
     * @param width Window width.
     * @param height Window height.
     * @param x Window horizontal position.
     * @param y Window vertical position.
     */
    void resize(int width, int height);
    void resize(int width, int height, int x, int y);

    /**
     * @brief Update rendering-related parameters.
     * This is called right before OSG's viewer->frame() on the main thread.
     * The actual drawing/rendering is done internally by OSG.
     */
    void update();

    typedef std::vector<SGSceneryPick> PickList;
    /**
     * @brief Pick into the scene and return the pick callbacks on the way.
     * @param windowPos A 2D coordinate in window space.
     */
    PickList pick(const osg::Vec2& windowPos);

    /**
     * @brief Add a Canvas RTT camera to the renderer.
     * @param camera A valid pointer to an already configured osg::Camera.
     */
    void addCanvasCamera(osg::Camera* camera);

    /**
     * @brief Remove a Canvas RTT camera from the renderer.
     * @param camera A valid pointer to a previously added Canvas camera.
     */
    void removeCanvasCamera(osg::Camera* camera);

    osgViewer::ViewerBase* getViewerBase() const;
    
    /** Both should only be used on reset. */
    osg::ref_ptr<osgViewer::CompositeViewer> getCompositeViewer();
    void setCompositeViewer(osg::ref_ptr<osgViewer::CompositeViewer> composite_viewer);

    osg::FrameStamp* getFrameStamp() const;

    osgViewer::View* getView();
    const osgViewer::View* getView() const;
    void setView(osgViewer::View* view);

    flightgear::FGEventHandler* getEventHandler();
    const flightgear::FGEventHandler* getEventHandler() const;
    void setEventHandler(flightgear::FGEventHandler* event_handler);

    SGSky* getSky() const;

    SplashScreen* getSplash();

private:
    void addChangeListener(SGPropertyChangeListener* l, const char* path);
    void updateSky();

    osg::ref_ptr<osgViewer::CompositeViewer> _composite_viewer;
    osg::ref_ptr<flightgear::FGEventHandler> _event_handler;
    osg::ref_ptr<SGUpdateVisitor> _update_visitor;
    osg::ref_ptr<osg::Group> _scene_root;

    SGPropertyNode_ptr _scenery_loaded, _position_finalized;
    SGPropertyNode_ptr _splash_alpha;
    SGPropertyNode_ptr _textures;
    SGPropertyNode_ptr _cloud_status, _visibility_m;
    SGPropertyNode_ptr _xsize, _ysize;
    SGPropertyNode_ptr _xpos, _ypos;
    SGPropertyNode_ptr _panel_hotspots, _sim_delta_sec, _altitude_ft;
    SGTimeStamp _splash_time;
    int _maximum_texture_size{0};

    typedef std::vector<SGPropertyChangeListener*> SGPropertyChangeListenerVec;
    SGPropertyChangeListenerVec _listeners;

    osg::ref_ptr<SplashScreen> _splash;

    // NOTE: Raw pointer, must be deleted in destructor
    SGSky* _sky;
};

bool fgDumpSceneGraphToFile(const char* filename);
bool fgDumpTerrainBranchToFile(const char* filename);
bool fgDumpNodeToFile(osg::Node* node, const char* filename);
bool fgPrintVisibleSceneInfo(FGRenderer* renderer);

/**
 * Attempt to create an off-screen pixel buffer to check whether our target
 * OpenGL version is available on this computer.
 * @return Whether the version check was successful or not.
 */
bool fgPreliminaryGLVersionCheck();
