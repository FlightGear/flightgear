#include <stdlib.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <osg/GraphicsContext>
#include <osg/Group>
#include <osg/Matrixd>
#include <osg/Viewport>
#include <osgViewer/StatsHandler>
#include <osgViewer/Viewer>
#include <osgGA/MatrixManipulator>

#include "fg_os.hxx"
#include "util.hxx"
#include "globals.hxx"
#include "renderer.hxx"

// fg_os implementation using OpenSceneGraph's osgViewer::Viewer class
// to create the graphics window and run the event/update/render loop.
//
// fg_os callback registration APIs
//


// Event handling and scene graph update is all handled by a
// manipulator. See FGManipulator.cpp
void fgRegisterIdleHandler(fgIdleHandler func)
{
    globals->get_renderer()->getManipulator()->setIdleHandler(func);
}

void fgRegisterDrawHandler(fgDrawHandler func)
{
    globals->get_renderer()->getManipulator()->setDrawHandler(func);
}

void fgRegisterWindowResizeHandler(fgWindowResizeHandler func)
{
    globals->get_renderer()->getManipulator()->setWindowResizeHandler(func);
}

void fgRegisterKeyHandler(fgKeyHandler func)
{
    globals->get_renderer()->getManipulator()->setKeyHandler(func);
}

void fgRegisterMouseClickHandler(fgMouseClickHandler func)
{
    globals->get_renderer()->getManipulator()->setMouseClickHandler(func);
}

void fgRegisterMouseMotionHandler(fgMouseMotionHandler func)
{
    globals->get_renderer()->getManipulator()->setMouseMotionHandler(func);
}

// Redraw "happens" every frame whether you want it or not.
void fgRequestRedraw()
{
}

//
// fg_os implementation
//


static osg::ref_ptr<osgViewer::Viewer> viewer;

void fgOSOpenWindow(int w, int h, int bpp,
                    bool alpha, bool stencil, bool fullscreen)
{
    viewer = new osgViewer::Viewer;
    // Avoid complications with fg's custom drawables.
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    osg::ref_ptr<osg::GraphicsContext::Traits> traits
	= new osg::GraphicsContext::Traits;
    int cbits = (bpp <= 16) ?  5 :  8;
    int zbits = (bpp <= 16) ? 16 : 24;
    traits->width = w;
    traits->height = h;
    traits->red = traits->green = traits->blue = cbits;
    traits->depth = zbits;
    if (alpha)
	traits->alpha = 8;
    if (stencil)
	traits->stencil = 8;
    if (fullscreen)
	traits->windowDecoration = false;
    else
	traits->windowDecoration = true;
    traits->supportsResize = true;
    traits->doubleBuffer = true;
    osg::GraphicsContext* gc
	= osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer->getCamera()->setGraphicsContext(gc);
    // If a viewport isn't set on the camera, then it's hard to dig it
    // out of the SceneView objects in the viewer, and the coordinates
    // of mouse events are somewhat bizzare.
    viewer->getCamera()->setViewport(new osg::Viewport(0, 0,
						       traits->width,
						       traits->height));
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
    viewer->requestWarpPointer((float)x, (float)y);
}

// Noop
void fgOSInit(int* argc, char** argv)
{
}

void fgOSFullScreen()
{
    // Noop, but is probably doable

}

// No support in OSG yet
void fgSetMouseCursor(int cursor)
{
}

int fgGetMouseCursor()
{
    return 0;
}

void fgMakeCurrent()
{
    osg::GraphicsContext* gc = viewer->getCamera()->getGraphicsContext();
    gc->makeCurrent();
}
