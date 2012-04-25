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

#include "CameraGroup.hxx"
#include "WindowSystemAdapter.hxx"

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osg/Viewport>

using namespace osg;
using namespace std;

namespace flightgear
{
ref_ptr<WindowSystemAdapter> WindowSystemAdapter::_wsa;

void GraphicsContextOperation::operator()(GraphicsContext* gc)
{
    run(gc);
    ++done;
}

WindowSystemAdapter::WindowSystemAdapter() :
    _nextWindowID(0), _isPuInitialized(false)
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

// The pu getWindow callback is supposed to return a window ID that
// would allow drawing a GUI on different windows. All that stuff is
// broken in multi-threaded OSG, and we only have one GUI "window"
// anyway, so just return a constant. 
int WindowSystemAdapter::puGetWindow()
{
    return 1;
}

void WindowSystemAdapter::puGetWindowSize(int* width, int* height)
{
    *width = 0;
    *height = 0;
    Camera* camera = getGUICamera(CameraGroup::getDefault());
    if (!camera)
        return;
    Viewport* vport = camera->getViewport();
    *width = (int)vport->width();
    *height = (int)vport->height();
}

void WindowSystemAdapter::puInitialize()
{
    puSetWindowFuncs(puGetWindow, 0, puGetWindowSize, 0);
    puRealInit();
}

GraphicsWindow* WindowSystemAdapter::findWindow(const string& name)
{
    for (WindowVector::iterator iter = windows.begin(), e = windows.end();
         iter != e;
         ++iter) {
        if ((*iter)->name == name)
            return iter->get();
    }
    return 0;
}
}
