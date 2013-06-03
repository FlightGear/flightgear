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
#include <Canvas/window.hxx>

#include <Main/fg_os.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/CameraGroup.hxx>
#include <Viewer/renderer.hxx>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasPlacement.hxx>

#include <osg/BlendFunc>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventHandler>

#include <boost/bind.hpp>

/**
 * Event handler
 */
class GUIEventHandler:
  public osgGA::GUIEventHandler
{
  public:
    GUIEventHandler(GUIMgr* gui_mgr):
      _gui_mgr( gui_mgr )
    {}

    bool handle( const osgGA::GUIEventAdapter& ea,
                 osgGA::GUIActionAdapter& aa,
                 osg::Object*,
                 osg::NodeVisitor* )
    {
      if( ea.getHandled() )
        return false;
      return _gui_mgr->handleEvent(ea);
    }

  protected:
    GUIMgr *_gui_mgr;
};

/**
 * Track a canvas placement on a window
 */
class WindowPlacement:
  public simgear::canvas::Placement
{
  public:
    WindowPlacement( SGPropertyNode* node,
                     canvas::WindowPtr window,
                     simgear::canvas::CanvasPtr canvas ):
      Placement(node),
      _window(window),
      _canvas(canvas)
    {}

    /**
     * Remove placement from window
     */
    virtual ~WindowPlacement()
    {
      canvas::WindowPtr window = _window.lock();
      simgear::canvas::CanvasPtr canvas = _canvas.lock();

      if( window && canvas && canvas == window->getCanvas().lock() )
        window->setCanvas( simgear::canvas::CanvasPtr() );
    }

  private:
    canvas::WindowWeakPtr _window;
    simgear::canvas::CanvasWeakPtr _canvas;
};

/**
 * Store pointer to window as user data
 */
class WindowUserData:
  public osg::Referenced
{
  public:
    canvas::WindowWeakPtr window;
    WindowUserData(canvas::WindowPtr window):
      window(window)
    {}
};

//------------------------------------------------------------------------------
typedef boost::shared_ptr<canvas::Window> WindowPtr;
WindowPtr windowFactory(SGPropertyNode* node)
{
  return WindowPtr(new canvas::Window(node));
}

//------------------------------------------------------------------------------
GUIMgr::GUIMgr():
  PropertyBasedMgr( fgGetNode("/sim/gui/canvas", true),
                    "window",
                    &windowFactory ),
  _event_handler( new GUIEventHandler(this) ),
  _transform( new osg::MatrixTransform ),
  _cb_mouse_mode( this,
                  &GUIMgr::handleMouseMode,
                  fgGetNode("/devices/status/mice/mouse[0]/mode") ),
  _handle_events(true),
  _width(_props, "size[0]"),
  _height(_props, "size[1]"),
  _resize(canvas::Window::NONE),
  _last_cursor(MOUSE_CURSOR_NONE),
  _last_scroll_time(0)
{
  _width = _height = -1;

  osg::Camera* camera =
    flightgear::getGUICamera( flightgear::CameraGroup::getDefault() );
  assert(camera);
  camera->addChild(_transform);

  simgear::canvas::Canvas::addPlacementFactory
  (
    "window",
    boost::bind(&GUIMgr::addPlacement, this, _1, _2)
  );

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
}

//------------------------------------------------------------------------------
canvas::WindowPtr GUIMgr::createWindow(const std::string& name)
{
  return boost::static_pointer_cast<canvas::Window>( createElement(name) );
}

//------------------------------------------------------------------------------
void GUIMgr::init()
{
  handleResize
  (
    0,
    0,
    fgGetInt("/sim/startup/xsize"),
    fgGetInt("/sim/startup/ysize")
  );

  PropertyBasedMgr::init();

  globals->get_renderer()
         ->getViewer()
         ->getEventHandlers()
         // GUI is on top of everything so lets install as first event handler
         .push_front( _event_handler );
}

//------------------------------------------------------------------------------
void GUIMgr::shutdown()
{
  PropertyBasedMgr::shutdown();

  globals->get_renderer()
         ->getViewer()
         ->removeEventHandler( _event_handler );
}

//------------------------------------------------------------------------------
bool GUIMgr::handleEvent(const osgGA::GUIEventAdapter& ea)
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

//------------------------------------------------------------------------------
void GUIMgr::elementCreated(simgear::PropertyBasedElementPtr element)
{
  canvas::WindowPtr window =
    boost::static_pointer_cast<canvas::Window>(element);

  size_t layer_index = std::max(0, window->getProps()->getIntValue("layer", 1));
  osg::Group *layer = 0;

  if( layer_index < _transform->getNumChildren() )
  {
    layer = _transform->getChild(layer_index)->asGroup();
    assert(layer);
  }
  else
  {
    while( _transform->getNumChildren() <= layer_index )
    {
      layer = new osg::Group;
      _transform->addChild(layer);
    }
  }
  window->getGroup()->setUserData(new WindowUserData(window));
  layer->addChild(window->getGroup());
}

//------------------------------------------------------------------------------
canvas::WindowPtr GUIMgr::getWindow(size_t i)
{
  return boost::static_pointer_cast<canvas::Window>(_elements[i]);
}

//------------------------------------------------------------------------------
simgear::canvas::Placements
GUIMgr::addPlacement( SGPropertyNode* node,
                      simgear::canvas::CanvasPtr canvas )
{
  int placement_index = node->getIntValue("index", -1);

  simgear::canvas::Placements placements;
  for( size_t i = 0; i < _elements.size(); ++i )
  {
    if( placement_index >= 0 && static_cast<int>(i) != placement_index )
      continue;

    canvas::WindowPtr window = getWindow(i);
    if( !window )
      continue;

    window->setCanvas(canvas);
    placements.push_back(
      simgear::canvas::PlacementPtr(new WindowPlacement(node, window, canvas))
    );
  }
  return placements;
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
bool GUIMgr::handleMouse(const osgGA::GUIEventAdapter& ea)
{
  if( !_transform->getNumChildren() || !_handle_events )
    return false;

  namespace sc = simgear::canvas;
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
        _resize_window.lock()->handleResize(canvas::Window::NONE);
        _resize_window.reset();
        break;
      case osgGA::GUIEventAdapter::DRAG:
        _resize_window.lock()->handleResize(_resize, event->delta);
        return true;
      default:
        return false;
    }
  }

  canvas::WindowPtr window_at_cursor;
  for( int i = _transform->getNumChildren() - 1; i >= 0; --i )
  {
    osg::Group *layer = _transform->getChild(i)->asGroup();
    assert(layer);
    if( !layer->getNumChildren() )
      continue;

    for( int j = layer->getNumChildren() - 1; j >= 0; --j )
    {
      assert(layer->getChild(j)->getUserData());
      canvas::WindowPtr window =
        static_cast<WindowUserData*>(layer->getChild(j)->getUserData())
          ->window.lock();

      if( !window->isCapturingEvents() || !window->isVisible() )
        continue;

      float margin = window->isResizable() ? resize_margin_pos : 0;
      if( window->getRegion().contains( event->getScreenX(),
                                        event->getScreenY(),
                                        margin ) )
      {
        window_at_cursor = window;
        break;
      }
    }

    if( window_at_cursor )
      break;
  }

  if( window_at_cursor )
  {
    const SGRect<float>& reg = window_at_cursor->getRegion();

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
        _resize |= canvas::Window::LEFT;
      else if( event->getScreenX() >= reg.r() - resize_corner )
        _resize |= canvas::Window::RIGHT;

      if( event->getScreenY() <= reg.t() + resize_corner )
        _resize |= canvas::Window::TOP;
      else if( event->getScreenY() >= reg.b() - resize_corner )
        _resize |= canvas::Window::BOTTOM;

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
        window_at_cursor->doRaise();
        window_at_cursor->handleResize( _resize | canvas::Window::INIT,
                                        event->delta );
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

  canvas::WindowPtr target_window = window_at_cursor;
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
      canvas::WindowPtr last_mouse_over = _last_mouse_over.lock();
      if( last_mouse_over != window_at_cursor && last_mouse_over )
      {
        sc::MouseEventPtr move_event( new sc::MouseEvent(*event) );
        move_event->type = sc::Event::MOUSE_LEAVE;

        last_mouse_over->handleMouseEvent(move_event);
      }
      _last_mouse_over = window_at_cursor;
      event->type = sc::Event::MOUSE_MOVE;
      break;
    }
    case osgGA::GUIEventAdapter::RELEASE:
      target_window = _last_push.lock();
      _last_push.reset();
      event->type = sc::Event::MOUSE_UP;
      break;

    case osgGA::GUIEventAdapter::DRAG:
      target_window = _last_push.lock();
      event->type = sc::Event::DRAG;
      break;

    default:
      return false;
  }

  if( target_window )
    return target_window->handleMouseEvent(event);
  else
    return false;
}

//------------------------------------------------------------------------------
void GUIMgr::handleResize(int x, int y, int width, int height)
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
void GUIMgr::handleMouseMode(SGPropertyNode* node)
{
  // pass-through indicates events should pass through to the UI
  _handle_events = fgGetNode("/input/mice/mouse[0]/mode", node->getIntValue())
                     ->getBoolValue("pass-through");
}
