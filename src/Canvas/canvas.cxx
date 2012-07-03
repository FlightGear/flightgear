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
#include <Canvas/property_helper.hxx>

#include <osg/Camera>
#include <osg/Geode>
#include <osgText/Text>

#include <iostream>

//------------------------------------------------------------------------------
/**
 * Callback used to disable/enable rendering to the texture if it is not
 * visible
 */
class CameraCullCallback:
  public osg::NodeCallback
{
  public:

    CameraCullCallback():
      _render( true )
    {}

    /**
     * Enable rendering for the next frame
     */
    void enableRendering()
    {
      _render = true;
    }

  private:

    bool _render;

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
      if( _render )
      {
        traverse(node, nv);
        _render = false;
      }
    }
};

/**
 * This callback is installed on every placement of the canvas in the scene to
 * only render the canvas if at least one placement is visible
 */
class PlacementCullCallback:
  public osg::NodeCallback
{
  public:

    PlacementCullCallback(Canvas* canvas, CameraCullCallback* camera_cull):
      _canvas( canvas ),
      _camera_cull( camera_cull )
    {}

  private:

    Canvas *_canvas;
    CameraCullCallback *_camera_cull;

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
      _camera_cull->enableRendering();
      traverse(node, nv);
    }
};

//------------------------------------------------------------------------------
Canvas::Canvas():
  _size_x(-1),
  _size_y(-1),
  _view_width(-1),
  _view_height(-1),
  _status(0),
  _sampling_dirty(false),
  _color_dirty(true),
  _node(0)
{
  setStatusFlags(MISSING_SIZE_X | MISSING_SIZE_Y);

  CameraCullCallback *camera_callback = new CameraCullCallback;
  _camera_callback = camera_callback;
  _cull_callback = new PlacementCullCallback(this, camera_callback);
}

//------------------------------------------------------------------------------
Canvas::~Canvas()
{
  clearPlacements();

  unbind();
  _node = 0;
}

//------------------------------------------------------------------------------
int Canvas::getStatus() const
{
  return _status;
}

//------------------------------------------------------------------------------
void Canvas::reset(SGPropertyNode* node)
{
  if( node )
    SG_LOG
    (
      SG_GL,
      SG_INFO,
      "Canvas::reset() texture[" << node->getIndex() << "]"
    );

  unbind();

  _node = node;
  setStatusFlags(MISSING_SIZE_X | MISSING_SIZE_Y);

  if( _node )
  {
    _root_group.reset( new canvas::Group(_node) );
    _node->tie
    (
      "size[0]",
      SGRawValueMethods<Canvas, int>( *this, &Canvas::getSizeX,
                                             &Canvas::setSizeX )
    );
    _node->tie
    (
      "size[1]",
      SGRawValueMethods<Canvas, int>( *this, &Canvas::getSizeY,
                                             &Canvas::setSizeY )
    );
    _node->tie
    (
      "view[0]",
      SGRawValueMethods<Canvas, int>( *this, &Canvas::getViewWidth,
                                             &Canvas::setViewWidth )
    );
    _node->tie
    (
      "view[1]",
      SGRawValueMethods<Canvas, int>( *this, &Canvas::getViewHeight,
                                             &Canvas::setViewHeight )
    );
    _node->tie
    (
      "status",
      SGRawValueMethods<Canvas, int>(*this, &Canvas::getStatus)
    );
    _node->tie
    (
      "status-msg",
      SGRawValueMethods<Canvas, const char*>(*this, &Canvas::getStatusMsg)
    );
    _node->addChangeListener(this);

    canvas::linkColorNodes
    (
      "color-background",
      _node,
      _color_background,
      osg::Vec4f(0,0,0,1)
    );
  }
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
      // Remove maybe already existing placements
      clearPlacements(node->getIndex());

    // add new placements
    _placements[node->getIndex()] = _texture.set_texture(
      node,
      _texture.getTexture(),
      _cull_callback
    );
  }
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
int Canvas::getSizeX() const
{
  return _size_x;
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
int Canvas::getSizeY() const
{
  return _size_y;
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
int Canvas::getViewWidth() const
{
  return _view_width;
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
int Canvas::getViewHeight() const
{
  return _view_height;
}

//------------------------------------------------------------------------------
const char* Canvas::getStatusMsg() const
{
  return _status_msg.c_str();
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
    clearPlacements(child->getIndex());
  else
    static_cast<canvas::Element*>(_root_group.get())
      ->childRemoved(parent, child);
}

//----------------------------------------------------------------------------
void Canvas::valueChanged(SGPropertyNode* node)
{
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
  }
  else if( node->getParent() == _node )
  {
    if(    node->getNameString() == "mipmapping"
        || node->getNameString() == "coverage-samples"
        || node->getNameString() == "color-samples" )
      _sampling_dirty = true;
  }

  _root_group->valueChanged(node);
}

//------------------------------------------------------------------------------
void Canvas::setStatusFlags(unsigned int flags, bool set)
{
  if( set )
    _status |= flags;
  else
    _status &= ~flags;

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
void Canvas::clearPlacements(int index)
{
  Placements& placements = _placements.at(index);
  while( !placements.empty() )
  {
    osg::ref_ptr<osg::Group> group = placements.back();
    placements.pop_back();

    assert( group->getNumChildren() == 1 );
    osg::Node *child = group->getChild(0);

    if( group->getNumParents() )
    {
      osg::Group *parent = group->getParent(0);
      parent->addChild(child);
      parent->removeChild(group);
    }

    group->removeChild(child);
  }
}

//------------------------------------------------------------------------------
void Canvas::clearPlacements()
{
  for(size_t i = 0; i < _placements.size(); ++i)
    clearPlacements(i);
  _placements.clear();
}

//------------------------------------------------------------------------------
void Canvas::unbind()
{
  if( !_node )
    return;

  _node->untie("size[0]");
  _node->untie("size[1]");
  _node->untie("view[0]");
  _node->untie("view[1]");
  _node->untie("status");
  _node->untie("status-msg");
  _node->removeChangeListener(this);
}
