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

#include "gui_mgr.hxx"

#include <Main/fg_os.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/CameraGroup.hxx>
#include <Viewer/renderer.hxx>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasPlacement.hxx>
#include <simgear/canvas/CanvasWindow.hxx>
#include <simgear/canvas/events/KeyboardEvent.hxx>
#include <simgear/scene/util/OsgMath.hxx>

#include <osg/BlendFunc>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventHandler>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

class DesktopGroup;
typedef SGSharedPtr<DesktopGroup> DesktopPtr;
typedef SGWeakPtr<DesktopGroup> DesktopWeakPtr;

namespace sc = simgear::canvas;

/**
 * Event handler
 */
class GUIEventHandler:
  public osgGA::GUIEventHandler
{
  public:
    GUIEventHandler(const DesktopWeakPtr& desktop_group);

    bool handle( const osgGA::GUIEventAdapter& ea,
                 osgGA::GUIActionAdapter&,
                 osg::Object*,
                 osg::NodeVisitor* );

  protected:
    DesktopWeakPtr _desktop;
};

/**
 * Track a canvas placement on a window
 */
class WindowPlacement:
  public sc::Placement
{
  public:
    WindowPlacement( SGPropertyNode* node,
                     sc::WindowPtr window,
                     sc::CanvasPtr canvas ):
      Placement(node),
      _window(window),
      _canvas(canvas)
    {}

    /**
     * Remove placement from window
     */
    virtual ~WindowPlacement()
    {
      sc::WindowPtr window = _window.lock();
      sc::CanvasPtr canvas = _canvas.lock();

      if( window && canvas && canvas == window->getCanvasContent().lock() )
        window->setCanvasContent( sc::CanvasPtr() );
    }

  private:
    sc::WindowWeakPtr _window;
    sc::CanvasWeakPtr _canvas;
};

/**
 * Desktop root group
 */
class DesktopGroup:
  public sc::Group
{
  public:
    DesktopGroup();

    void setFocusWindow(const sc::WindowPtr& window);

    bool grabPointer(const sc::WindowPtr& window);
    void ungrabPointer(const sc::WindowPtr& window);

    bool handleEvent(const osgGA::GUIEventAdapter& ea);

  protected:

    friend class GUIMgr;

    SGPropertyChangeCallback<DesktopGroup> _cb_mouse_mode;
    bool                                   _handle_events;

    simgear::PropertyObject<int>        _width,
                                        _height;

    sc::WindowWeakPtr _last_push,
                      _last_mouse_over,
                      _resize_window,
                      _focus_window,
                      _pointer_grab_window;

    uint8_t _resize;
    int     _last_cursor;

    osg::Vec2 _drag_start;
    float _last_x,
          _last_y;
    double _last_scroll_time;

    uint32_t _last_key_down_no_mod; // Key repeat for non modifier keys

    bool handleMouse(const osgGA::GUIEventAdapter& ea);
    bool handleKeyboard(const osgGA::GUIEventAdapter& ea);

    bool handleEvent( const sc::EventPtr& event,
                      const sc::WindowPtr& active_window );
    bool handleRootEvent(const sc::EventPtr& event);

    void handleResize(int x, int y, int width, int height);
    void handleMouseMode(SGPropertyNode* node);

    /**
     *
     */
    sc::ElementFactory
    getChildFactory(const std::string& type) const
    {
      if( type == "window" )
        return &Element::create<sc::Window>;

      return Group::getChildFactory(type);
    }
};

//------------------------------------------------------------------------------
GUIEventHandler::GUIEventHandler(const DesktopWeakPtr& desktop_group):
  _desktop( desktop_group )
{

}

//------------------------------------------------------------------------------
bool GUIEventHandler::handle( const osgGA::GUIEventAdapter& ea,
                              osgGA::GUIActionAdapter&,
                              osg::Object*,
                              osg::NodeVisitor* )
{
  if( ea.getHandled() )
    return false;

  DesktopPtr desktop = _desktop.lock();
  return desktop && desktop->handleEvent(ea);
}

//------------------------------------------------------------------------------
DesktopGroup::DesktopGroup():
  Group(sc::CanvasPtr(), fgGetNode("/sim/gui/canvas", true)),
  _cb_mouse_mode( this,
                  &DesktopGroup::handleMouseMode,
                  fgGetNode("/devices/status/mice/mouse[0]/mode") ),
  _handle_events(true),
  _width(_node, "size[0]"),
  _height(_node, "size[1]"),
  _resize(sc::Window::NONE),
  _last_cursor(MOUSE_CURSOR_NONE),
  _last_x(-1),
  _last_y(-1),
  _last_scroll_time(0),
  _last_key_down_no_mod(-1)
{
  osg::Camera* camera =
    flightgear::getGUICamera( flightgear::CameraGroup::getDefault() );
  assert(camera);
  camera->addChild( getMatrixTransform() );

  osg::StateSet* stateSet = _transform->getOrCreateStateSet();
  stateSet->setDataVariance(osg::Object::STATIC);
  stateSet->setRenderBinDetails(1000, "RenderBin");

  // speed optimization?
  stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
  stateSet->setAttribute(new osg::BlendFunc(
    osg::BlendFunc::SRC_ALPHA,
    osg::BlendFunc::ONE_MINUS_SRC_ALPHA)
  );
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
  stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

  _width = _height = -1;
}

//------------------------------------------------------------------------------
void DesktopGroup::setFocusWindow(const sc::WindowPtr& window)
{
  _focus_window = window;
}

//------------------------------------------------------------------------------
bool DesktopGroup::grabPointer(const sc::WindowPtr& window)
{
  sc::WindowPtr resize = _resize_window.lock();
  if( (resize && resize != window) || !_pointer_grab_window.expired() )
    // Already grabbed (resize -> implicit grab)
    return false;

  _pointer_grab_window = window;
  return true;
}

//------------------------------------------------------------------------------
void DesktopGroup::ungrabPointer(const sc::WindowPtr& window)
{
  if( _pointer_grab_window.expired() )
    SG_LOG(SG_GUI, SG_WARN, "ungrabPointer: no active grab.");
  else if( window != _pointer_grab_window.lock() )
    SG_LOG(SG_GUI, SG_WARN, "ungrabPointer: window is not owner of the grab.");
  else
    _pointer_grab_window.reset();
}

//------------------------------------------------------------------------------
bool DesktopGroup::handleEvent(const osgGA::GUIEventAdapter& ea)
{
  switch( ea.getEventType() )
  {
    case osgGA::GUIEventAdapter::PUSH:
    case osgGA::GUIEventAdapter::RELEASE:
//    case osgGA::GUIEventAdapter::DOUBLECLICK:
//    // DOUBLECLICK doesn't seem to be triggered...
    case osgGA::GUIEventAdapter::DRAG:
    case osgGA::GUIEventAdapter::MOVE:
    case osgGA::GUIEventAdapter::SCROLL:
      return handleMouse(ea);
    case osgGA::GUIEventAdapter::KEYDOWN:
    case osgGA::GUIEventAdapter::KEYUP:
      return handleKeyboard(ea);
    case osgGA::GUIEventAdapter::RESIZE:
      handleResize( ea.getWindowX(),
                    ea.getWindowY(),
                    ea.getWindowWidth(),
                    ea.getWindowHeight() );
      return false; // Let other event handlers also consume resize events
    default:
      return false;
  }
}

/*
RESIZE AREAS
============

|   || |      _ inside corner region (L-shaped part inside margin) both
|___||_|_ _ _/  directions can be resized (outside only one axis)
|   || |     |
|   || |
|   || |_____|__                  _
|   ||       |   } margin_neg      \
|    ========|== <-- window border  |_ area where resize
|            |   } margin_pos       |  can be initiated
|____________|__/                 _/
|<- corner ->|
*/
const float resize_margin_pos = 12;
const float resize_margin_neg = 2;
const float resize_corner = 20;

//------------------------------------------------------------------------------
bool DesktopGroup::handleMouse(const osgGA::GUIEventAdapter& ea)
{
  if( !_transform->getNumChildren() || !_handle_events )
    return false;

  sc::MouseEventPtr event(new sc::MouseEvent(ea));

  event->screen_pos.x() = 0.5 * (ea.getXnormalized() + 1) * _width + 0.5;
  event->screen_pos.y() = 0.5 * (ea.getYnormalized() + 1) * _height + 0.5;
  if(    ea.getMouseYOrientation()
      != osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS )
    event->screen_pos.y() = _height - event->screen_pos.y();

  event->delta.x() = event->getScreenX() - _last_x;
  event->delta.y() = event->getScreenY() - _last_y;

  _last_x = event->getScreenX();
  _last_y = event->getScreenY();

  event->local_pos = event->client_pos = event->screen_pos;

  if( !_resize_window.expired() )
  {
    switch( ea.getEventType() )
    {
      case osgGA::GUIEventAdapter::RELEASE:
        _resize_window.lock()->handleResize(sc::Window::NONE);
        _resize_window.reset();
        break;
      case osgGA::GUIEventAdapter::DRAG:
        _resize_window.lock()->handleResize( _resize,
                                             event->screen_pos - _drag_start );
        return true;
      default:
        return false;
    }
  }

  sc::WindowPtr window_at_cursor = _pointer_grab_window.lock();
  if( !window_at_cursor )
  {
    for( int i = _transform->getNumChildren() - 1; i >= 0; --i )
    {
      osg::Group *element = _transform->getChild(i)->asGroup();

      assert(element);
      assert(element->getUserData());

      sc::WindowPtr window =
        dynamic_cast<sc::Window*>
        (
          static_cast<sc::Element::OSGUserData*>(
            element->getUserData()
          )->element.get()
        );

      if( !window || !window->isCapturingEvents() || !window->isVisible() )
        continue;

      float margin = window->isResizable() ? resize_margin_pos : 0;
      if( window->getScreenRegion().contains( event->getScreenX(),
                                              event->getScreenY(),
                                              margin ) )
      {
        window_at_cursor = window;
        break;
      }
    }
  }

  if( window_at_cursor )
  {
    const SGRect<float>& reg = window_at_cursor->getScreenRegion();

    if(     window_at_cursor->isResizable()
        && (  ea.getEventType() == osgGA::GUIEventAdapter::MOVE
           || ea.getEventType() == osgGA::GUIEventAdapter::PUSH
           || ea.getEventType() == osgGA::GUIEventAdapter::RELEASE
           )
        && !reg.contains( event->getScreenX(),
                          event->getScreenY(),
                          -resize_margin_neg ) )
    {
      if( !_last_cursor )
        _last_cursor = fgGetMouseCursor();

      _resize = 0;

      if( event->getScreenX() <= reg.l() + resize_corner )
        _resize |= sc::Window::LEFT;
      else if( event->getScreenX() >= reg.r() - resize_corner )
        _resize |= sc::Window::RIGHT;

      if( event->getScreenY() <= reg.t() + resize_corner )
        _resize |= sc::Window::TOP;
      else if( event->getScreenY() >= reg.b() - resize_corner )
        _resize |= sc::Window::BOTTOM;

      static const int cursor_mapping[] =
      {
        0, MOUSE_CURSOR_LEFTSIDE, MOUSE_CURSOR_RIGHTSIDE, 0,
        MOUSE_CURSOR_TOPSIDE, MOUSE_CURSOR_TOPLEFT, MOUSE_CURSOR_TOPRIGHT, 0,
        MOUSE_CURSOR_BOTTOMSIDE, MOUSE_CURSOR_BOTTOMLEFT, MOUSE_CURSOR_BOTTOMRIGHT,
      };

      if( !cursor_mapping[_resize] )
        return false;

      fgSetMouseCursor(cursor_mapping[_resize]);

      if( ea.getEventType() == osgGA::GUIEventAdapter::PUSH )
      {
        _resize_window = window_at_cursor;
        _drag_start = event->screen_pos;

        window_at_cursor->raise();
        window_at_cursor->handleResize(_resize | sc::Window::INIT);
      }

      return true;
    }
  }

  if( _last_cursor )
  {
    fgSetMouseCursor(_last_cursor);
    _last_cursor = 0;
    return true;
  }

  sc::WindowPtr target_window = window_at_cursor;
  switch( ea.getEventType() )
  {
    case osgGA::GUIEventAdapter::PUSH:
      _last_push = window_at_cursor;
      event->type = sc::Event::MOUSE_DOWN;
      break;
    case osgGA::GUIEventAdapter::SCROLL:
      switch( ea.getScrollingMotion() )
      {
        case osgGA::GUIEventAdapter::SCROLL_UP:
          event->delta.y() = 1;
          break;
        case osgGA::GUIEventAdapter::SCROLL_DOWN:
          event->delta.y() = -1;
          break;
        default:
          return false;
      }

      // osg sends two events for every scrolling motion. We don't need
      // duplicate events, so lets ignore the second event with the same
      // timestamp.
      if( _last_scroll_time == ea.getTime() )
        return window_at_cursor ? true : false;
      _last_scroll_time = ea.getTime();

      event->type = sc::Event::WHEEL;
      break;
    case osgGA::GUIEventAdapter::MOVE:
    {
      sc::WindowPtr last_mouse_over = _last_mouse_over.lock();
      if( last_mouse_over != window_at_cursor && last_mouse_over )
      {
        sc::MouseEventPtr move_event( new sc::MouseEvent(*event) );
        move_event->type = sc::Event::MOUSE_LEAVE;
        move_event->client_pos -= toOsg(last_mouse_over->getPosition());
        move_event->local_pos = move_event->client_pos;

        last_mouse_over->handleEvent(move_event);
      }
      _last_mouse_over = window_at_cursor;
      event->type = sc::Event::MOUSE_MOVE;
      break;
    }
    case osgGA::GUIEventAdapter::RELEASE:
    {
      event->type = sc::Event::MOUSE_UP;

      sc::WindowPtr last_push = _last_push.lock();
      _last_push.reset();

      if( last_push && last_push != target_window )
      {
        // Leave old window
        sc::MouseEventPtr leave_event( new sc::MouseEvent(*event) );
        leave_event->type = sc::Event::MOUSE_LEAVE;
        leave_event->client_pos -= toOsg(last_push->getPosition());
        leave_event->local_pos = leave_event->client_pos;

        last_push->handleEvent(leave_event);
      }
      break;
    }
    case osgGA::GUIEventAdapter::DRAG:
      target_window = _last_push.lock();
      event->type = sc::Event::DRAG;
      break;

    default:
      return false;
  }

  if( target_window )
  {
    event->client_pos -= toOsg(target_window->getPosition());
    event->local_pos = event->client_pos;
    return target_window->handleEvent(event);
  }
  else
    return handleRootEvent(event);
}

//------------------------------------------------------------------------------
bool DesktopGroup::handleKeyboard(const osgGA::GUIEventAdapter& ea)
{
  if( !_transform->getNumChildren() || !_handle_events )
    return false;

  sc::KeyboardEventPtr event(new sc::KeyboardEvent(ea));

  // Detect key repeat (of non modifier keys)
  if( !event->isModifier() )
  {
    if( event->getType() == sc::Event::KEY_DOWN )
    {
      if( event->keyCode() == _last_key_down_no_mod )
        event->setRepeat(true);
      _last_key_down_no_mod = event->keyCode();
    }
    else
    {
      if( event->keyCode() == _last_key_down_no_mod )
      _last_key_down_no_mod = -1;
    }
  }

  sc::WindowPtr active_window = _focus_window.lock();
  bool handled = handleEvent(event, active_window);

  if(    event->getType() == sc::Event::KEY_DOWN
      && !event->defaultPrevented()
      && event->isPrint() )
  {
    sc::KeyboardEventPtr keypress( new sc::KeyboardEvent(*event) );
    keypress->type = sc::Event::KEY_PRESS;

    handled |= handleEvent(keypress, active_window);
  }

  return handled;
}

//------------------------------------------------------------------------------
bool DesktopGroup::handleEvent( const sc::EventPtr& event,
                                const sc::WindowPtr& active_window )
{
  return active_window
       ? active_window->handleEvent(event)
       : handleRootEvent(event);
}

//------------------------------------------------------------------------------
bool DesktopGroup::handleRootEvent(const sc::EventPtr& event)
{
  sc::Element::handleEvent(event);

  // stopPropagation() on DesktopGroup stops propagation to internal event
  // handling.
  return event->propagation_stopped;
}

//------------------------------------------------------------------------------
void DesktopGroup::handleResize(int x, int y, int width, int height)
{
  if( _width == width && _height == height )
    return;

  _width = width;
  _height = height;

  // Origin should be at top left corner, therefore we need to mirror the y-axis
  _transform->setMatrix(osg::Matrix(
    1,  0, 0, 0,
    0, -1, 0, 0,
    0,  0, 1, 0,
    0, _height, 0, 1
  ));
}

//------------------------------------------------------------------------------
void DesktopGroup::handleMouseMode(SGPropertyNode* node)
{
  // pass-through indicates events should pass through to the UI
  _handle_events = fgGetNode("/input/mice/mouse[0]/mode", node->getIntValue())
                     ->getBoolValue("pass-through");
}

//------------------------------------------------------------------------------
GUIMgr::GUIMgr()
{

}

//------------------------------------------------------------------------------
sc::WindowPtr GUIMgr::createWindow(const std::string& name)
{
  sc::WindowPtr window = _desktop->createChild<sc::Window>(name);
  if( name.empty() )
    window->set<std::string>
    (
      "id",
      boost::lexical_cast<std::string>(window->getProps()->getIndex())
    );
  return window;
}

//------------------------------------------------------------------------------
void GUIMgr::init()
{
  if( _desktop && _event_handler )
  {
    SG_LOG(SG_GUI, SG_WARN, "GUIMgr::init() already initialized.");
    return;
  }

  DesktopPtr desktop( new DesktopGroup );
  desktop->handleResize
  (
    0,
    0,
    fgGetInt("/sim/startup/xsize"),
    fgGetInt("/sim/startup/ysize")
  );
  _desktop = desktop;

  _event_handler = new GUIEventHandler(desktop);
  globals->get_renderer()
         ->getViewer()
         ->getEventHandlers()
         // GUI is on top of everything so lets install as first event handler
         .push_front( _event_handler );

  sc::Canvas::addPlacementFactory
  (
    "window",
    boost::bind(&GUIMgr::addWindowPlacement, this, _1, _2)
  );

  _desktop->getProps()->fireCreatedRecursive();
}

//------------------------------------------------------------------------------
void GUIMgr::shutdown()
{
  if( !_desktop && !_event_handler )
  {
    SG_LOG(SG_GUI, SG_WARN, "GUIMgr::shutdown() not running.");
    return;
  }

  sc::Canvas::removePlacementFactory("window");

  if( _desktop )
  {
    _desktop->destroy();
    _desktop.reset();
  }

  if( _event_handler )
  {
    globals->get_renderer()
           ->getViewer()
           ->removeEventHandler( _event_handler );
    _event_handler = 0;
  }
}

//------------------------------------------------------------------------------
void GUIMgr::update(double dt)
{
  _desktop->update(dt);
}

//------------------------------------------------------------------------------
sc::GroupPtr GUIMgr::getDesktop()
{
  return _desktop;
}

//------------------------------------------------------------------------------
void GUIMgr::setInputFocus(const simgear::canvas::WindowPtr& window)
{
  static_cast<DesktopGroup*>(_desktop.get())->setFocusWindow(window);
}

//------------------------------------------------------------------------------
bool GUIMgr::grabPointer(const sc::WindowPtr& window)
{
  return static_cast<DesktopGroup*>(_desktop.get())->grabPointer(window);
}

//------------------------------------------------------------------------------
void GUIMgr::ungrabPointer(const sc::WindowPtr& window)
{
  static_cast<DesktopGroup*>(_desktop.get())->ungrabPointer(window);
}

//------------------------------------------------------------------------------
sc::Placements
GUIMgr::addWindowPlacement( SGPropertyNode* placement,
                            sc::CanvasPtr canvas )
{
  const std::string& id = placement->getStringValue("id");

  sc::Placements placements;
  sc::WindowPtr window = _desktop->getChild<sc::Window>(id);
  if( window )
  {
    window->setCanvasContent(canvas);
    placements.push_back(
      sc::PlacementPtr(
        new WindowPlacement(placement, window, canvas)
    ));
  }
  return placements;
}
