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

#include "window.hxx"
#include <Canvas/canvas.hxx>

#include <osgGA/GUIEventHandler>

#include <boost/foreach.hpp>

namespace canvas
{
  //----------------------------------------------------------------------------
  Window::Window(SGPropertyNode* node):
    PropertyBasedElement(node),
    _image(node)
  {
    // TODO probably better remove default position and size
    node->setFloatValue("x", 50);
    node->setFloatValue("y", 100);
    node->setFloatValue("size[0]", 400);
    node->setFloatValue("size[1]", 300);

    node->setFloatValue("source/right", 1);
    node->setFloatValue("source/bottom", 1);
    node->setBoolValue("source/normalized", true);
  }

  //----------------------------------------------------------------------------
  Window::~Window()
  {
    BOOST_FOREACH(osg::Group* parent, getGroup()->getParents())
    {
      parent->removeChild(getGroup());
    }
  }

  //----------------------------------------------------------------------------
  void Window::update(double delta_time_sec)
  {
    _image.update(delta_time_sec);
  }

  //----------------------------------------------------------------------------
  void Window::valueChanged(SGPropertyNode * node)
  {
    if( node->getParent() == _node && node->getNameString() == "raise-top" )
      doRaise(node);
    else
      _image.valueChanged(node);
  }

  //----------------------------------------------------------------------------
  osg::Group* Window::getGroup()
  {
    return _image.getMatrixTransform();
  }

  //----------------------------------------------------------------------------
  const Rect<float>& Window::getRegion() const
  {
    return _image.getRegion();
  }

  //----------------------------------------------------------------------------
  void Window::setCanvas(CanvasPtr canvas)
  {
    _image.setCanvas(canvas);
  }

  //----------------------------------------------------------------------------
  CanvasWeakPtr Window::getCanvas() const
  {
    return _image.getCanvas();
  }

  //----------------------------------------------------------------------------
  bool Window::handleMouseEvent(const MouseEvent& event)
  {
    if( !getCanvas().expired() )
      return getCanvas().lock()->handleMouseEvent(event);
    else
      return false;
  }

  //----------------------------------------------------------------------------
  void Window::doRaise(SGPropertyNode* node_raise)
  {
    if( !node_raise->getBoolValue() )
      return;

    BOOST_FOREACH(osg::Group* parent, getGroup()->getParents())
    {
      // Remove window...
      parent->removeChild(getGroup());

      // ...and add again as topmost window
      parent->addChild(getGroup());
    }

    node_raise->setBoolValue(false);
  }

} // namespace canvas
