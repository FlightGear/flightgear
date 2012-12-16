// Viewer.hxx -- alternative flightgear viewer application
//
// Copyright (C) 2009 - 2012  Mathias Froehlich
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

#include "Drawable.hxx"

#include "Viewer.hxx"

#include <simgear/structure/SGWeakPtr.hxx>

namespace fgviewer  {

class Drawable::_ResizedCallback : public osg::GraphicsContext::ResizedCallback {
public:
    _ResizedCallback(Drawable* drawable) :
        _drawable(drawable)
    { }
    virtual ~_ResizedCallback()
    { }
    virtual void resizedImplementation(osg::GraphicsContext*, int x, int y, int width, int height)
    {
        SGSharedPtr<Drawable> drawable = _drawable.lock();
        if (!drawable.valid())
            return;
        drawable->resize(x, y, width, height);
    }
    SGWeakPtr<Drawable> _drawable;
};

Drawable::Drawable(const std::string& name) :
    _name(name),
    _position(0, 0),
    _size(600, 800),
    _offscreen(false),
    _fullscreen(false)
{
}

Drawable::~Drawable()
{
}

void
Drawable::attachSlaveCamera(SlaveCamera* slaveCamera)
{
    if (!slaveCamera)
        return;
    _slaveCameraList.push_back(slaveCamera);
}

bool
Drawable::resize(int x, int y, int width, int height)
{
    unsigned numCameras = _slaveCameraList.size();
    if (1 < numCameras)
        return false;
    _graphicsContext->resizedImplementation(x, y, width, height);
    if (numCameras < 1)
        return true;
    _slaveCameraList.front()->setViewport(SGVec4i(0, 0, width, height));
    return true;
}
    
bool
Drawable::realize(Viewer& viewer)
{
    if (_graphicsContext.valid())
        return false;
    _graphicsContext = _realizeImplementation(viewer);
    if (!_graphicsContext.valid())
        return false;
    _graphicsContext->setResizedCallback(new _ResizedCallback(this));
    return true;
}
    
osg::GraphicsContext*
Drawable::_realizeImplementation(Viewer& viewer)
{
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = _getTraits(viewer);
    return viewer.createGraphicsContext(traits.get());
}
    
osg::ref_ptr<osg::GraphicsContext::Traits>
Drawable::_getTraits(Viewer& viewer)
{
    osg::GraphicsContext::ScreenIdentifier screenIdentifier;
    screenIdentifier.setScreenIdentifier(_screenIdentifier);
    
    osg::ref_ptr<osg::GraphicsContext::Traits> traits;
    traits = viewer.getTraits(screenIdentifier);
    
    // The window name as displayed by the window manager
    traits->windowName = getName();
    
    traits->x = _position[0];
    traits->y = _position[1];
    if (!_fullscreen) {
        traits->width = _size[0];
        traits->height = _size[1];
    }
    
    traits->windowDecoration = !_fullscreen;
    if (_slaveCameraList.size() <= 1)
        traits->supportsResize = true;
    else
        traits->supportsResize = false;
    traits->pbuffer = _offscreen;
    
    return traits;
}

} // namespace fgviewer
