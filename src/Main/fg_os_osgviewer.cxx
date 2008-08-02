// fg_os_osgviewer.cxx -- common functions for fg_os interface
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
#include <config.h>
#endif

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include <stdlib.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <osg/Camera>
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
#include "CameraGroup.hxx"
#include "WindowBuilder.hxx"
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

namespace
{
// If a camera group isn't specified, build one from the top-level
// camera specs and then add a camera aligned with the master camera
// if it doesn't seem to exist.
CameraGroup* buildDefaultCameraGroup(osgViewer::Viewer* viewer,
                                     const SGPropertyNode* gnode)
{
    WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
    CameraGroup* cgroup = CameraGroup::buildCameraGroup(viewer, gnode);
    // Look for a camera with no shear
    Camera* masterCamera = 0;
    for (CameraGroup::CameraIterator citer = cgroup->camerasBegin(),
             e = cgroup->camerasEnd();
         citer != e;
         ++citer) {
        const View::Slave& slave = viewer->getSlave((*citer)->slaveIndex);
        if (slave._projectionOffset.isIdentity()) {
            masterCamera = (*citer)->camera.get();
            break;
        }
    }
    if (!masterCamera) {
        // No master camera found; better add one.
        GraphicsWindow* window
            = WindowBuilder::getWindowBuilder()->getDefaultWindow();
        masterCamera = new Camera();
        masterCamera->setGraphicsContext(window->gc.get());
        const GraphicsContext::Traits *traits = window->gc->getTraits();
        masterCamera->setViewport(new Viewport(0, 0,
                                               traits->width, traits->height));
        cgroup->addCamera(CameraGroup::DO_INTERSECTION_TEST, masterCamera,
                          Matrix(), Matrix());
    }
    // Find window on which the GUI is drawn.
    WindowVector::iterator iter = wsa->windows.begin();
    WindowVector::iterator end = wsa->windows.end();
    for (; iter != end; ++iter) {
        if ((*iter)->gc.get() == masterCamera->getGraphicsContext())
            break;
    }
    if (iter != end) {            // Better not happen
        (*iter)->flags |= GraphicsWindow::GUI;
        cgroup->buildGUICamera(0, iter->get());
    }
    return cgroup;
}
}

void fgOSOpenWindow(bool stencil)
{
    osg::GraphicsContext::WindowingSystemInterface* wsi
        = osg::GraphicsContext::getWindowingSystemInterface();

    viewer = new osgViewer::Viewer;
    viewer->setDatabasePager(FGScenery::getPagerSingleton());
    CameraGroup* cameraGroup = 0;
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
    WindowBuilder::initWindowBuilder(stencil);
    WindowBuilder *windowBuilder = WindowBuilder::getWindowBuilder();

    // Look for windows, camera groups, and the old syntax of
    // top-level cameras
    const SGPropertyNode* renderingNode = fgGetNode("/sim/rendering");
    for (int i = 0; i < renderingNode->nChildren(); ++i) {
        const SGPropertyNode* propNode = renderingNode->getChild(i);
        const char* propName = propNode->getName();
        if (!strcmp(propName, "window")) {
            windowBuilder->buildWindow(propNode);
        } else if (!strcmp(propName, "camera-group")) {
            cameraGroup = CameraGroup::buildCameraGroup(viewer.get(), propNode);
        }
    }
    if (!cameraGroup)
        cameraGroup = buildDefaultCameraGroup(viewer.get(), renderingNode);
    Camera* guiCamera = getGUICamera(cameraGroup);
    if (guiCamera) {
        Viewport* guiViewport = guiCamera->getViewport();
        fgSetInt("/sim/startup/xsize", guiViewport->width());
        fgSetInt("/sim/startup/ysize", guiViewport->height());
    }
    FGManipulator* manipulator = globals->get_renderer()->getManipulator();
    WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
    if (wsa->windows.size() != 1) {
        manipulator->setResizable(false);
    }
    viewer->getCamera()->setProjectionResizePolicy(osg::Camera::FIXED);
    viewer->setCameraManipulator(manipulator);
    // Let FG handle the escape key with a confirmation
    viewer->setKeyEventSetsDone(0);
    // The viewer won't start without some root.
    viewer->setSceneData(new osg::Group);
    globals->get_renderer()->setViewer(viewer.get());
    CameraGroup::setDefault(cameraGroup);
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
    warpGUIPointer(CameraGroup::getDefault(), x, y);
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
