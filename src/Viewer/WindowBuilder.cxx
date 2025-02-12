// Copyright (C) 2008  Tim Moore
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

#include "config.h"

#include "WindowBuilder.hxx"
#include "WindowSystemAdapter.hxx"
#include <Main/fg_props.hxx>
#include <osg/Version>

#include <GUI/MessageBox.hxx>

#include <sstream>

#if defined(SG_MAC)
    #include <osgViewer/api/Cocoa/GraphicsWindowCocoa>
#endif

using namespace std;
using namespace osg;

// forwarding proxy from QtLauncher.cxx to avoid weird double-gl.h include
// errors from MSVC
void fgqt_setPoseAsStandaloneApp(bool b)
{
    flightgear::WindowBuilder::setPoseAsStandaloneApp(b);
}

namespace flightgear
{
string makeName(const string& prefix, int num)
{
    stringstream stream;
    stream << prefix << num;
    return stream.str();
}

ref_ptr<WindowBuilder> WindowBuilder::windowBuilder;

string WindowBuilder::defaultWindowName("FlightGear");

// default to true (historical behaviour), we will clear the flag if
// we run another GUI.
bool WindowBuilder::poseAsStandaloneApp = true;

void WindowBuilder::initWindowBuilder()
{
    windowBuilder = new WindowBuilder();
}

WindowBuilder::WindowBuilder() : defaultCounter(0)
{
    makeDefaultTraits();
}

void WindowBuilder::makeDefaultTraits()
{
    GraphicsContext::WindowingSystemInterface* wsi
        = osg::GraphicsContext::getWindowingSystemInterface();
#if defined(HAVE_QT)
    if (usingQtGraphicsWindow) {
        // use the correct WSI for OpenSceneGraph >= 3.6
        wsi = osg::GraphicsContext::getWindowingSystemInterface("FlightGearQt5");
    }
#endif

    defaultTraits = new osg::GraphicsContext::Traits;

    auto traits = defaultTraits.get();
    traits->readDISPLAY();
    traits->setUndefinedScreenDetailsToDefaultScreen();

    // Should be configurable by the Compositor on a per-window basis
    // traits->red = traits->green = traits->blue = cbits;
    // traits->depth = zbits;
    // traits->stencil = 8;
    // traits->sampleBuffers = fgGetInt("/sim/rendering/multi-sample-buffers", traits->sampleBuffers);
    // traits->samples = fgGetInt("/sim/rendering/multi-samples", traits->samples);

    traits->vsync = fgGetBool("/sim/rendering/vsync-enable", traits->vsync);
    traits->doubleBuffer = true;
    traits->mipMapGeneration = true;
    traits->windowName = defaultWindowName;

    const bool wantFullscreen = fgGetBool("/sim/startup/fullscreen");
    unsigned screenwidth = 0;
    unsigned screenheight = 0;
    // this is a deprecated method, should be screen-aware.
    wsi->getScreenResolution(*traits, screenwidth, screenheight);
    
    // handle fullscreen manually
    traits->windowDecoration = !wantFullscreen;
    if (!traits->windowDecoration) {
        // fullscreen
        traits->supportsResize = false;
        traits->width = screenwidth;
        traits->height = screenheight;
        SG_LOG(SG_VIEW,SG_DEBUG,"Using full screen size for window: " << screenwidth << " x " << screenheight);
    } else {
        // window
        int w = fgGetInt("/sim/startup/xsize");
        int h = fgGetInt("/sim/startup/ysize");
        traits->supportsResize = true;
        traits->width = w;
        traits->height = h;
        if ((w>0)&&(h>0))
        {
            traits->x = ((unsigned)w>screenwidth) ? 0 : (screenwidth-w)/3;
            traits->y = ((unsigned)h>screenheight) ? 0 : (screenheight-h)/3;
        }
        SG_LOG(SG_VIEW,SG_DEBUG,"Using initial window size: " << w << " x " << h);
    }
}
    
} // of namespace flightgear

namespace
{
// Helper functions that set a value based on a property if it exists,
// returning 1 if the value was set.

inline int setFromProperty(string& place, const SGPropertyNode* node,
                            const char* name)
{
    const SGPropertyNode* valNode = node->getNode(name);
    if (valNode) {
        place = valNode->getStringValue();
        return 1;
    }
    return 0;
}

inline int setFromProperty(int& place, const SGPropertyNode* node,
                            const char* name)
{
    const SGPropertyNode* valNode = node->getNode(name);
    if (valNode) {
        place = valNode->getIntValue();
        return 1;
    }
    return 0;
}

inline int setFromProperty(bool& place, const SGPropertyNode* node,
                            const char* name)
{
    const SGPropertyNode* valNode = node->getNode(name);
    if (valNode) {
        place = valNode->getBoolValue();
        return 1;
    }
    return 0;
}
}

namespace flightgear
{

GraphicsContext* WindowBuilder::attemptToCreateGraphicsContext(
    const std::string& contextVersion, unsigned int profileMask) const
{
    // We create the traits object locally here because it gets deleted if
    // context creation is unsuccessful.
    GraphicsContext::Traits* traits = new GraphicsContext::Traits(*defaultTraits);
    setMacPoseAsStandaloneApp(traits);
    traits->glContextVersion = contextVersion;
    traits->glContextProfileMask = profileMask;
    return GraphicsContext::createGraphicsContext(traits);
}

void WindowBuilder::setFullscreenTraits(const SGPropertyNode* winNode, GraphicsContext::Traits* traits)
{
    const SGPropertyNode* orrNode = winNode->getNode("overrideRedirect");
    bool overrideRedirect = orrNode && orrNode->getBoolValue();
    traits->overrideRedirect = overrideRedirect;

    traits->windowDecoration = false;
    
    unsigned int width = 0;
    unsigned int height = 0;
    auto wsi = osg::GraphicsContext::getWindowingSystemInterface();
    wsi->getScreenResolution(*traits, width, height);
    traits->width = width;
    traits->height = height;
    traits->supportsResize = false;
    traits->x = 0;
    traits->y = 0;
}

bool WindowBuilder::setWindowedTraits(const SGPropertyNode* winNode, GraphicsContext::Traits* traits)
{
    bool customTraits = false;
    int resizable = 0;
    const SGPropertyNode* fullscreenNode = winNode->getNode("fullscreen");
    if (fullscreenNode && !fullscreenNode->getBoolValue())
    {
        traits->windowDecoration = true;
        resizable = 1;
    }
    resizable |= setFromProperty(traits->windowDecoration, winNode, "decoration");
    resizable |= setFromProperty(traits->width, winNode, "width");
    resizable |= setFromProperty(traits->height, winNode, "height");
    if (resizable) {
        traits->supportsResize = true;
        customTraits = true;
    }
    
    return customTraits;
}
    
void WindowBuilder::setMacPoseAsStandaloneApp(GraphicsContext::Traits* traits) const
{
#if defined(SG_MAC)
    // this logic is unecessary if using a Qt window, since everything
    // plays together nicely
    int flags = osgViewer::GraphicsWindowCocoa::WindowData::CheckForEvents;
    
    // avoid both QApplication and OSG::CocoaViewer doing single-application
    // init (Apple menu, making front process, etc)
    if (poseAsStandaloneApp) {
        flags |= osgViewer::GraphicsWindowCocoa::WindowData::PoseAsStandaloneApp;
    }
    traits->inheritedWindowData = new osgViewer::GraphicsWindowCocoa::WindowData(flags);
#endif
}
    
GraphicsWindow* WindowBuilder::buildWindow(const SGPropertyNode* winNode, bool isMainWindow)
{
    WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
    string windowName;
    if (winNode->hasChild("window-name"))
        windowName = winNode->getStringValue("window-name");
    else if (winNode->hasChild("name"))
        windowName = winNode->getStringValue("name");
    if (isMainWindow) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "Changing defaultWindowName from "
                << defaultWindowName << " to " << windowName);
        defaultWindowName = windowName;
    }
    GraphicsWindow* result = 0;
    if (!windowName.empty()) {
        // look for an existing window and return that
        result = wsa->findWindow(windowName);
        if (result)
            return result;
    }
    auto traits = new GraphicsContext::Traits(*defaultTraits);

    // Attempt to share context with the window that was created first
    if (!wsa->windows.empty())
        traits->sharedContext = wsa->windows.front()->gc;

    int traitsSet = setFromProperty(traits->hostName, winNode, "host-name");
    traitsSet |= setFromProperty(traits->displayNum, winNode, "display");
    traitsSet |= setFromProperty(traits->screenNum, winNode, "screen");

    const SGPropertyNode* fullscreenNode = winNode->getNode("fullscreen");
    if (fullscreenNode && fullscreenNode->getBoolValue()) {
        setFullscreenTraits(winNode, traits);
        traitsSet = 1;
    } else {
        traitsSet |= setWindowedTraits(winNode, traits);
    }
    traitsSet |= setFromProperty(traits->x, winNode, "x");
    traitsSet |= setFromProperty(traits->y, winNode, "y");
    if (!windowName.empty() && windowName != traits->windowName) {
        traits->windowName = windowName;
        traitsSet = 1;
    } else if (traitsSet) {
        traits->windowName = makeName("FlightGear", defaultCounter++);
    }

    setMacPoseAsStandaloneApp(traits);

    bool drawGUI = false;
    traitsSet |= setFromProperty(drawGUI, winNode, "gui");
    if (traitsSet) {
        GraphicsContext* gc = GraphicsContext::createGraphicsContext(traits);
        if (gc) {
            GraphicsWindow* window = WindowSystemAdapter::getWSA()
                ->registerWindow(gc, traits->windowName);
            if (drawGUI)
                window->flags |= GraphicsWindow::GUI;
            return window;
        } else {
            return 0;
        }
    } else {
        // XXX What if the window has no traits, but does have a name?
        // We should create a "default window" registered with that name.
        return getDefaultWindow();
    }
}

GraphicsWindow* WindowBuilder::getDefaultWindow()
{
    GraphicsWindow* defaultWindow
        = WindowSystemAdapter::getWSA()->findWindow(defaultWindowName);
    if (defaultWindow)
        return defaultWindow;

    auto display_settings = osg::DisplaySettings::instance();
    GraphicsContext* gc = nullptr;

#if !defined(SG_MAC)
    // Attempt to create an OpenGL 4.3 core profile context first if we are not
    // on MacOS (max version there is 4.1). We can optionally take advantage of
    // 4.3 features like compute shaders.
    display_settings->setValue("FG_GLSL_VERSION", "#version 430 core");
    gc = attemptToCreateGraphicsContext("4.3", 0x1);
#endif

    if (!gc) {
        // 4.3 is unsupported, so try with 4.1. This version is required, i.e.
        // we crash if we can't successfully create an OpenGL context.
        display_settings->setValue("FG_GLSL_VERSION", "#version 410 core");
        gc = attemptToCreateGraphicsContext("4.1", 0x1);
        if (!gc) {
            flightgear::fatalMessageBoxThenExit(
                "Unable to create OpenGL 4.1 core profile context",
                "FlightGear was unable to create a window supporting 3D rendering. "
                "This is normally due to outdated graphics drivers, please check if updates are available. ",
                "Depending on your OS and graphics chipset, updates might come from AMD, nVidia or Intel.");
            return nullptr; // unreachable anyway
        }
    }

    // Copy the winning OpenGL version to the default traits so subsequent
    // windows can use it.
    defaultTraits->glContextVersion = gc->getTraits()->glContextVersion;
    defaultTraits->glContextProfileMask = gc->getTraits()->glContextProfileMask;

    defaultWindow = WindowSystemAdapter::getWSA()->registerWindow(gc, defaultWindowName);
    return defaultWindow;
}

void WindowBuilder::setPoseAsStandaloneApp(bool b)
{
    poseAsStandaloneApp = b;
}

} // of namespace flightgear
