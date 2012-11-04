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

#include <simgear/canvas/elements/CanvasImage.hxx>
#include <simgear/canvas/MouseEvent.hxx>
#include <simgear/props/PropertyBasedElement.hxx>
#include <simgear/props/propertyObject.hxx>

#include <osg/Geode>
#include <osg/Geometry>

namespace canvas
{
  class Window:
    public simgear::PropertyBasedElement
  {
    public:
      Window(SGPropertyNode* node);
      virtual ~Window();

      virtual void update(double delta_time_sec);
      virtual void valueChanged (SGPropertyNode * node);

      osg::Group* getGroup();
      const simgear::Rect<float>& getRegion() const;

      void setCanvas(simgear::canvas::CanvasPtr canvas);
      simgear::canvas::CanvasWeakPtr getCanvas() const;

      bool handleMouseEvent(const simgear::canvas::MouseEvent& event);

    protected:

      simgear::canvas::Image _image;

      void doRaise(SGPropertyNode* node_raise);
  };
} // namespace canvas

#endif /* CANVAS_WINDOW_HXX_ */
