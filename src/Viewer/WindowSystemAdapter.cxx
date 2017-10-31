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
    _nextWindowID(0)
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
