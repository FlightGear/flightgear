// Owner Drawn Gauge helper class
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN
//
// Ported to OSG by Tim Moore - Jun 2007
//
// Heavily modified to be usable for the 2d Canvas by Thomas Geymayer - April 2012
// Supports now multisampling/mipmapping, usage of the stencil buffer and placing
// the texture in the scene by certain filter criteria
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
//
//

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <osg/Texture2D>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/NodeVisitor>
#include <osg/Matrix>
#include <osg/PolygonMode>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/FrameBufferObject> // for GL_DEPTH_STENCIL_EXT on Windows

#include <osgDB/FileNameUtils>

#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>
#include <Scenery/scenery.hxx>
#include "od_gauge.hxx"

#include <cassert>

//------------------------------------------------------------------------------
FGODGauge::FGODGauge():
  _size_x( -1 ),
  _size_y( -1 ),
  _view_width( -1 ),
  _view_height( -1 ),
  _use_image_coords( false ),
  _use_stencil( false ),
  _use_mipmapping( false ),
  _coverage_samples( 0 ),
  _color_samples( 0 ),
  rtAvailable( false )
{
}

//------------------------------------------------------------------------------
FGODGauge::~FGODGauge()
{
  if( camera.valid() )
    globals->get_renderer()->removeCamera(camera.get());
}

//------------------------------------------------------------------------------
void FGODGauge::setSize(int size_x, int size_y)
{
  _size_x = size_x;
  _size_y = size_y < 0 ? size_x : size_y;

  if( texture.valid() )
    texture->setTextureSize(_size_x, _size_x);
}

//----------------------------------------------------------------------------
void FGODGauge::setViewSize(int width, int height)
{
  _view_width = width;
  _view_height = height < 0 ? width : height;

  if( camera )
    updateCoordinateFrame();
}

//------------------------------------------------------------------------------
void FGODGauge::useImageCoords(bool use)
{
  if( use == _use_image_coords )
    return;

  _use_image_coords = use;

  if( texture )
    updateCoordinateFrame();
}

//------------------------------------------------------------------------------
void FGODGauge::useStencil(bool use)
{
  if( use == _use_stencil )
    return;

  _use_stencil = use;

  if( texture )
    updateStencil();
}

//------------------------------------------------------------------------------
void FGODGauge::setSampling( bool mipmapping,
                             int coverage_samples,
                             int color_samples )
{
  if(    _use_mipmapping == mipmapping
      && _coverage_samples == coverage_samples
      && _color_samples == color_samples )
    return;

  _use_mipmapping = mipmapping;

  if( color_samples > coverage_samples )
  {
    SG_LOG
    (
      SG_GL,
      SG_WARN,
      "FGODGauge::setSampling: color_samples > coverage_samples not allowed!"
    );
    color_samples = coverage_samples;
  }

  _coverage_samples = coverage_samples;
  _color_samples = color_samples;

  updateSampling();
}

//------------------------------------------------------------------------------
bool FGODGauge::serviceable(void) 
{
  return rtAvailable;
}

//------------------------------------------------------------------------------
void FGODGauge::allocRT(osg::NodeCallback* camera_cull_callback)
{
  camera = new osg::Camera;
  camera->setDataVariance(osg::Object::DYNAMIC);
  // Only the far camera should trigger this texture to be rendered.
  camera->setNodeMask(simgear::BACKGROUND_BIT);
  camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  camera->setRenderOrder(osg::Camera::PRE_RENDER);
  camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f , 0.0f));
  camera->setClearStencil(0);
  camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT,
                                             osg::Camera::FRAME_BUFFER );

  if( camera_cull_callback )
    camera->setCullCallback(camera_cull_callback);

  updateCoordinateFrame();
  updateStencil();

  osg::StateSet* stateSet = camera->getOrCreateStateSet();
  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
  stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
  stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
  stateSet->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
          osg::PolygonMode::FILL),
          osg::StateAttribute::ON);
  stateSet->setAttributeAndModes(new osg::AlphaFunc(osg::AlphaFunc::GREATER,
          0.0f),
          osg::StateAttribute::ON);
  stateSet->setAttribute(new osg::ShadeModel(osg::ShadeModel::FLAT));
  stateSet->setAttributeAndModes(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA,
          osg::BlendFunc::ONE_MINUS_SRC_ALPHA),
          osg::StateAttribute::ON);
  if( !texture )
  {
    texture = new osg::Texture2D;
    texture->setTextureSize(_size_x, _size_y);
    texture->setInternalFormat(GL_RGBA);
  }

  updateSampling();

  globals->get_renderer()->addCamera(camera.get(), false);
  rtAvailable = true;
}

//------------------------------------------------------------------------------
void FGODGauge::updateCoordinateFrame()
{
  assert( camera );

  if( _view_width < 0 )
    _view_width = _size_x;
  if( _view_height < 0 )
    _view_height = _size_y;

  camera->setViewport(0, 0, _size_x, _size_y);

  if( _use_image_coords )
    camera->setProjectionMatrix(
      osg::Matrix::ortho2D(0, _view_width, _view_height, 0)
    );
  else
    camera->setProjectionMatrix(
      osg::Matrix::ortho2D( -_view_width/2.,  _view_width/2.,
                            -_view_height/2., _view_height/2. )
    );
}

//------------------------------------------------------------------------------
void FGODGauge::updateStencil()
{
  assert( camera );

  GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

  if( _use_stencil)
  {
    camera->attach( osg::Camera::PACKED_DEPTH_STENCIL_BUFFER,
                     GL_DEPTH_STENCIL_EXT );
    mask |= GL_STENCIL_BUFFER_BIT;
  }
  else
  {
    camera->detach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER);
  }

  camera->setClearMask(mask);
}

//------------------------------------------------------------------------------
void FGODGauge::updateSampling()
{
  assert( camera );
  assert( texture );

  texture->setFilter(
    osg::Texture2D::MIN_FILTER,
    _use_mipmapping ? osg::Texture2D::LINEAR_MIPMAP_NEAREST
                    : osg::Texture2D::LINEAR
  );
  camera->attach(
    osg::Camera::COLOR_BUFFER,
    texture.get(),
    0, 0,
    _use_mipmapping,
    _coverage_samples,
    _color_samples
  );
}

/**
 * Replace a texture in the airplane model with the gauge texture.
 */
class ReplaceStaticTextureVisitor:
  public osg::NodeVisitor
{
  public:

    ReplaceStaticTextureVisitor( const char* name,
                                 osg::Texture2D* new_texture ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name( osgDB::getSimpleFileName(name) ),
        _new_texture(new_texture)
    {}

    ReplaceStaticTextureVisitor( const SGPropertyNode* placement,
                                 osg::Texture2D* new_texture,
                                 osg::NodeCallback* cull_callback = 0 ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name( osgDB::getSimpleFileName(
          placement->getStringValue("texture"))
        ),
        _node_name( placement->getStringValue("node") ),
        _parent_name( placement->getStringValue("parent") ),
        _new_texture(new_texture),
        _cull_callback(cull_callback)
    {
      if(    _tex_name.empty()
          && _node_name.empty()
          && _parent_name.empty() )
        SG_LOG
        (
          SG_GL,
          SG_WARN,
          "No filter criterion for replacing texture. "
          " Every texture will be replaced!"
        );
    }

    /**
     * Get a list of groups which have been inserted into the scene graph to
     * replace the given texture
     */
    Placements& getPlacements()
    {
      return _placements;
    }

    virtual void apply(osg::Geode& node)
    {
      simgear::EffectGeode* eg = dynamic_cast<simgear::EffectGeode*>(&node);
      if( !eg )
        return;

      osg::StateSet* ss = eg->getEffect()->getDefaultStateSet();
      if( !ss )
        return;

      osg::Group *parent = node.getParent(0);
      if( !_node_name.empty() && parent->getName() != _node_name )
        return;

      if( !_parent_name.empty() )
      {
        // Traverse nodes upwards starting at the parent node (skip current
        // node)
        const osg::NodePath& np = getNodePath();
        bool found = false;
        for( int i = static_cast<int>(np.size()) - 2; i >= 0; --i )
        {
          const osg::Node* path_segment = np[i];
          const osg::Node* path_parent = path_segment->getParent(0);

          // A node without a name is always the parent of the root node of
          // the model just containing the file name
          if( path_parent && path_parent->getName().empty() )
            return;

          if( path_segment->getName() == _parent_name )
          {
            found = true;
            break;
          }
        }

        if( !found )
          return;
      }

      for( size_t unit = 0; unit < ss->getNumTextureAttributeLists(); ++unit )
      {
        osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>
        (
          ss->getTextureAttribute(unit, osg::StateAttribute::TEXTURE)
        );

        if( !tex || !tex->getImage() || tex == _new_texture )
          continue;

        if( !_tex_name.empty() )
        {
          std::string tex_name = tex->getImage()->getFileName();
          std::string tex_name_simple = osgDB::getSimpleFileName(tex_name);
          if( !osgDB::equalCaseInsensitive(_tex_name, tex_name_simple) )
            continue;
        }

        // insert a new group between the geode an it's parent which overrides
        // the texture
        osg::ref_ptr<osg::Group> group = new osg::Group;
        group->setName("canvas texture group");
        group->addChild(eg);
        parent->removeChild(eg);
        parent->addChild(group);

        if( _cull_callback )
          group->setCullCallback(_cull_callback);

        _placements.push_back(group);

        osg::StateSet* stateSet = group->getOrCreateStateSet();
        stateSet->setTextureAttribute( unit, _new_texture,
                                             osg::StateAttribute::OVERRIDE );
        stateSet->setTextureMode( unit, GL_TEXTURE_2D,
                                        osg::StateAttribute::ON );

        SG_LOG
        (
          SG_GL,
          SG_INFO,
             "Replaced texture '" << _tex_name << "'"
          << " for object '" << parent->getName() << "'"
          << (!_parent_name.empty() ? " with parent '" + _parent_name + "'"
                                    : "")
        );
        return;
      }
    }



  protected:

    std::string _tex_name,      ///<! Name of texture to be replaced
                _node_name,     ///<! Only replace if node name matches
                _parent_name;   ///<! Only replace if any parent node matches
                                ///   given name (all the tree upwards)
    osg::Texture2D     *_new_texture;
    osg::NodeCallback  *_cull_callback;

    Placements _placements;
};

//------------------------------------------------------------------------------
  Placements FGODGauge::set_texture(const char* name, osg::Texture2D* new_texture)
{
  osg::Group* root = globals->get_scenery()->get_aircraft_branch();
  ReplaceStaticTextureVisitor visitor(name, new_texture);
  root->accept(visitor);
  return visitor.getPlacements();
}

//------------------------------------------------------------------------------
Placements FGODGauge::set_texture( const SGPropertyNode* placement,
                             osg::Texture2D* new_texture,
                             osg::NodeCallback* cull_callback )
{
  osg::Group* root = globals->get_scenery()->get_aircraft_branch();
  ReplaceStaticTextureVisitor visitor(placement, new_texture, cull_callback);
  root->accept(visitor);
  return visitor.getPlacements();
}
