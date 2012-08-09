// An image on the canvas
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

#include "CanvasImage.hxx"

#include <osgDB/ReadFile>

#include <Canvas/canvas.hxx>
#include <Canvas/canvas_mgr.hxx>
#include <Canvas/property_helper.hxx>
#include <osg/Array>
#include <osg/Geometry>
#include <osg/PrimitiveSet>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <boost/algorithm/string/predicate.hpp>

/**
 * Callback to enable/disable rendering of canvas displayed inside windows or
 * other canvases.
 */
class CullCallback:
  public osg::Drawable::CullCallback
{
  public:
    CullCallback(Canvas::CameraCullCallback* camera_cull);

  private:
    Canvas::CameraCullCallback *_camera_cull;

    virtual bool cull( osg::NodeVisitor* nv,
                       osg::Drawable* drawable,
                       osg::RenderInfo* renderInfo ) const;
};

//------------------------------------------------------------------------------
CullCallback::CullCallback(Canvas::CameraCullCallback* camera_cull):
  _camera_cull( camera_cull )
{

}

//------------------------------------------------------------------------------
bool CullCallback::cull( osg::NodeVisitor* nv,
                         osg::Drawable* drawable,
                         osg::RenderInfo* renderInfo ) const
{
  _camera_cull->enableRendering();

  // TODO check if window/image should be culled
  return false;
}

namespace canvas
{
  //----------------------------------------------------------------------------
  Image::Image(SGPropertyNode_ptr node):
    Element(node, COLOR_FILL | BOUNDING_BOX),
    _texture(new osg::Texture2D),
    _node_src_rect( node->getNode("source", 0, true) ),
    _src_rect(0,0,0,0),
    _region(0,0,0,0)
  {
    _geom = new osg::Geometry;
    _geom->setUseDisplayList(false);

    osg::StateSet *stateSet = _geom->getOrCreateStateSet();
    stateSet->setTextureAttributeAndModes(0, _texture.get());
    stateSet->setDataVariance(osg::Object::STATIC);

    // allocate arrays for the image
    _vertices = new osg::Vec3Array(4);
    _vertices->setDataVariance(osg::Object::STATIC);
    _geom->setVertexArray(_vertices);

    _texCoords = new osg::Vec2Array(4);
    _texCoords->setDataVariance(osg::Object::STATIC);
    _geom->setTexCoordArray(0, _texCoords);

    _colors = new osg::Vec4Array(4);
    _colors->setDataVariance(osg::Object::STATIC);
    _geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    _geom->setColorArray(_colors);

    osg::DrawArrays* prim = new osg::DrawArrays(osg::PrimitiveSet::QUADS);
    prim->set(osg::PrimitiveSet::QUADS, 0, 4);
    prim->setDataVariance(osg::Object::STATIC);
    _geom->addPrimitiveSet(prim);

    setDrawable(_geom);
  }

  //----------------------------------------------------------------------------
  Image::~Image()
  {

  }

  //----------------------------------------------------------------------------
  void Image::update(double dt)
  {
    Element::update(dt);

    if( _attributes_dirty & DEST_SIZE )
    {
      (*_vertices)[0].set(_region.l(), _region.t(), 0);
      (*_vertices)[1].set(_region.r(), _region.t(), 0);
      (*_vertices)[2].set(_region.r(), _region.b(), 0);
      (*_vertices)[3].set(_region.l(), _region.b(), 0);
      _vertices->dirty();

      _attributes_dirty &= ~DEST_SIZE;
      _geom->dirtyBound();
    }

    if( _attributes_dirty & SRC_RECT )
    {
      double u0 = _src_rect.l(),
             u1 = _src_rect.r(),
             v0 = _src_rect.b(),
             v1 = _src_rect.t();

      if( !_node_src_rect->getBoolValue("normalized", true) )
      {
        osg::Texture2D *texture = !_canvas.expired()
                                ? _canvas.lock()->getTexture()
                                : _texture.get();

        int texWidth = texture->getTextureWidth();
        int texHeight = texture->getTextureHeight();

        u0 /= texWidth;
        u1 /= texWidth;
        v0 /= texHeight;
        v1 /= texHeight;
      }

      (*_texCoords)[0].set(u0, v0);
      (*_texCoords)[1].set(u1, v0);
      (*_texCoords)[2].set(u1, v1);
      (*_texCoords)[3].set(u0, v1);
      _texCoords->dirty();

      _attributes_dirty &= ~SRC_RECT;
    }
  }

  //----------------------------------------------------------------------------
  void Image::setCanvas(CanvasPtr canvas)
  {
    _canvas = canvas;
    _geom->getOrCreateStateSet()
         ->setTextureAttribute(0, canvas ? canvas->getTexture() : 0);
    _geom->setCullCallback(
      canvas ? new CullCallback(canvas->getCameraCullCallback()) : 0
    );
  }

  //----------------------------------------------------------------------------
  CanvasWeakPtr Image::getCanvas() const
  {
    return _canvas;
  }

  //----------------------------------------------------------------------------
  void Image::setImage(osg::Image *img)
  {
    // remove canvas...
    setCanvas( CanvasPtr() );

    _texture->setImage(img);
    _geom->getOrCreateStateSet()
         ->setTextureAttributeAndModes(0, _texture);
  }

  //----------------------------------------------------------------------------
  const Rect<float>& Image::getRegion() const
  {
    return _region;
  }

  //----------------------------------------------------------------------------
  void Image::valueChanged(SGPropertyNode *node)
  {
    if( node->getParent() == _node_src_rect )
    {
      _attributes_dirty |= SRC_RECT;

      if( node->getNameString() == "left" )
        _src_rect.setLeft( node->getFloatValue() );
      else if( node->getNameString() == "right" )
        _src_rect.setRight( node->getFloatValue() );
      else if( node->getNameString() == "top" )
        _src_rect.setTop( node->getFloatValue() );
      else if( node->getNameString() == "bottom" )
        _src_rect.setBottom( node->getFloatValue() );
    }
    else
      Element::valueChanged(node);
  }

  //----------------------------------------------------------------------------
  void Image::childChanged(SGPropertyNode* child)
  {
    const std::string& name = child->getNameString();

    if( name == "x" )
    {
      _region.setX( child->getFloatValue() );
      _attributes_dirty |= DEST_SIZE;
    }
    else if( name == "y" )
    {
      _region.setY( child->getFloatValue() );
      _attributes_dirty |= DEST_SIZE;
    }
    else if( name == "size" )
    {
      if( child->getIndex() == 0 )
        _region.setWidth( child->getFloatValue() );
      else
        _region.setHeight( child->getFloatValue() );

      _attributes_dirty |= DEST_SIZE;
    }
    else if( name == "file" )
    {
      static const std::string CANVAS_PROTOCOL = "canvas://";
      const std::string& path = child->getStringValue();

      if( boost::starts_with(path, CANVAS_PROTOCOL) )
      {
        CanvasMgr* canvas_mgr =
          dynamic_cast<CanvasMgr*>(globals->get_subsystem("Canvas"));
        if( !canvas_mgr )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: Failed to get CanvasMgr");
          return;
        }

        const SGPropertyNode* canvas_node =
          canvas_mgr->getPropertyRoot()
                    ->getParent()
                    ->getNode( path.substr(CANVAS_PROTOCOL.size()) );
        if( !canvas_node )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: No such canvas: " << path);
          return;
        }

        // TODO add support for other means of addressing canvases (eg. by
        // name)
        CanvasPtr canvas = canvas_mgr->getCanvas( canvas_node->getIndex() );
        if( !canvas )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: Invalid canvas: " << path);
          return;
        }

        setCanvas(canvas);
      }
      else
      {
        SGPath tpath = globals->resolve_ressource_path(path);
        if( tpath.isNull() || !tpath.exists() )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: No such image: " << path);
          return;
        }

        setImage( osgDB::readImageFile(tpath.c_str()) );
      }
    }
  }

  //----------------------------------------------------------------------------
  void Image::colorFillChanged(const osg::Vec4& color)
  {
    for( int i = 0; i < 4; ++i )
      (*_colors)[i] = color;
    _colors->dirty();
  }

} // namespace canvas
