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

#include <boost/foreach.hpp>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osg/Group>
#include <osg/Matrixd>
#include <osg/Viewport>
#include <osg/Version>
#include <osg/View>
#include <osgViewer/ViewerEventHandlers>
#include <osgViewer/Viewer>

#include <Include/general.hxx>
#include <Scenery/scenery.hxx>
#include "fg_os.hxx"
#include "fg_props.hxx"
#include "util.hxx"
#include "globals.hxx"
#include "renderer.hxx"
#include "CameraGroup.hxx"
#include "FGEventHandler.hxx"
#include "WindowBuilder.hxx"
#include "WindowSystemAdapter.hxx"

// Static linking of OSG needs special macros
#ifdef OSG_LIBRARY_STATIC
#include <osgDB/Registry>
USE_GRAPHICSWINDOW();
// Image formats
USE_OSGPLUGIN(bmp);
USE_OSGPLUGIN(dds);
USE_OSGPLUGIN(hdr);
USE_OSGPLUGIN(pic);
USE_OSGPLUGIN(pnm);
USE_OSGPLUGIN(rgb);
USE_OSGPLUGIN(tga);
#ifdef OSG_JPEG_ENABLED
  USE_OSGPLUGIN(jpeg);
#endif
#ifdef OSG_PNG_ENABLED
  USE_OSGPLUGIN(png);
#endif
#ifdef OSG_TIFF_ENABLED
  USE_OSGPLUGIN(tiff);
#endif
// Model formats
USE_OSGPLUGIN(3ds);
USE_OSGPLUGIN(ac);
USE_OSGPLUGIN(ive);
USE_OSGPLUGIN(osg);
USE_OSGPLUGIN(txf);
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

void fgOSOpenWindow(bool stencil)
{
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
    SGPropertyNode* renderingNode = fgGetNode("/sim/rendering");
    SGPropertyNode* cgroupNode = renderingNode->getNode("camera-group", true);
    bool oldSyntax = !cgroupNode->hasChild("camera");
    if (oldSyntax) {
        for (int i = 0; i < renderingNode->nChildren(); ++i) {
            SGPropertyNode* propNode = renderingNode->getChild(i);
            const char* propName = propNode->getName();
            if (!strcmp(propName, "window") || !strcmp(propName, "camera")) {
                SGPropertyNode* copiedNode
                    = cgroupNode->getNode(propName, propNode->getIndex(), true);
                copyProperties(propNode, copiedNode);
            }
        }
        vector<SGPropertyNode_ptr> cameras = cgroupNode->getChildren("camera");
        SGPropertyNode* masterCamera = 0;
        BOOST_FOREACH(SGPropertyNode_ptr& camera, cameras) {
            if (camera->getDoubleValue("shear-x", 0.0) == 0.0
                && camera->getDoubleValue("shear-y", 0.0) == 0.0) {
                masterCamera = camera.ptr();
                break;
            }
        }
        if (!masterCamera) {
            masterCamera = cgroupNode->getChild("camera", cameras.size(), true);
            setValue(masterCamera->getNode("window/name", true),
                     windowBuilder->getDefaultWindowName());
        }
        SGPropertyNode* nameNode = masterCamera->getNode("window/name");
        if (nameNode)
            setValue(cgroupNode->getNode("gui/window/name", true),
                     nameNode->getStringValue());
    }
    cameraGroup = CameraGroup::buildCameraGroup(viewer.get(), cgroupNode);
    Camera* guiCamera = getGUICamera(cameraGroup);
    if (guiCamera) {
        Viewport* guiViewport = guiCamera->getViewport();
        fgSetInt("/sim/startup/xsize", guiViewport->width());
        fgSetInt("/sim/startup/ysize", guiViewport->height());
    }
    FGEventHandler* manipulator = globals->get_renderer()->getEventHandler();
    WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
    if (wsa->windows.size() != 1) {
        manipulator->setResizable(false);
    }
    viewer->getCamera()->setProjectionResizePolicy(osg::Camera::FIXED);
    viewer->addEventHandler(manipulator);
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
    ref_ptr<FGEventHandler> manipulator
        = globals->get_renderer()->getEventHandler();
    viewer->setReleaseContextAtEndOfFrameHint(false);
    if (!viewer->isRealized())
        viewer->realize();
    while (!viewer->done()) {
        fgIdleHandler idleFunc = manipulator->getIdleHandler();
        fgDrawHandler drawFunc = manipulator->getDrawHandler();
        if (idleFunc)
            (*idleFunc)();
        if (drawFunc)
            (*drawFunc)();
        viewer->frame();
    }
    fgExit(status);
}

int fgGetKeyModifiers()
{
    return globals->get_renderer()->getEventHandler()->getCurrentModifiers();
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
    else if(cursor == MOUSE_CURSOR_TOPSIDE)
        mouseCursor = osgViewer::GraphicsWindow::TopSideCursor;
    else if(cursor == MOUSE_CURSOR_BOTTOMSIDE)
        mouseCursor = osgViewer::GraphicsWindow::BottomSideCursor;
    else if(cursor == MOUSE_CURSOR_LEFTSIDE)
        mouseCursor = osgViewer::GraphicsWindow::LeftSideCursor;
    else if(cursor == MOUSE_CURSOR_RIGHTSIDE)
        mouseCursor = osgViewer::GraphicsWindow::RightSideCursor;
    else if(cursor == MOUSE_CURSOR_TOPLEFT)
        mouseCursor = osgViewer::GraphicsWindow::TopLeftCorner;
    else if(cursor == MOUSE_CURSOR_TOPRIGHT)
        mouseCursor = osgViewer::GraphicsWindow::TopRightCorner;
    else if(cursor == MOUSE_CURSOR_BOTTOMLEFT)
        mouseCursor = osgViewer::GraphicsWindow::BottomLeftCorner;
    else if(cursor == MOUSE_CURSOR_BOTTOMRIGHT)
        mouseCursor = osgViewer::GraphicsWindow::BottomRightCorner;

    gw->setCursor(mouseCursor);
}

static int _cursor = -1;

void fgSetMouseCursor(int cursor)
{
    _cursor = cursor;
    setMouseCursor(viewer->getCamera(), cursor);
    for (unsigned i = 0; i < viewer->getNumSlaves(); ++i)
        setMouseCursor(viewer->getSlave(i)._camera.get(), cursor);
}

int fgGetMouseCursor()
{
    return _cursor;
}
