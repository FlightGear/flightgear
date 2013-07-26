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
  namespace sc = simgear::canvas;

  //----------------------------------------------------------------------------
  const std::string Window::TYPE_NAME = "window";

  //----------------------------------------------------------------------------
  Window::Window( const simgear::canvas::CanvasWeakPtr& canvas,
                  const SGPropertyNode_ptr& node,
                  const Style& parent_style,
                  Element* parent ):
    Image(canvas, node, parent_style, parent),
    _attributes_dirty(0),
    _resizable(false),
    _capture_events(true),
    _resize_top(node, "resize-top"),
    _resize_right(node, "resize-right"),
    _resize_bottom(node, "resize-bottom"),
    _resize_left(node, "resize-left"),
    _resize_status(node, "resize-status")
  {
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
    if( _attributes_dirty & DECORATION )
    {
      updateDecoration();
      _attributes_dirty &= ~DECORATION;
    }

    Image::update(delta_time_sec);
  }

  //----------------------------------------------------------------------------
  void Window::valueChanged(SGPropertyNode * node)
  {
    bool handled = false;
    if( node->getParent() == _node )
    {
      handled = true;
      const std::string& name = node->getNameString();
      if( name  == "resize" )
        _resizable = node->getBoolValue();
      else if( name == "update" )
        update(0);
      else if( name == "capture-events" )
        _capture_events = node->getBoolValue();
      else if( name == "decoration-border" )
        parseDecorationBorder(node->getStringValue());
      else if( boost::starts_with(name, "shadow-") )
        _attributes_dirty |= DECORATION;
      else
        handled = false;
    }

    if( !handled )
      Image::valueChanged(node);
  }

  //----------------------------------------------------------------------------
  osg::Group* Window::getGroup()
  {
    return getMatrixTransform();
  }

  //----------------------------------------------------------------------------
  const SGVec2<float> Window::getPosition() const
  {
    const osg::Matrix& m = getMatrixTransform()->getMatrix();
    return SGVec2<float>( m(3, 0), m(3, 1) );
  }

  //----------------------------------------------------------------------------
  const SGRect<float> Window::getScreenRegion() const
  {
    return getPosition() + getRegion();
  }

  //----------------------------------------------------------------------------
  void Window::setCanvasContent(sc::CanvasPtr canvas)
  {
    _canvas_content = canvas;

    if( _image_content )
      // Placement within decoration canvas
      _image_content->setSrcCanvas(canvas);
    else
      setSrcCanvas(canvas);
  }

  //----------------------------------------------------------------------------
  sc::CanvasWeakPtr Window::getCanvasContent() const
  {
    return _canvas_content;
  }

  //----------------------------------------------------------------------------
  sc::CanvasPtr Window::getCanvasDecoration()
  {
    return _canvas_decoration;
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
  void Window::raise()
  {
    // on writing the z-index the window always is moved to the top of all other
    // windows with the same z-index.
    set<int>("z-index", get<int>("z-index", 0));
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
  void Window::parseDecorationBorder(const std::string& str)
  {
    _decoration_border = simgear::CSSBorder::parse(str);
    _attributes_dirty |= DECORATION;
  }

  //----------------------------------------------------------------------------
  void Window::updateDecoration()
  {
    int shadow_radius = get<float>("shadow-radius") + 0.5;
    if( shadow_radius < 2 )
      shadow_radius = 0;

    if( _decoration_border.isNone() && !shadow_radius )
    {
      sc::CanvasPtr canvas_content = _canvas_content.lock();
      setSrcCanvas(canvas_content);
      set<int>("size[0]", canvas_content->getViewWidth());
      set<int>("size[1]", canvas_content->getViewHeight());

      _image_content.reset();
      _image_shadow.reset();
      _canvas_decoration->destroy();
      _canvas_decoration.reset();
      return;
    }

    sc::CanvasPtr content = _canvas_content.lock();
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
      _canvas_decoration->getProps()
                        ->setStringValue("background", "rgba(0,0,0,0)");
      setSrcCanvas(_canvas_decoration);

      _image_content = _canvas_decoration->getRootGroup()
                                         ->createChild<sc::Image>("content");
      _image_content->setSrcCanvas(content);

      // Draw content on top of decoration
      _image_content->set<int>("z-index", 1);
    }

    sc::GroupPtr group_decoration =
      _canvas_decoration->getOrCreateGroup("decoration");
    group_decoration->set<int>("tf/t[0]", shadow_radius);
    group_decoration->set<int>("tf/t[1]", shadow_radius);
    // TODO do we need clipping or shall we trust the decorator not to draw over
    //      the shadow?

    simgear::CSSBorder::Offsets const border =
      _decoration_border.getAbsOffsets(content->getViewport());

    int shad2 = 2 * shadow_radius,
        outer_width  = border.l + content->getViewWidth()  + border.r + shad2,
        outer_height = border.t + content->getViewHeight() + border.b + shad2;

    _canvas_decoration->setSizeX( outer_width );
    _canvas_decoration->setSizeY( outer_height );
    _canvas_decoration->setViewWidth( outer_width );
    _canvas_decoration->setViewHeight( outer_height );

    set<int>("size[0]", outer_width - shad2);
    set<int>("size[1]", outer_height - shad2);
    set<int>("outset", shadow_radius);

    assert(_image_content);
    _image_content->set<int>("x", shadow_radius + border.l);
    _image_content->set<int>("y", shadow_radius + border.t);

    if( !shadow_radius )
    {
      if( _image_shadow )
      {
        _image_shadow->destroy();
        _image_shadow.reset();
      }
      return;
    }

    int shadow_inset = std::max<int>(get<float>("shadow-inset") + 0.5, 0),
        slice_width  = shadow_radius + shadow_inset;

    _image_shadow = _canvas_decoration->getRootGroup()
                                      ->getOrCreateChild<sc::Image>("shadow");
    _image_shadow->set<std::string>("file", "gui/images/shadow.png");
    _image_shadow->set<float>("slice", 7);
    _image_shadow->set<std::string>("fill", "#000000");
    _image_shadow->set<float>("slice-width", slice_width);
    _image_shadow->set<int>("size[0]", outer_width);
    _image_shadow->set<int>("size[1]", outer_height);

    // Draw shadow below decoration
    _image_shadow->set<int>("z-index", -1);
  }

} // namespace canvas
