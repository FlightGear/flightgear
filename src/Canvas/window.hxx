// Window for placing a Canvas onto it (for dialogs, menus, etc.)
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

#ifndef CANVAS_WINDOW_HXX_
#define CANVAS_WINDOW_HXX_

#include "property_based_element.hxx"
#include "rect.hxx"
#include <Canvas/MouseEvent.hxx>

#include <simgear/props/propertyObject.hxx>

#include <osg/Geode>
#include <osg/Geometry>

namespace canvas
{
  class Window:
    public PropertyBasedElement
  {
    public:
      Window(SGPropertyNode* node);
      virtual ~Window();

      virtual void update(double delta_time_sec);
      virtual void valueChanged (SGPropertyNode * node);

      osg::Drawable* getDrawable() { return _geometry; }
      const Rect<int>& getRegion() const { return _region; }

      void setCanvas(CanvasPtr canvas);
      CanvasWeakPtr getCanvas() const;

      bool handleMouseEvent(const MouseEvent& event);

    protected:

      bool _dirty;

      osg::ref_ptr<osg::Geometry>   _geometry;
      osg::ref_ptr<osg::Vec3Array>  _vertices;
      osg::ref_ptr<osg::Vec2Array>  _tex_coords;

      simgear::PropertyObject<int>  _x, _y,
                                    _width, _height;
      Rect<int>                     _region;

      CanvasWeakPtr _canvas;
  };
} // namespace canvas

#endif /* CANVAS_WINDOW_HXX_ */
