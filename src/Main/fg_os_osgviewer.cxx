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

#include <stdlib.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <osg/GraphicsContext>
#include <osg/Group>
#include <osg/Matrixd>
#include <osg/Viewport>
#include <osg/Version>
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

#if (FG_OSG_VERSION >= 19008)
#define OSG_HAS_MOUSE_CURSOR_PATCH
#endif

// fg_os implementation using OpenSceneGraph's osgViewer::Viewer class
// to create the graphics window and run the event/update/render loop.

//
// fg_os implementation
//

static osg::ref_ptr<osgViewer::Viewer> viewer;
static osg::ref_ptr<osg::Camera> mainCamera;

void fgOSOpenWindow(int w, int h, int bpp,
                    bool alpha, bool stencil, bool fullscreen)
{
    osg::GraphicsContext::WindowingSystemInterface* wsi;
    wsi = osg::GraphicsContext::getWindowingSystemInterface();

    viewer = new osgViewer::Viewer;
    viewer->setDatabasePager(FGScenery::getPagerSingleton());
    // Avoid complications with fg's custom drawables.
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
    osg::ref_ptr<osg::GraphicsContext::Traits> traits;
    traits = new osg::GraphicsContext::Traits;
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
#ifdef WIN32
	// Ugly Hack, why does CW_USEDEFAULT works like phase of the moon?
	traits->x = 100;
	traits->y = 100;
#endif
        traits->supportsResize = true;
    }

    osg::Camera::ProjectionResizePolicy rsp = osg::Camera::VERTICAL;

    // Ok, first the children.
    // that achieves some magic ordering og the slaves so that we end up
    // in the main window more often.
    // This can be sorted out better when we got rid of glut and sdl.
    FGManipulator* manipulator = globals->get_renderer()->getManipulator();
    int nCameras = 0;
    if (fgHasNode("/sim/rendering/camera")) {
      SGPropertyNode* renderingNode = fgGetNode("/sim/rendering");
      for (int i = 0; i < renderingNode->nChildren(); ++i) {
        SGPropertyNode* cameraNode = renderingNode->getChild(i);
        if (strcmp(cameraNode->getName(), "camera") != 0)
          continue;

	nCameras++;
        // get a new copy of the traits struct
        osg::ref_ptr<osg::GraphicsContext::Traits> cameraTraits;
        cameraTraits = new osg::GraphicsContext::Traits(*traits);

        double shearx = cameraNode->getDoubleValue("shear-x", 0);
        double sheary = cameraNode->getDoubleValue("shear-y", 0);
        cameraTraits->hostName
          = cameraNode->getStringValue("host-name", traits->hostName.c_str());
        cameraTraits->displayNum
          = cameraNode->getIntValue("display", traits->displayNum);
        cameraTraits->screenNum
          = cameraNode->getIntValue("screen", traits->screenNum);
        if (cameraNode->getBoolValue("fullscreen", fullscreen)) {
          unsigned width = 0;
          unsigned height = 0;
          wsi->getScreenResolution(*cameraTraits, width, height);
          cameraTraits->windowDecoration = false;
          cameraTraits->width = width;
          cameraTraits->height = height;
          cameraTraits->supportsResize = false;
        } else {
          cameraTraits->windowDecoration = true;
          cameraTraits->width = cameraNode->getIntValue("width", w);
          cameraTraits->height = cameraNode->getIntValue("height", h);
          cameraTraits->supportsResize = true;
        }
        // FIXME, currently this is too much of a problem to route the resize
        // events. When we do no longer need sdl and such this
        // can be simplified
        cameraTraits->supportsResize = false;

        // ok found a camera configuration, add a new slave ...
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;

        osg::GraphicsContext* gc;
        gc = osg::GraphicsContext::createGraphicsContext(cameraTraits.get());
        gc->realize();
        camera->setGraphicsContext(gc);
        // If a viewport isn't set on the camera, then it's hard to dig it
        // out of the SceneView objects in the viewer, and the coordinates
        // of mouse events are somewhat bizzare.
        camera->setViewport(new osg::Viewport(0, 0, cameraTraits->width, cameraTraits->height));
        camera->setProjectionResizePolicy(rsp);
        viewer->addSlave(camera.get(), osg::Matrix::translate(-shearx, -sheary, 0), osg::Matrix());
      }
      if (nCameras > 1)
	manipulator->setResizable(false);
    }

    // now the main camera ...
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    mainCamera = camera;
    osg::GraphicsContext* gc;
    gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    gc->realize();
    gc->makeCurrent();
    camera->setGraphicsContext(gc);
    // If a viewport isn't set on the camera, then it's hard to dig it
    // out of the SceneView objects in the viewer, and the coordinates
    // of mouse events are somewhat bizzare.
    camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
    camera->setProjectionResizePolicy(rsp);
    viewer->addSlave(camera.get());

    viewer->setCameraManipulator(globals->get_renderer()->getManipulator());
    // Let FG handle the escape key with a confirmation
    viewer->setKeyEventSetsDone(0);
    osgViewer::StatsHandler* statsHandler = new osgViewer::StatsHandler;
    statsHandler->setKeyEventTogglesOnScreenStats('*');
    statsHandler->setKeyEventPrintsOutStats(0);
    viewer->addEventHandler(statsHandler);
    // The viewer won't start without some root.
    viewer->setSceneData(new osg::Group);
    globals->get_renderer()->setViewer(viewer.get());
}

static int status = 0;

void fgOSExit(int code)
{
    viewer->setDone(true);
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

// Noop
void fgOSInit(int* argc, char** argv)
{
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

void fgMakeCurrent()
{
    if (!mainCamera.valid())
        return;
    osg::GraphicsContext* gc = mainCamera->getGraphicsContext();
    if (!gc)
        return;
    gc->makeCurrent();
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
