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
  _width(_props, "size[0]"),
  _height(_props, "size[1]")
{
  _width = _height = -1;

  osg::Camera* camera =
    flightgear::getGUICamera( flightgear::CameraGroup::getDefault() );
  assert(camera);
  camera->addChild(_transform);

  osg::Viewport* vp = camera->getViewport();
  handleResize(vp->x(), vp->y(), vp->width(), vp->height());

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
void GUIMgr::init()
{
  PropertyBasedMgr::init();

  globals->get_renderer()
         ->getViewer()
         ->addEventHandler( _event_handler );
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
      return true;
    default:
      return false;
  }
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

//------------------------------------------------------------------------------
bool GUIMgr::handleMouse(const osgGA::GUIEventAdapter& ea)
{
  if( !_transform->getNumChildren() )
    return false;

  namespace sc = simgear::canvas;
  sc::MouseEventPtr event(new sc::MouseEvent);

  event->pos.x() = 0.5 * (ea.getXnormalized() + 1) * _width + 0.5;
  event->pos.y() = 0.5 * (ea.getYnormalized() + 1) * _height + 0.5;
  if(    ea.getMouseYOrientation()
      != osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS )
    event->pos.y() = _height - event->pos.y();

  event->delta.x() = event->pos.x() - _last_x;
  event->delta.y() = event->pos.y() - _last_y;

  _last_x = event->pos.x();
  _last_y = event->pos.y();

  event->button = ea.getButton();
  event->state = ea.getButtonMask();
  event->mod = ea.getModKeyMask();
  //event->scroll = ea.getScrollingMotion();

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
      if( window->getRegion().contains(event->pos.x(), event->pos.y()) )
      {
        window_at_cursor = window;
        break;
      }
    }

    if( window_at_cursor )
      break;
  }

  canvas::WindowPtr target_window = window_at_cursor;
  switch( ea.getEventType() )
  {
    case osgGA::GUIEventAdapter::PUSH:
      _last_push = window_at_cursor;
      event->type = sc::Event::MOUSE_DOWN;
      break;
//    case osgGA::GUIEventAdapter::SCROLL:
//      event->type = sc::Event::SCROLL;
//      break;
    case osgGA::GUIEventAdapter::MOVE:
    {
      canvas::WindowPtr last_mouse_over = _last_mouse_over.lock();
      if( last_mouse_over != window_at_cursor && last_mouse_over )
      {
        sc::MouseEventPtr move_event( new sc::MouseEvent(*event) );
        move_event->type = sc::Event::MOUSE_LEAVE;

        // Let the event position be always relative to the top left window corner
        move_event->pos.x() -= last_mouse_over->getRegion().x();
        move_event->pos.y() -= last_mouse_over->getRegion().y();

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

//    case osgGA::GUIEventAdapter::DRAG:
//      target_window = _last_push.lock();
//      break;

    default:
      return false;
  }

  if( target_window )
  {
    // Let the event position be always relative to the top left window corner
    event->pos.x() -= target_window->getRegion().x();
    event->pos.y() -= target_window->getRegion().y();

    return target_window->handleMouseEvent(event);
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
