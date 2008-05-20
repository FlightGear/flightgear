// Copyright (C) 2008 Tim Moore
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

#include <plib/pu.h>

#include<algorithm>
#include <functional>

#include "WindowSystemAdapter.hxx"

using namespace osg;
using namespace std;

using namespace flightgear;

ref_ptr<WindowSystemAdapter> WindowSystemAdapter::_wsa;

void GraphicsContextOperation::operator()(GraphicsContext* gc)
{
    run(gc);
    ++done;
}

WindowSystemAdapter::WindowSystemAdapter() :
    _nextWindowID(0), _nextCameraID(0), _isPuInitialized(false)
{
}

GraphicsWindow*
WindowSystemAdapter::registerWindow(GraphicsContext* gc,
                                    const string& windowName)
{
    GraphicsWindow* window = new GraphicsWindow(gc, windowName,
                                                _nextWindowID++);
    windows.push_back(window);
    return window;
}

Camera3D*
WindowSystemAdapter::registerCamera3D(GraphicsWindow* gw, Camera* camera,
                                      const string& cameraName)
{
    Camera3D* camera3D = new Camera3D(gw, camera, cameraName);
    cameras.push_back(camera3D);
    return camera3D;
}

GraphicsWindow*
WindowSystemAdapter::getGUIWindow()
{
    WindowVector::const_iterator contextIter
        = std::find_if(windows.begin(), windows.end(),
                       FlagTester<GraphicsWindow>(GraphicsWindow::GUI));
    if (contextIter == windows.end())
        return 0;
    else
        return contextIter->get();
}

int
WindowSystemAdapter::getGUIWindowID()
{
    const GraphicsWindow* gw = getGUIWindow();
    if (!gw)
        return -1;
    else
        return gw->id;
}

GraphicsContext*
WindowSystemAdapter::getGUIGraphicsContext()
{
    GraphicsWindow* gw = getGUIWindow();
    if (!gw)
        return 0;
    else
        return gw->gc.get();
}


int WindowSystemAdapter::puGetWindow()
{
    WindowSystemAdapter* wsa = getWSA();
    return wsa->getGUIWindowID();
}

void WindowSystemAdapter::puGetWindowSize(int* width, int* height)
{
    // XXX This will have to be different when multiple cameras share
    // a single window.
    WindowSystemAdapter* wsa = getWSA();
    const GraphicsContext* gc = wsa->getGUIGraphicsContext();
    const GraphicsContext::Traits *traits = gc->getTraits();
    *width = traits->width;
    *height = traits->height;
}

void WindowSystemAdapter::puInitialize()
{
    puSetWindowFuncs(puGetWindow, 0, puGetWindowSize, 0);
    puRealInit();
}
