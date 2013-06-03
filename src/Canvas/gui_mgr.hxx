// Canvas gui/dialog manager
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef CANVAS_GUI_MGR_HXX_
#define CANVAS_GUI_MGR_HXX_

#include "canvas_fwd.hpp"

#include <simgear/canvas/canvas_fwd.hxx>
#include <simgear/props/PropertyBasedMgr.hxx>
#include <simgear/props/propertyObject.hxx>

#include <osg/ref_ptr>
#include <osg/Geode>
#include <osg/MatrixTransform>

namespace osgGA
{
  class GUIEventAdapter;
}

class GUIEventHandler;
class GUIMgr:
  public simgear::PropertyBasedMgr
{
  public:
    GUIMgr();

    canvas::WindowPtr createWindow(const std::string& name = "");

    virtual void init();
    virtual void shutdown();

    bool handleEvent(const osgGA::GUIEventAdapter& ea);

  protected:

    osg::ref_ptr<GUIEventHandler>       _event_handler;
    osg::ref_ptr<osg::MatrixTransform>  _transform;
    SGPropertyChangeCallback<GUIMgr>    _cb_mouse_mode;
    bool                                _handle_events;

    simgear::PropertyObject<int>        _width,
                                        _height;

    canvas::WindowWeakPtr _last_push,
                          _last_mouse_over,
                          _resize_window;
    uint8_t _resize;
    int     _last_cursor;

    float _last_x,
          _last_y;
    double _last_scroll_time;

    virtual void elementCreated(simgear::PropertyBasedElementPtr element);

    canvas::WindowPtr getWindow(size_t i);
    simgear::canvas::Placements
    addPlacement(SGPropertyNode*, simgear::canvas::CanvasPtr canvas);

    bool handleMouse(const osgGA::GUIEventAdapter& ea);
    void handleResize(int x, int y, int width, int height);
    void handleMouseMode(SGPropertyNode* node);
};

#endif /* CANVAS_GUI_MGR_HXX_ */
