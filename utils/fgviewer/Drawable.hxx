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

#ifndef _fgviewer_Drawable_hxx
#define _fgviewer_Drawable_hxx

#include <string>
#include <list>
#include <osg/GraphicsContext>
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGWeakReferenced.hxx>

namespace fgviewer  {

class Viewer;
class SlaveCamera;

class Drawable : public SGWeakReferenced {
public:
    Drawable(const std::string& name);
    virtual ~Drawable();
    
    const std::string& getName() const
    { return _name; }
    
    virtual bool setScreenIdentifier(const std::string& screenIdentifier)
    {
        if (_graphicsContext.valid())
            return false;
        _screenIdentifier = screenIdentifier;
        return true;
    }
    const std::string& getScreenIdentifier() const
    { return _screenIdentifier; }
    
    virtual bool setPosition(const SGVec2i& position)
    { _position = position; return true; }
    const SGVec2i& getPosition() const
    { return _position; }
    
    virtual bool setSize(const SGVec2i& size)
    { _size = size; return true; }
    const SGVec2i& getSize() const
    { return _size; }
    
    virtual bool setOffscreen(bool offscreen)
    {
        if (_graphicsContext.valid())
            return false;
        _offscreen = offscreen;
        return true;
    }
    bool getOffscreen() const
    { return _offscreen; }
    
    virtual bool setFullscreen(bool fullscreen)
    { _fullscreen = fullscreen; return true; }
    bool getFullscreen() const
    { return _fullscreen; }
    
    osg::GraphicsContext* getGraphicsContext()
    { return _graphicsContext.get(); }

    void attachSlaveCamera(SlaveCamera* slaveCamera);
    
    virtual bool resize(int x, int y, int width, int height);

    bool realize(Viewer& viewer);

protected:
    virtual osg::GraphicsContext* _realizeImplementation(Viewer& viewer);
    osg::ref_ptr<osg::GraphicsContext::Traits> _getTraits(Viewer& viewer);
    
private:
    Drawable(const Drawable&);
    Drawable& operator=(const Drawable&);

    class _ResizedCallback;

    /// The immutable name that is used to reference this slave camera.
    const std::string _name;
    std::string _screenIdentifier;
    SGVec2i _position;
    SGVec2i _size;
    bool _offscreen;
    bool _fullscreen;
    osg::ref_ptr<osg::GraphicsContext> _graphicsContext;
    std::list<SGSharedPtr<SlaveCamera> > _slaveCameraList;
};

} // namespace fgviewer

#endif
