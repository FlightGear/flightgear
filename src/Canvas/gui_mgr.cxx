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

class DesktopGroup;
typedef SGSharedPtr<DesktopGroup> DesktopPtr;
typedef SGWeakPtr<DesktopGroup> DesktopWeakPtr;

namespace sc = simgear::canvas;
using osgEA = osgGA::GUIEventAdapter;

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
const float RESIZE_MARGIN_POS = 12;
const float RESIZE_MARGIN_NEG = 2;
const float RESIZE_CORNER = 20;

/**
 * Event handler
 */
class GUIEventHandler:
  public osgGA::GUIEventHandler
{
  public:
    GUIEventHandler(const DesktopWeakPtr& desktop_group);

    bool handle( const osgEA& ea,
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

    sc::WindowPtr windowAtPosition(const osg::Vec2f& screen_pos);
    osg::Vec2f toScreenPos(const osgEA& ea) const;

    bool handleOsgEvent(const osgEA& ea);

  protected:

    friend class GUIMgr;

    SGPropertyChangeCallback<DesktopGroup> _cb_mouse_mode;
    bool                                   _handle_events {true};

    simgear::PropertyObject<int>        _width,
                                        _height;

    sc::WindowWeakPtr _last_push,
                      _last_drag,
                      _last_mouse_over,
                      _resize_window,
                      _focus_window,
                      _pointer_grab_window;

    uint8_t _resize {sc::Window::NONE};
    int     _last_cursor {MOUSE_CURSOR_NONE};
    bool    _drag_finished {false};

    osg::Vec2f  _drag_start,
                _last_mouse_pos;
    double _last_scroll_time {0};

    uint32_t _last_key_down_no_mod {~0u}; // Key repeat for non modifier keys

    bool canHandleInput() const;
    bool handleMouse(const osgEA& ea);
    bool handleKeyboard(const osgEA& ea);

    bool propagateEvent( const sc::EventPtr& event,
                         const sc::WindowPtr& active_window );
    bool propagateRootEvent(const sc::EventPtr& event);

    void handleResize(int x, int y, int width, int height);
    bool handleDrag(const sc::EventPtr& event);
    void finishDrag(const sc::WindowPtr& drag_src, const sc::EventPtr& event);
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
bool GUIEventHandler::handle( const osgEA& ea,
                              osgGA::GUIActionAdapter&,
                              osg::Object*,
                              osg::NodeVisitor* )
{
  if( ea.getHandled() )
    return false;

  DesktopPtr desktop = _desktop.lock();
  return desktop && desktop->handleOsgEvent(ea);
}

//------------------------------------------------------------------------------
DesktopGroup::DesktopGroup():
  Group(sc::CanvasPtr(), fgGetNode("/sim/gui/canvas", true)),
  _cb_mouse_mode( this,
                  &DesktopGroup::handleMouseMode,
                  fgGetNode("/devices/status/mice/mouse[0]/mode") ),
  _width(_node, "size[0]"),
  _height(_node, "size[1]")
{
  auto camera = flightgear::getGUICamera(flightgear::CameraGroup::getDefault());
  if( !camera )
  {
    SG_LOG(SG_GUI, SG_WARN, "DesktopGroup: failed to get GUI camera.");
    return;
  }

  camera->addChild(_scene_group.get());

  osg::StateSet* stateSet = _scene_group->getOrCreateStateSet();
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
sc::WindowPtr DesktopGroup::windowAtPosition(const osg::Vec2f& screen_pos)
{
  for( int i = _scene_group->getNumChildren() - 1; i >= 0; --i )
  {
    osg::Group *element = _scene_group->getChild(i)->asGroup();

    if( !element || !element->getUserData() )
      continue; // TODO warn/log?

    sc::WindowPtr window =
      dynamic_cast<sc::Window*>
      (
        static_cast<sc::Element::OSGUserData*>(
          element->getUserData()
        )->element.get()
      );

    if( !window || !window->isCapturingEvents() || !window->isVisible() )
      continue;

    float margin = window->isResizable() ? RESIZE_MARGIN_POS : 0;
    if( window->getScreenRegion().contains( screen_pos.x(),
                                            screen_pos.y(),
                                            margin ) )
    {
      return window;
    }
  }

  return {};
}

//------------------------------------------------------------------------------
osg::Vec2f DesktopGroup::toScreenPos(const osgEA& ea) const
{
  float x = SGMiscf::round(0.5 * (ea.getXnormalized() + 1) * _width);
  float y = SGMiscf::round(0.5 * (ea.getYnormalized() + 1) * _height);

  if( ea.getMouseYOrientation() != osgEA::Y_INCREASING_DOWNWARDS )
    y = _height - y;

  return {x, y};
}

//------------------------------------------------------------------------------
bool DesktopGroup::handleOsgEvent(const osgEA& ea)
{
  switch( ea.getEventType() )
  {
    case osgEA::PUSH:
    case osgEA::RELEASE:
//    case osgEA::DOUBLECLICK:
//    // DOUBLECLICK doesn't seem to be triggered...
    case osgEA::DRAG:
    case osgEA::MOVE:
    case osgEA::SCROLL:
      return handleMouse(ea);
    case osgEA::KEYDOWN:
    case osgEA::KEYUP:
      return handleKeyboard(ea);
    case osgEA::RESIZE:
      handleResize( ea.getWindowX(),
                    ea.getWindowY(),
                    ea.getWindowWidth(),
                    ea.getWindowHeight() );
      return false; // Let other event handlers also consume resize events
    default:
      return false;
  }
}

//------------------------------------------------------------------------------
bool DesktopGroup::canHandleInput() const
{
  return _handle_events
      && _scene_group.valid()
      && _scene_group->getNumChildren() > 0;
}

//------------------------------------------------------------------------------
bool DesktopGroup::handleMouse(const osgEA& ea)
{
  if( !canHandleInput() )
    return false;

  osg::Vec2f mouse_pos = toScreenPos(ea),
             delta = mouse_pos - _last_mouse_pos;
  _last_mouse_pos = mouse_pos;

  if( auto resize_window = _resize_window.lock() )
  {
    switch( ea.getEventType() )
    {
      case osgEA::RELEASE:
        resize_window->handleResize(sc::Window::NONE);
        _resize_window.reset();
        break;
      case osgEA::DRAG:
        resize_window->handleResize(_resize, mouse_pos - _drag_start);
        return true;
      default:
        // Ignore all other events while resizing
        return true;
    }
  }

  sc::MouseEventPtr event(new sc::MouseEvent(ea));
  event->screen_pos = mouse_pos;
  event->delta = delta;

  if( !_drag_finished && ea.getEventType() == osgEA::DRAG )
    return handleDrag(event);

  if( auto last_drag = _last_drag.lock() )
  {
    if( ea.getEventType() == osgEA::RELEASE )
      finishDrag(last_drag, event);
    else
      // While dragging ignore all other mouse events
      return true;
  }

  sc::WindowPtr window_at_cursor = _pointer_grab_window.lock();
  if( !window_at_cursor )
    window_at_cursor = windowAtPosition(event->screen_pos);

  if( window_at_cursor )
  {
    const SGRect<float>& reg = window_at_cursor->getScreenRegion();

    if(     window_at_cursor->isResizable()
        && !reg.contains( event->getScreenX(),
                          event->getScreenY(),
                          -RESIZE_MARGIN_NEG ) )
    {
      if( !_last_cursor )
        _last_cursor = fgGetMouseCursor();

      _resize = 0;

      if( event->getScreenX() <= reg.l() + RESIZE_CORNER )
        _resize |= sc::Window::LEFT;
      else if( event->getScreenX() >= reg.r() - RESIZE_CORNER )
        _resize |= sc::Window::RIGHT;

      if( event->getScreenY() <= reg.t() + RESIZE_CORNER )
        _resize |= sc::Window::TOP;
      else if( event->getScreenY() >= reg.b() - RESIZE_CORNER )
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

      if( ea.getEventType() == osgEA::PUSH )
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

  switch( ea.getEventType() )
  {
    case osgEA::PUSH:
      _last_push = window_at_cursor;
      _drag_finished = false;
      event->type = sc::Event::MOUSE_DOWN;
      break;
    case osgEA::SCROLL:
      switch( ea.getScrollingMotion() )
      {
        case osgEA::SCROLL_UP:
          event->delta.y() = 1;
          break;
        case osgEA::SCROLL_DOWN:
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

    // If drag has not been handled yet it has been aborted. So let's treat it
    // like a normal mouse movement.
    case osgEA::DRAG:
    case osgEA::MOVE:
    {
      sc::WindowPtr last_mouse_over = _last_mouse_over.lock();
      if( last_mouse_over && last_mouse_over != window_at_cursor )
        last_mouse_over->handleEvent(event->clone(sc::Event::MOUSE_LEAVE));

      _last_mouse_over = window_at_cursor;
      event->type = sc::Event::MOUSE_MOVE;
      break;
    }
    case osgEA::RELEASE:
    {
      sc::WindowPtr last_push = _last_push.lock();
      if( last_push && last_push != window_at_cursor )
      {
        // Leave old window
        last_push->handleEvent(event->clone(sc::Event::MOUSE_LEAVE));
      }

      _last_push.reset();
      event->type = sc::Event::MOUSE_UP;
      break;
    }

    default:
      return false;
  }

  return propagateEvent(event, window_at_cursor);
}

//------------------------------------------------------------------------------
bool DesktopGroup::handleKeyboard(const osgEA& ea)
{
  if( !canHandleInput() )
    return false;

  sc::KeyboardEventPtr event(new sc::KeyboardEvent(ea));

  if( auto drag = _last_drag.lock() )
  {
    if( ea.getKey() == osgEA::KEY_Escape )
      finishDrag(drag, event);

    // While dragging ignore all key events
    return true;
  }

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
      _last_key_down_no_mod = ~0u;
    }
  }

  sc::WindowPtr active_window = _focus_window.lock();
  bool handled = propagateEvent(event, active_window);

  if(    event->getType() == sc::Event::KEY_DOWN
      && !event->defaultPrevented()
      && event->isPrint() )
  {
    handled |= propagateEvent(event->clone(sc::Event::KEY_PRESS), active_window);
  }

  return handled;
}

//------------------------------------------------------------------------------
bool DesktopGroup::propagateEvent( const sc::EventPtr& event,
                                   const sc::WindowPtr& active_window )
{
  return active_window
       ? active_window->handleEvent(event)
       : propagateRootEvent(event);
}

//------------------------------------------------------------------------------
bool DesktopGroup::propagateRootEvent(const sc::EventPtr& event)
{
  handleEvent(event);

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

  if( _scene_group.valid() )
  {
    // Origin should be at top left corner, therefore we need to mirror the
    // y-axis
    _scene_group->setMatrix(osg::Matrix(
      1,  0, 0, 0,
      0, -1, 0, 0,
      0,  0, 1, 0,
      0, _height, 0, 1
    ));
  }
}

//------------------------------------------------------------------------------
bool DesktopGroup::handleDrag(const sc::EventPtr& event)
{
  event->type = sc::Event::DRAG;

  auto drag_window = _last_drag.lock();
  if( !drag_window )
  {
    _last_drag = drag_window = _last_push.lock();

    if( drag_window )
      drag_window->handleEvent(event->clone(sc::Event::DRAG_START));
  }

  // TODO dragover
  return drag_window && drag_window->handleEvent(event);
}

//------------------------------------------------------------------------------
void DesktopGroup::finishDrag( const sc::WindowPtr& drag_src,
                               const sc::EventPtr& event )
{
  drag_src->handleEvent(event->clone(sc::Event::DRAG_END));
  _last_drag.reset();
  _drag_finished = true;
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
    window->set("id", std::to_string(window->getProps()->getIndex()));
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
         ->getView()
         ->getEventHandlers()
         // GUI is on top of everything so lets install as first event handler
         .push_front( _event_handler );

  sc::Canvas::addPlacementFactory
  (
    "window",
   std::bind(&GUIMgr::addWindowPlacement, this, std::placeholders::_1, std::placeholders::_2)
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
           ->getView()
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


// Register the subsystem.
SGSubsystemMgr::Registrant<GUIMgr> registrantGUIMgr(
    SGSubsystemMgr::DISPLAY,
    {{"viewer", SGSubsystemMgr::Dependency::HARD},
     {"FGRenderer", SGSubsystemMgr::Dependency::NONSUBSYSTEM_HARD}});
