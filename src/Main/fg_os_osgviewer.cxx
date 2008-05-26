// fg_os_common.cxx -- common functions for fg_os interface
// implemented as an osgViewer
//
// Copyright (C) 2007  Tim Moore timoore@redhat.com
// Copyright (C) 2007 Mathias Froehlich 
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

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include <stdlib.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <osg/GraphicsContext>
#include <osg/Group>
#include <osg/Matrixd>
#include <osg/Viewport>
#include <osg/Version>
#include <osg/View>
#include <osgViewer/ViewerEventHandlers>
#include <osgViewer/Viewer>
#include <osgGA/MatrixManipulator>

#include <Include/general.hxx>
#include <Scenery/scenery.hxx>
#include "fg_os.hxx"
#include "fg_props.hxx"
#include "util.hxx"
#include "globals.hxx"
#include "renderer.hxx"
#include "WindowSystemAdapter.hxx"

#if (FG_OSG_VERSION >= 19008)
#define OSG_HAS_MOUSE_CURSOR_PATCH
#endif

// fg_os implementation using OpenSceneGraph's osgViewer::Viewer class
// to create the graphics window and run the event/update/render loop.

//
// fg_os implementation
//

using namespace std;    
using namespace flightgear;
using namespace osg;



static osg::ref_ptr<osgViewer::Viewer> viewer;
static osg::ref_ptr<osg::Camera> mainCamera;

// Callback to prevent the GraphicsContext resized function from messing
// with the projection matrix of the slave

namespace
{
// silly function for making the default window and camera names
std::string makeName(const string& prefix, int num)
{
    std::stringstream stream;
    stream << prefix << num;
    return stream.str();
}

GraphicsContext::Traits*
makeDefaultTraits(GraphicsContext::WindowingSystemInterface* wsi, bool stencil)
{
    int w = fgGetInt("/sim/startup/xsize");
    int h = fgGetInt("/sim/startup/ysize");
    int bpp = fgGetInt("/sim/rendering/bits-per-pixel");
    bool alpha = fgGetBool("/sim/rendering/clouds3d-enable");
    bool fullscreen = fgGetBool("/sim/startup/fullscreen");

    GraphicsContext::Traits* traits = new osg::GraphicsContext::Traits;
    traits->readDISPLAY();
    int cbits = (bpp <= 16) ?  5 :  8;
    int zbits = (bpp <= 16) ? 16 : 24;
    traits->red = traits->green = traits->blue = cbits;
    traits->depth = zbits;
    if (alpha)
	traits->alpha = 8;
    if (stencil)
	traits->stencil = 8;
    traits->doubleBuffer = true;
    traits->mipMapGeneration = true;
    traits->windowName = "FlightGear";
    // XXX should check per window too.
    traits->sampleBuffers = fgGetBool("/sim/rendering/multi-sample-buffers", traits->sampleBuffers);
    traits->samples = fgGetBool("/sim/rendering/multi-samples", traits->samples);
    traits->vsync = fgGetBool("/sim/rendering/vsync-enable", traits->vsync);
    if (fullscreen) {
        unsigned width = 0;
        unsigned height = 0;
        wsi->getScreenResolution(*traits, width, height);
	traits->windowDecoration = false;
        traits->width = width;
        traits->height = height;
        traits->supportsResize = false;
    } else {
	traits->windowDecoration = true;
        traits->width = w;
        traits->height = h;
#if defined(WIN32) || defined(__APPLE__)
        // Ugly Hack, why does CW_USEDEFAULT works like phase of the moon?
        // Mac also needs this to show window frame, menubar and Docks
        traits->x = 100;
        traits->y = 100;
#endif
        traits->supportsResize = true;
    }
    return traits;
}

void setTraitsFromProperties(GraphicsContext::Traits* traits,
                             const SGPropertyNode* winNode,
                             GraphicsContext::WindowingSystemInterface* wsi)
{
    traits->hostName
        = winNode->getStringValue("host-name", traits->hostName.c_str());
    traits->displayNum = winNode->getIntValue("display", traits->displayNum);
    traits->screenNum = winNode->getIntValue("screen", traits->screenNum);
    if (winNode->getBoolValue("fullscreen",
                              fgGetBool("/sim/startup/fullscreen"))) {
        unsigned width = 0;
        unsigned height = 0;
        wsi->getScreenResolution(*traits, width, height);
        traits->windowDecoration = false;
        traits->width = width;
        traits->height = height;
        traits->supportsResize = false;
    } else {
        traits->windowDecoration = winNode->getBoolValue("decoration", true);
        traits->width = winNode->getIntValue("width", traits->width);
        traits->height = winNode->getIntValue("height", traits->height);
        traits->supportsResize = true;
    }
    traits->x = winNode->getIntValue("x", traits->x);
    traits->y = winNode->getIntValue("y", traits->y);
    if (winNode->hasChild("window-name"))
        traits->windowName = winNode->getStringValue("window-name");
    else if (winNode->hasChild("name")) 
        traits->windowName = winNode->getStringValue("name");
}

} //namespace

void fgOSOpenWindow(bool stencil)
{
    osg::GraphicsContext::WindowingSystemInterface* wsi;
    wsi = osg::GraphicsContext::getWindowingSystemInterface();

    viewer = new osgViewer::Viewer;
    viewer->setDatabasePager(FGScenery::getPagerSingleton());
    std::string mode;
    mode = fgGetString("/sim/rendering/multithreading-mode", "SingleThreaded");
    if (mode == "AutomaticSelection")
      viewer->setThreadingModel(osgViewer::Viewer::AutomaticSelection);
    else if (mode == "CullDrawThreadPerContext")
      viewer->setThreadingModel(osgViewer::Viewer::CullDrawThreadPerContext);
    else if (mode == "DrawThreadPerContext")
      viewer->setThreadingModel(osgViewer::Viewer::DrawThreadPerContext);
    else if (mode == "CullThreadPerCameraDrawThreadPerContext")
      viewer->setThreadingModel(osgViewer::Viewer::CullThreadPerCameraDrawThreadPerContext);
    else
      viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    osg::ref_ptr<osg::GraphicsContext::Traits> traits
        = makeDefaultTraits(wsi, stencil);

    // Ok, first the children.
    // that achieves some magic ordering og the slaves so that we end up
    // in the main window more often.
    // This can be sorted out better when we got rid of glut and sdl.
    FGManipulator* manipulator = globals->get_renderer()->getManipulator();
    string defaultName("slave");
    WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
    if (fgHasNode("/sim/rendering/camera")) {
        SGPropertyNode* renderingNode = fgGetNode("/sim/rendering");
        for (int i = 0; i < renderingNode->nChildren(); ++i) {
            SGPropertyNode* cameraNode = renderingNode->getChild(i);
            if (strcmp(cameraNode->getName(), "camera") != 0)
                continue;

            // get a new copy of the traits struct
            osg::ref_ptr<osg::GraphicsContext::Traits> cameraTraits;
            cameraTraits = new osg::GraphicsContext::Traits(*traits);
            double shearx = cameraNode->getDoubleValue("shear-x", 0);
            double sheary = cameraNode->getDoubleValue("shear-y", 0);
            double heading = cameraNode->getDoubleValue("heading-deg", 0);
            setTraitsFromProperties(cameraTraits.get(), cameraNode, wsi);
            // FIXME, currently this is too much of a problem to route
            // the resize events. When we do no longer need sdl and
            // such this can be simplified
            cameraTraits->supportsResize = false;

            // ok found a camera configuration, add a new slave if possible
            GraphicsContext* gc
                = GraphicsContext::createGraphicsContext(cameraTraits.get());
            if (gc) {
                gc->realize();
                Camera *camera = new Camera;
                camera->setGraphicsContext(gc);
                // If a viewport isn't set on the camera, then it's
                // hard to dig it out of the SceneView objects in the
                // viewer, and the coordinates of mouse events are
                // somewhat bizzare.
                camera->setViewport(new Viewport(0, 0, cameraTraits->width,
                                                 cameraTraits->height));
                const char* cameraName = cameraNode->getStringValue("name");
                string cameraNameString = (cameraName ? string(cameraName)
                                           : makeName(defaultName, i));
                GraphicsWindow* window = wsa->registerWindow(gc,
                                                             cameraNameString);
                Camera3D* cam3D = wsa->registerCamera3D(window, camera,
                                                        cameraNameString);
                if (shearx == 0 && sheary == 0)
                    cam3D->flags |= Camera3D::MASTER;
                
                osg::Matrix pOff = osg::Matrix::translate(-shearx, -sheary, 0);
                osg::Matrix vOff;
                vOff.makeRotate(SGMiscd::deg2rad(heading), osg::Vec3(0, 1, 0));
                viewer->addSlave(camera, pOff, vOff);
            } else {
                SG_LOG(SG_GENERAL, SG_WARN,
                       "Couldn't create graphics context on "
                       << cameraTraits->hostName << ":"
                       << cameraTraits->displayNum
                       << "." << cameraTraits->screenNum);
            }
        }
    }
    // now the main camera ...
    // XXX mainCamera's purpose is to establish a "main graphics
    // context" that can be made current (if necessary). But that
    // should be a context established with a window. It's used to
    // choose whether to render the GUI and panel camera nodes, but
    // that's obsolete because the GUI is rendered in its own
    // slave. And it's used to translate mouse event coordinates, but
    // that's bogus because mouse clicks should work on any camera. In
    // short, mainCamera must die :)
    Camera3DVector::iterator citr
        = find_if(wsa->cameras.begin(), wsa->cameras.end(),
                  WindowSystemAdapter::FlagTester<Camera3D>(Camera3D::MASTER));
    if (citr == wsa->cameras.end()) {
        // Create a camera aligned with the master camera. Eventually
        // this will be optional.
        Camera* camera = new osg::Camera;
        mainCamera = camera;
        osg::GraphicsContext* gc
            = osg::GraphicsContext::createGraphicsContext(traits.get());
        gc->realize();
        gc->makeCurrent();
        camera->setGraphicsContext(gc);
        // If a viewport isn't set on the camera, then it's hard to dig it
        // out of the SceneView objects in the viewer, and the coordinates
        // of mouse events are somewhat bizzare.
        camera->setViewport(new osg::Viewport(0, 0,
                                              traits->width, traits->height));
        GraphicsWindow* window = wsa->registerWindow(gc, string("main"));
        window->flags |= GraphicsWindow::GUI;
        Camera3D* camera3d = wsa->registerCamera3D(window, camera,
                                                   string("main"));
        camera3d->flags |= Camera3D::MASTER;
        // Why a slave? It seems to be the easiest way to assign cameras,
        // for which we've created the graphics context ourselves, to
        // the viewer. 
        viewer->addSlave(camera);
    } else {
        mainCamera = (*citr)->camera;
    }
    if (wsa->cameras.size() != 1) {
        manipulator->setResizable(false);
    }
    viewer->getCamera()->setProjectionResizePolicy(osg::Camera::FIXED);
    viewer->setCameraManipulator(manipulator);
    // Let FG handle the escape key with a confirmation
    viewer->setKeyEventSetsDone(0);
    // The viewer won't start without some root.
    viewer->setSceneData(new osg::Group);
    globals->get_renderer()->setViewer(viewer.get());
}

static int status = 0;

void fgOSExit(int code)
{
    viewer->setDone(true);
    viewer->getDatabasePager()->cancel();
    status = code;
}

void fgOSMainLoop()
{
    viewer->run();
    fgExit(status);
}

int fgGetKeyModifiers()
{
    return globals->get_renderer()->getManipulator()->getCurrentModifiers();
}

void fgWarpMouse(int x, int y)
{
    globals->get_renderer()->getManipulator()->setMouseWarped();
    // Hack, currently the pointer is just recentered. So, we know the
    // relative coordinates ...
    if (!mainCamera.valid()) {
        viewer->requestWarpPointer(0, 0);
        return;
    }
    float xsize = (float)mainCamera->getGraphicsContext()->getTraits()->width;
    float ysize = (float)mainCamera->getGraphicsContext()->getTraits()->height;
    viewer->requestWarpPointer(2.0f * (float)x / xsize - 1.0f,
                               1.0f - 2.0f * (float)y / ysize);
}

void fgOSInit(int* argc, char** argv)
{
    WindowSystemAdapter::setWSA(new WindowSystemAdapter);
}

// Noop
void fgOSFullScreen()
{
}

#ifdef OSG_HAS_MOUSE_CURSOR_PATCH
static void setMouseCursor(osg::Camera* camera, int cursor)
{
    if (!camera)
        return;
    osg::GraphicsContext* gc = camera->getGraphicsContext();
    if (!gc)
        return;
    osgViewer::GraphicsWindow* gw;
    gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc);
    if (!gw)
        return;
    
    osgViewer::GraphicsWindow::MouseCursor mouseCursor;
    mouseCursor = osgViewer::GraphicsWindow::InheritCursor;
    if     (cursor == MOUSE_CURSOR_NONE)
        mouseCursor = osgViewer::GraphicsWindow::NoCursor;
    else if(cursor == MOUSE_CURSOR_POINTER)
        mouseCursor = osgViewer::GraphicsWindow::RightArrowCursor;
    else if(cursor == MOUSE_CURSOR_WAIT)
        mouseCursor = osgViewer::GraphicsWindow::WaitCursor;
    else if(cursor == MOUSE_CURSOR_CROSSHAIR)
        mouseCursor = osgViewer::GraphicsWindow::CrosshairCursor;
    else if(cursor == MOUSE_CURSOR_LEFTRIGHT)
        mouseCursor = osgViewer::GraphicsWindow::LeftRightCursor;

    gw->setCursor(mouseCursor);
}
#endif

static int _cursor = -1;

void fgSetMouseCursor(int cursor)
{
    _cursor = cursor;
#ifdef OSG_HAS_MOUSE_CURSOR_PATCH
    setMouseCursor(viewer->getCamera(), cursor);
    for (unsigned i = 0; i < viewer->getNumSlaves(); ++i)
        setMouseCursor(viewer->getSlave(i)._camera.get(), cursor);
#endif
}

int fgGetMouseCursor()
{
    return _cursor;
}

bool fgOSIsMainContext(const osg::GraphicsContext* context)
{
    if (!mainCamera.valid())
        return false;
    return context == mainCamera->getGraphicsContext();
}

bool fgOSIsMainCamera(const osg::Camera* camera)
{
  if (!camera)
    return false;
  if (camera == mainCamera.get())
    return true;
  if (!viewer.valid())
    return false;
  if (camera == viewer->getCamera())
    return true;
  return false;
}

GraphicsContext* fgOSGetMainContext()
{
    WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
    WindowVector::iterator contextIter
        = std::find_if(wsa->windows.begin(), wsa->windows.end(),
                       WindowSystemAdapter::FlagTester<GraphicsWindow>(GraphicsWindow::GUI));
    if (contextIter == wsa->windows.end())
        return 0;
    else
        return (*contextIter)->gc.get();
}

