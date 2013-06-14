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
#include <simgear/scene/util/OsgMath.hxx>

#include <osg/BlendFunc>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventHandler>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

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

      if( window && canvas && canvas == window->getCanvasContent().lock() )
        window->setCanvasContent( simgear::canvas::CanvasPtr() );
    }

  private:
    canvas::WindowWeakPtr _window;
    simgear::canvas::CanvasWeakPtr _canvas;
};

//------------------------------------------------------------------------------
GUIMgr::GUIMgr():
  Group(simgear::canvas::CanvasPtr(), fgGetNode("/sim/gui/canvas", true)),
  _event_handler( new GUIEventHandler(this) ),
  _cb_mouse_mode( this,
                  &GUIMgr::handleMouseMode,
                  fgGetNode("/devices/status/mice/mouse[0]/mode") ),
  _handle_events(true),
  _width(_node, "size[0]"),
  _height(_node, "size[1]"),
  _resize(canvas::Window::NONE),
  _last_cursor(MOUSE_CURSOR_NONE),
  _last_x(-1),
  _last_y(-1),
  _last_scroll_time(0)
{
  // We handle the property listener manually within ::init and ::shutdown.
  removeListener();

  _width = _height = -1;

  osg::Camera* camera =
    flightgear::getGUICamera( flightgear::CameraGroup::getDefault() );
  assert(camera);
  camera->addChild( getMatrixTransform() );

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
  canvas::WindowPtr window = createChild<canvas::Window>(name);
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
  handleResize
  (
    0,
    0,
    fgGetInt("/sim/startup/xsize"),
    fgGetInt("/sim/startup/ysize")
  );

  globals->get_renderer()
         ->getViewer()
         ->getEventHandlers()
         // GUI is on top of everything so lets install as first event handler
         .push_front( _event_handler );

  _node->addChangeListener(this);
  _node->fireCreatedRecursive();
}

//------------------------------------------------------------------------------
void GUIMgr::shutdown()
{
  _node->removeChangeListener(this);

  globals->get_renderer()
         ->getViewer()
         ->removeEventHandler( _event_handler );
}

//------------------------------------------------------------------------------
void GUIMgr::update(double dt)
{
  Group::update(dt);
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
GUIMgr::ElementFactory GUIMgr::getChildFactory(const std::string& type) const
{
  if( type == "window" )
    return &Element::create<canvas::Window>;

  return Group::getChildFactory(type);
}

//------------------------------------------------------------------------------
simgear::canvas::Placements
GUIMgr::addPlacement( SGPropertyNode* node,
                      simgear::canvas::CanvasPtr canvas )
{
  const std::string& id = node->getStringValue("id");

  simgear::canvas::Placements placements;
  canvas::WindowPtr window = getChild<canvas::Window>(id);
  if( window )
  {
    window->setCanvasContent(canvas);
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
    osg::Group *element = _transform->getChild(i)->asGroup();

    assert(element);
    assert(element->getUserData());

    canvas::WindowPtr window =
      boost::static_pointer_cast<canvas::Window>
      (
        static_cast<sc::Element::OSGUserData*>(element->getUserData())->element
      );

    if( !window->isCapturingEvents() || !window->isVisible() )
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
        window_at_cursor->raise();
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
        move_event->client_pos -= toOsg(last_mouse_over->getPosition());
        move_event->local_pos = move_event->client_pos;

        last_mouse_over->handleEvent(move_event);
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
  {
    event->client_pos -= toOsg(target_window->getPosition());
    event->local_pos = event->client_pos;
    return target_window->handleEvent(event);
  }
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
