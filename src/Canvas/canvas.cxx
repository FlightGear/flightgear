// The canvas for rendering with the 2d api
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

#include "canvas.hxx"
#include "elements/group.hxx"

#include <Canvas/MouseEvent.hxx>
#include <Canvas/property_helper.hxx>
#include <Main/globals.hxx>
#include <Viewer/CameraGroup.hxx>
#include <Viewer/renderer.hxx>

#include <simgear/scene/util/RenderConstants.hxx>

#include <osg/Camera>
#include <osg/Geode>
#include <osgText/Text>
#include <osgViewer/Viewer>

#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

//----------------------------------------------------------------------------
Canvas::CameraCullCallback::CameraCullCallback():
  _render( true ),
  _render_frame( 0 )
{

}

//----------------------------------------------------------------------------
void Canvas::CameraCullCallback::enableRendering()
{
  _render = true;
}

//----------------------------------------------------------------------------
void Canvas::CameraCullCallback::operator()( osg::Node* node,
                                             osg::NodeVisitor* nv )
{
  if( !_render && nv->getTraversalNumber() != _render_frame )
    return;

  traverse(node, nv);

  _render = false;
  _render_frame = nv->getTraversalNumber();
}

//----------------------------------------------------------------------------
Canvas::CullCallback::CullCallback(CameraCullCallback* camera_cull):
  _camera_cull( camera_cull )
{

}

//----------------------------------------------------------------------------
void Canvas::CullCallback::operator()( osg::Node* node,
                                       osg::NodeVisitor* nv )
{
  if( (nv->getTraversalMask() & simgear::MODEL_BIT) && _camera_cull.valid() )
    _camera_cull->enableRendering();

  traverse(node, nv);
}

//------------------------------------------------------------------------------
Canvas::Canvas(SGPropertyNode* node):
  PropertyBasedElement(node),
  _size_x(-1),
  _size_y(-1),
  _view_width(-1),
  _view_height(-1),
  _status(node, "status"),
  _status_msg(node, "status-msg"),
  _mouse_x(node, "mouse/x"),
  _mouse_y(node, "mouse/y"),
  _mouse_dx(node, "mouse/dx"),
  _mouse_dy(node, "mouse/dy"),
  _mouse_button(node, "mouse/button"),
  _mouse_state(node, "mouse/state"),
  _mouse_mod(node, "mouse/mod"),
  _mouse_scroll(node, "mouse/scroll"),
  _mouse_event(node, "mouse/event"),
  _sampling_dirty(false),
  _color_dirty(true),
  _render_dirty(true),
  _root_group( new canvas::Group(node) ),
  _render_always(false)
{
  // Remove automatically created property listener as we forward them on our
  // own
  _root_group->removeListener();

  _status = 0;
  setStatusFlags(MISSING_SIZE_X | MISSING_SIZE_Y);

  _camera_callback = new CameraCullCallback;
  _cull_callback = new CullCallback(_camera_callback);

  canvas::linkColorNodes
  (
    "color-background",
    _node,
    _color_background,
    osg::Vec4f(0,0,0,1)
  );
}

//------------------------------------------------------------------------------
Canvas::~Canvas()
{

}

//------------------------------------------------------------------------------
void Canvas::update(double delta_time_sec)
{
  if( !_texture.serviceable() )
  {
    if( _status != STATUS_OK )
      return;

    _texture.setSize(_size_x, _size_y);
    _texture.useImageCoords(true);
    _texture.useStencil(true);
    _texture.allocRT(_camera_callback);
    _texture.getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f , 1.0f));
    _texture.getCamera()->addChild(_root_group->getMatrixTransform());

    if( _texture.serviceable() )
    {
      setStatusFlags(STATUS_OK);
    }
    else
    {
      setStatusFlags(CREATE_FAILED);
      return;
    }
  }

  _root_group->update(delta_time_sec);

  _texture.setRender(_render_dirty);

  // Always render if sampling or color has changed
  _render_dirty = _sampling_dirty || _color_dirty;

  if( _sampling_dirty )
  {
    _texture.setSampling(
      _node->getBoolValue("mipmapping"),
      _node->getIntValue("coverage-samples"),
      _node->getIntValue("color-samples")
    );
    _sampling_dirty = false;
  }
  if( _color_dirty )
  {
    _texture.getCamera()->setClearColor
    (
      osg::Vec4( _color_background[0]->getFloatValue(),
                 _color_background[1]->getFloatValue(),
                 _color_background[2]->getFloatValue(),
                 _color_background[3]->getFloatValue() )
    );
    _color_dirty = false;
  }

  while( !_dirty_placements.empty() )
  {
    SGPropertyNode *node = _dirty_placements.back();
    _dirty_placements.pop_back();

    if( node->getIndex() >= static_cast<int>(_placements.size()) )
      // New placement
      _placements.resize(node->getIndex() + 1);
    else
      // Remove possibly existing placements
      _placements[ node->getIndex() ].clear();

    // Get new placements
    PlacementFactoryMap::const_iterator placement_factory =
      _placement_factories.find( node->getStringValue("type", "object") );
    if( placement_factory != _placement_factories.end() )
    {
      canvas::Placements& placements =
        _placements[ node->getIndex() ] =
          placement_factory->second
          (
            node,
            boost::static_pointer_cast<Canvas>(_self.lock())
          );
      node->setStringValue
      (
        "status-msg",
        placements.empty() ? "No match" : "Ok"
      );
    }
    else
      node->setStringValue("status-msg", "Unknown placement type");
  }

  if( _render_always )
    _camera_callback->enableRendering();
}

//------------------------------------------------------------------------------
void Canvas::setSizeX(int sx)
{
  if( _size_x == sx )
    return;
  _size_x = sx;

  // TODO resize if texture already allocated

  if( _size_x <= 0 )
    setStatusFlags(MISSING_SIZE_X);
  else
    setStatusFlags(MISSING_SIZE_X, false);

  // reset flag to allow creation with new size
  setStatusFlags(CREATE_FAILED, false);
}

//------------------------------------------------------------------------------
void Canvas::setSizeY(int sy)
{
  if( _size_y == sy )
    return;
  _size_y = sy;

  // TODO resize if texture already allocated

  if( _size_y <= 0 )
    setStatusFlags(MISSING_SIZE_Y);
  else
    setStatusFlags(MISSING_SIZE_Y, false);

  // reset flag to allow creation with new size
  setStatusFlags(CREATE_FAILED, false);
}

//------------------------------------------------------------------------------
void Canvas::setViewWidth(int w)
{
  if( _view_width == w )
    return;
  _view_width = w;

  _texture.setViewSize(_view_width, _view_height);
}

//------------------------------------------------------------------------------
void Canvas::setViewHeight(int h)
{
  if( _view_height == h )
    return;
  _view_height = h;

  _texture.setViewSize(_view_width, _view_height);
}

//------------------------------------------------------------------------------
bool Canvas::handleMouseEvent(const canvas::MouseEvent& event)
{
  _mouse_x = event.x;
  _mouse_y = event.y;
  _mouse_dx = event.dx;
  _mouse_dy = event.dy;
  _mouse_button = event.button;
  _mouse_state = event.state;
  _mouse_mod = event.mod;
  _mouse_scroll = event.scroll;
  // Always set event type last because all listeners are attached to it
  _mouse_event = event.type;

  return _root_group->handleMouseEvent(event);
}

//------------------------------------------------------------------------------
void Canvas::childAdded( SGPropertyNode * parent,
                         SGPropertyNode * child )
{
  if( parent != _node )
    return;

  if( child->getNameString() == "placement" )
    _dirty_placements.push_back(child);
  else
    static_cast<canvas::Element*>(_root_group.get())
      ->childAdded(parent, child);
}

//------------------------------------------------------------------------------
void Canvas::childRemoved( SGPropertyNode * parent,
                           SGPropertyNode * child )
{
  if( parent != _node )
    return;

  if( child->getNameString() == "placement" )
    _placements[ child->getIndex() ].clear();
  else
    static_cast<canvas::Element*>(_root_group.get())
      ->childRemoved(parent, child);
}

//----------------------------------------------------------------------------
void Canvas::valueChanged(SGPropertyNode* node)
{
  if(    boost::starts_with(node->getNameString(), "status")
      || node->getParent()->getNameString() == "bounding-box" )
    return;

  _render_dirty = true;

  bool handled = true;
  if( node->getParent()->getParent() == _node )
  {
    if(    !_color_background.empty()
        && _color_background[0]->getParent() == node->getParent() )
    {
      _color_dirty = true;
    }
    else if( node->getParent()->getNameString() == "placement" )
    {
      // prevent double updates...
      for( size_t i = 0; i < _dirty_placements.size(); ++i )
      {
        if( node->getParent() == _dirty_placements[i] )
          return;
      }

      _dirty_placements.push_back(node->getParent());
    }
    else
      handled = false;
  }
  else if( node->getParent() == _node )
  {
    if(    node->getNameString() == "mipmapping"
        || node->getNameString() == "coverage-samples"
        || node->getNameString() == "color-samples" )
      _sampling_dirty = true;
    else if( node->getNameString() == "render-always" )
      _render_always = node->getBoolValue();
    else if( node->getNameString() == "size" )
    {
      if( node->getIndex() == 0 )
        setSizeX( node->getIntValue() );
      else if( node->getIndex() == 1 )
        setSizeY( node->getIntValue() );
    }
    else if( node->getNameString() == "view" )
    {
      if( node->getIndex() == 0 )
        setViewWidth( node->getIntValue() );
      else if( node->getIndex() == 1 )
        setViewHeight( node->getIntValue() );
    }
    else if( node->getNameString() == "freeze" )
      _texture.setRender( node->getBoolValue() );
    else
      handled = false;
  }
  else
    handled = false;

  if( !handled )
    _root_group->valueChanged(node);
}

//------------------------------------------------------------------------------
osg::Texture2D* Canvas::getTexture() const
{
  return _texture.getTexture();
}

//------------------------------------------------------------------------------
GLuint Canvas::getTexId() const
{
  osg::Texture2D* tex = _texture.getTexture();
  if( !tex )
    return 0;

//  osgViewer::Viewer::Contexts contexts;
//  globals->get_renderer()->getViewer()->getContexts(contexts);
//
//  if( contexts.empty() )
//    return 0;

  osg::Camera* guiCamera =
    flightgear::getGUICamera(flightgear::CameraGroup::getDefault());

  osg::State* state = guiCamera->getGraphicsContext()->getState(); //contexts[0]->getState();
  if( !state )
    return 0;

  osg::Texture::TextureObject* tobj =
    tex->getTextureObject( state->getContextID() );
  if( !tobj )
    return 0;

  return tobj->_id;
}

//------------------------------------------------------------------------------
Canvas::CameraCullCallbackPtr Canvas::getCameraCullCallback() const
{
  return _camera_callback;
}

//----------------------------------------------------------------------------
Canvas::CullCallbackPtr Canvas::getCullCallback() const
{
  return _cull_callback;
}

//------------------------------------------------------------------------------
void Canvas::addPlacementFactory( const std::string& type,
                                  canvas::PlacementFactory factory )
{
  if( _placement_factories.find(type) != _placement_factories.end() )
    SG_LOG
    (
      SG_GENERAL,
      SG_WARN,
      "Canvas::addPlacementFactory: replace existing factor for type " << type
    );

  _placement_factories[type] = factory;
}

//------------------------------------------------------------------------------
void Canvas::setStatusFlags(unsigned int flags, bool set)
{
  if( set )
    _status = _status | flags;
  else
    _status = _status & ~flags;
  // TODO maybe extend simgear::PropertyObject to allow |=, &= etc.

  if( (_status & MISSING_SIZE_X) && (_status & MISSING_SIZE_Y) )
    _status_msg = "Missing size";
  else if( _status & MISSING_SIZE_X )
    _status_msg = "Missing size-x";
  else if( _status & MISSING_SIZE_Y )
    _status_msg = "Missing size-y";
  else if( _status & CREATE_FAILED )
    _status_msg = "Creating render target failed";
  else if( _status == STATUS_OK && !_texture.serviceable() )
    _status_msg = "Creation pending...";
  else
    _status_msg = "Ok";
}

//------------------------------------------------------------------------------
Canvas::PlacementFactoryMap Canvas::_placement_factories;
