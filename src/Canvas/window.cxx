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

#include "canvas_mgr.hxx"
#include "window.hxx"
#include <Main/globals.hxx>
#include <simgear/canvas/Canvas.hxx>
#include <simgear/scene/util/OsgMath.hxx>

#include <osgGA/GUIEventHandler>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>

namespace canvas
{
  //----------------------------------------------------------------------------
  Window::Window(SGPropertyNode* node):
    PropertyBasedElement(node),
    _attributes_dirty(0),
    _image(simgear::canvas::CanvasPtr(), node),
    _resizable(false),
    _capture_events(true),
    _resize_top(node, "resize-top"),
    _resize_right(node, "resize-right"),
    _resize_bottom(node, "resize-bottom"),
    _resize_left(node, "resize-left"),
    _resize_status(node, "resize-status")
  {
    _image.removeListener();

    node->setFloatValue("source/right", 1);
    node->setFloatValue("source/bottom", 1);
    node->setBoolValue("source/normalized", true);
  }

  //----------------------------------------------------------------------------
  Window::~Window()
  {
    if( _canvas_decoration )
      _canvas_decoration->destroy();
  }

  //----------------------------------------------------------------------------
  void Window::update(double delta_time_sec)
  {
    _image.update(delta_time_sec);

    if( _attributes_dirty & SHADOW )
    {
      float radius = get<float>("shadow-radius"),
            inset = get<float>("shadow-inset"),
            slice_width = radius + inset;

      if( slice_width <= 1 || _canvas_content.expired() )
      {
        if( _image_shadow )
        {
          getGroup()->removeChild(_image_shadow->getMatrixTransform());
          _image_shadow.reset();
        }
      }
      else
      {
        if( !_image_shadow )
        {
          _image_shadow.reset(new simgear::canvas::Image(
            _canvas_content,
            _node->getChild("image-shadow", 0, true)
          ));
          _image_shadow->set<std::string>("file", "gui/images/shadow.png");
          _image_shadow->set<float>("slice", 7);
          _image_shadow->set<std::string>("fill", "#000000");
          getGroup()->insertChild(0, _image_shadow->getMatrixTransform());
        }

        simgear::canvas::CanvasPtr canvas = _canvas_decoration
                                          ? _canvas_decoration
                                          : _canvas_content.lock();

        _image_shadow->set<float>("slice-width", slice_width);
        _image_shadow->set<int>("x", -radius);
        _image_shadow->set<int>("y", -radius);
        _image_shadow->set<int>("size[0]", canvas->getViewWidth() + 2 * radius);
        _image_shadow->set<int>("size[1]", canvas->getViewHeight()+ 2 * radius);
      }

      _attributes_dirty &= ~SHADOW;
    }

    if( _image_shadow )
      _image_shadow->update(delta_time_sec);
  }

  //----------------------------------------------------------------------------
  void Window::valueChanged(SGPropertyNode * node)
  {
    bool handled = false;
    if( node->getParent() == _node )
    {
      handled = true;
      const std::string& name = node->getNameString();
      if( name == "raise-top" )
        doRaise(node);
      else if( name  == "resize" )
        _resizable = node->getBoolValue();
      else if( name == "capture-events" )
        _capture_events = node->getBoolValue();
      else if( name == "decoration-border" )
        parseDecorationBorder(node->getStringValue());
      else if( boost::starts_with(name, "shadow-") )
        _attributes_dirty |= SHADOW;
      else
        handled = false;
    }

    if( !handled )
      _image.valueChanged(node);
  }

  //----------------------------------------------------------------------------
  void Window::childAdded(SGPropertyNode* parent, SGPropertyNode* child)
  {
    _image.childAdded(parent, child);
  }

  //----------------------------------------------------------------------------
  void Window::childRemoved(SGPropertyNode* parent, SGPropertyNode* child)
  {
    _image.childRemoved(parent, child);
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
  const SGVec2<float> Window::getPosition() const
  {
    const osg::Matrix& m = _image.getMatrixTransform()->getMatrix();
    return SGVec2<float>( m(3, 0), m(3, 1) );
  }

  //----------------------------------------------------------------------------
  const SGRect<float> Window::getScreenRegion() const
  {
    return getPosition() + getRegion();
  }

  //----------------------------------------------------------------------------
  void Window::setCanvas(simgear::canvas::CanvasPtr canvas)
  {
    _canvas_content = canvas;
    _image.setSrcCanvas(canvas);
  }

  //----------------------------------------------------------------------------
  simgear::canvas::CanvasWeakPtr Window::getCanvas() const
  {
    return _image.getSrcCanvas();
  }

  //----------------------------------------------------------------------------
  simgear::canvas::CanvasPtr Window::getCanvasDecoration()
  {
    return _canvas_decoration;
  }

  //----------------------------------------------------------------------------
  bool Window::isVisible() const
  {
    return _image.isVisible();
  }

  //----------------------------------------------------------------------------
  bool Window::isResizable() const
  {
    return _resizable;
  }

  //----------------------------------------------------------------------------
  bool Window::isCapturingEvents() const
  {
    return _capture_events;
  }

  //----------------------------------------------------------------------------
  bool Window::handleMouseEvent(const simgear::canvas::MouseEventPtr& event)
  {
    return _image.handleEvent(event);
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

  //----------------------------------------------------------------------------
  void Window::parseDecorationBorder( const std::string& str )
  {
    _decoration_border = simgear::CSSBorder::parse(str);
    if( _decoration_border.isNone() )
    {
      simgear::canvas::CanvasPtr canvas_content = _canvas_content.lock();
      _image.setSrcCanvas(canvas_content);
      _image.set<int>("size[0]", canvas_content->getViewWidth());
      _image.set<int>("size[1]", canvas_content->getViewHeight());

      _image_content.reset();
      _canvas_decoration->destroy();
      _canvas_decoration.reset();
      return;
    }

    simgear::canvas::CanvasPtr content = _canvas_content.lock();
    if( !_canvas_decoration )
    {
      CanvasMgr* mgr =
        dynamic_cast<CanvasMgr*>(globals->get_subsystem("Canvas"));

      if( !mgr )
      {
        SG_LOG(SG_GENERAL, SG_WARN, "canvas::Window: no canvas manager!");
        return;
      }

      _canvas_decoration = mgr->createCanvas("window-decoration");
      _canvas_decoration->getProps()->setStringValue("background", "rgba(0,0,0,0)");
      _image.setSrcCanvas(_canvas_decoration);

      // Decoration should be drawn first...
      _canvas_decoration->createGroup("decoration");

      // ...to allow drawing the actual content on top of the decoration
      _image_content =
        boost::dynamic_pointer_cast<simgear::canvas::Image>(
            _canvas_decoration->getRootGroup()->createChild("image", "content")
        );
      _image_content->setSrcCanvas(content);
    }

    simgear::CSSBorder::Offsets const border =
      _decoration_border.getAbsOffsets(content->getViewport());

    int outer_width  = border.l + content->getViewWidth()  + border.r,
        outer_height = border.t + content->getViewHeight() + border.b;

    _canvas_decoration->setSizeX( outer_width );
    _canvas_decoration->setSizeY( outer_height );
    _canvas_decoration->setViewWidth( outer_width );
    _canvas_decoration->setViewHeight( outer_height );

    _image.set<int>("size[0]", outer_width);
    _image.set<int>("size[1]", outer_height);

    assert(_image_content);
    _image_content->set<int>("x", border.l);
    _image_content->set<int>("y", border.t);
  }

} // namespace canvas
