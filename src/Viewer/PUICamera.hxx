// Copyright (C) 2017 James Turner
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

#ifndef PUICAMERA_HXX
#define PUICAMERA_HXX

#include <osg/Camera>
#include <osg/Version>
#include <osgViewer/View>

namespace osg
{
    class Texture2D;
    class Geometry;
}

namespace osgViewer {
class Viewer;
}

namespace osgGA {
class GUIEventHandler;
}

namespace flightgear
{

class PUICamera : public osg::Camera
{
public:
    static void initPUI();
    
    PUICamera();
    virtual ~PUICamera();

    osg::Object* cloneType() const override { return new PUICamera; }
    osg::Object* clone(const osg::CopyOp&) const override { return new PUICamera; }

    // osg::Camera already defines a resize() so use this name
    void resizeUi(int width, int height);

    void init(osg::Group* parent, osgViewer::View* view);

private:
    void manuallyResizeFBO(int width, int height);

    osg::Texture2D* _fboTexture = nullptr;
    osg::Geometry* _fullScreenQuad = nullptr;
    osgGA::GUIEventHandler* _eventHandler = nullptr;

    static void puGetWindowSize(int *width, int *height);
    static int puGetWindow();
};

} // of namespace flightgear

#endif // PUICAMERA_HXX
