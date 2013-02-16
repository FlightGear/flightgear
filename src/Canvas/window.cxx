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
#include <simgear/canvas/Canvas.hxx>

#include <osgGA/GUIEventHandler>

#include <boost/foreach.hpp>

namespace canvas
{
  //----------------------------------------------------------------------------
  Window::Window(SGPropertyNode* node):
    PropertyBasedElement(node),
    _image( simgear::canvas::CanvasPtr(),
            node,
            simgear::canvas::Style() ),
    _resizable(false),
    _resize_top(node, "resize-top"),
    _resize_right(node, "resize-right"),
    _resize_bottom(node, "resize-bottom"),
    _resize_left(node, "resize-left"),
    _resize_status(node, "resize-status")
  {
    _image.removeListener();

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

  }

  //----------------------------------------------------------------------------
  void Window::update(double delta_time_sec)
  {
    _image.update(delta_time_sec);
  }

  //----------------------------------------------------------------------------
  void Window::valueChanged(SGPropertyNode * node)
  {
    bool handled = false;
    if( node->getParent() == _node )
    {
      handled = true;
      if( node->getNameString() == "raise-top" )
        doRaise(node);
      else if( node->getNameString()  == "resize" )
        _resizable = node->getBoolValue();
      else
        handled = false;
    }

    if( !handled )
      _image.valueChanged(node);
  }

  //----------------------------------------------------------------------------
  osg::Group* Window::getGroup()
  {
    return _image.getMatrixTransform();
  }

  //----------------------------------------------------------------------------
  const SGRect<float>& Window::getRegion() const
  {
    return _image.getRegion();
  }

  //----------------------------------------------------------------------------
  void Window::setCanvas(simgear::canvas::CanvasPtr canvas)
  {
    _image.setSrcCanvas(canvas);
  }

  //----------------------------------------------------------------------------
  simgear::canvas::CanvasWeakPtr Window::getCanvas() const
  {
    return _image.getSrcCanvas();
  }

  //----------------------------------------------------------------------------
  bool Window::isResizable() const
  {
    return _resizable;
  }

  //----------------------------------------------------------------------------
  bool Window::handleMouseEvent(const simgear::canvas::MouseEventPtr& event)
  {
    if( !getCanvas().expired() )
      return getCanvas().lock()->handleMouseEvent(event);
    else
      return false;
  }

  //----------------------------------------------------------------------------
  void Window::handleResize(uint8_t mode, const osg::Vec2f& delta)
  {
    if( mode == NONE )
    {
      _resize_status = 0;
      return;
    }
    else if( mode & INIT )
    {
      _resize_top    = getRegion().t();
      _resize_right  = getRegion().r();
      _resize_bottom = getRegion().b();
      _resize_left   = getRegion().l();
      _resize_status = 1;
    }

    if( mode & BOTTOM )
      _resize_bottom += delta.y();
    else if( mode & TOP )
      _resize_top += delta.y();

    if( mode & canvas::Window::RIGHT )
      _resize_right += delta.x();
    else if( mode & canvas::Window::LEFT )
      _resize_left += delta.x();
  }

  //----------------------------------------------------------------------------
  void Window::doRaise(SGPropertyNode* node_raise)
  {
    if( node_raise && !node_raise->getBoolValue() )
      return;

    BOOST_FOREACH(osg::Group* parent, getGroup()->getParents())
    {
      // Remove window...
      parent->removeChild(getGroup());

      // ...and add again as topmost window
      parent->addChild(getGroup());
    }

    if( node_raise )
      node_raise->setBoolValue(false);
  }

} // namespace canvas
