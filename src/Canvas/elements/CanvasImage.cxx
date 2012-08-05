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

#include <Canvas/property_helper.hxx>
#include <osg/Array>
#include <osg/Geometry>
#include <osg/PrimitiveSet>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

namespace canvas
{
  //----------------------------------------------------------------------------
  Image::Image(SGPropertyNode_ptr node):
    Element(node, COLOR | COLOR_FILL | BOUNDING_BOX),
    _texture(new osg::Texture2D)
  {
    _source_rect = node->getChild("source");
    
    _geom = new osg::Geometry;
    _geom->setUseDisplayList(false);
  
    osg::StateSet *stateSet = _geom->getOrCreateStateSet();
    stateSet->setTextureAttributeAndModes(0, _texture.get());
    stateSet->setDataVariance(osg::Object::STATIC);

    // allocate arrays for the image
    _vertices = new osg::Vec2Array;
    _vertices->setDataVariance(osg::Object::STATIC);
    _vertices->reserve(4);
    _geom->setVertexArray(_vertices);
  
    _texCoords = new osg::Vec2Array;
    _texCoords->setDataVariance(osg::Object::STATIC);
    _texCoords->reserve(4);
    _geom->setTexCoordArray(0, _texCoords);
  
    _colors = new osg::Vec4Array;
    _colors->setDataVariance(osg::Object::STATIC);
    _geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    _geom->setColorArray(_colors);
  
    osg::DrawArrays* prim = new osg::DrawArrays(osg::PrimitiveSet::QUADS);
    prim->set(osg::PrimitiveSet::QUADS, 0, 1);
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

    if( _attributes_dirty & SRC_RECT ) {
      _attributes_dirty &= ~SRC_RECT;
      int texWidth = _texture->getTextureWidth();
      int texHeight = _texture->getTextureHeight();
      
      double x0 = _source_rect->getDoubleValue("left"),
          x1 = _source_rect->getDoubleValue("right"),
          y0 = _source_rect->getDoubleValue("top"),
          y1 =  _source_rect->getDoubleValue("bottom"); 
      double width = x1 - x0, height = y1 - y0;
      
      _vertices->clear();
      _vertices->push_back(osg::Vec2(0, 0));
      _vertices->push_back(osg::Vec2(width, 0));
      _vertices->push_back(osg::Vec2(width, height));
      _vertices->push_back(osg::Vec2(0, height));
      _vertices->dirty();
      
      double u0 = x0 / texWidth,
        u1 = x1 / texWidth,
        v0 = y0 / texHeight,
        v1 = y1 / texHeight;
            
      _texCoords->clear();
      _texCoords->push_back(osg::Vec2(u0, v0));
      _texCoords->push_back(osg::Vec2(u1, v0));
      _texCoords->push_back(osg::Vec2(u1, v1));
      _texCoords->push_back(osg::Vec2(u0, v1));
      _texCoords->dirty();
    }
  }

  //----------------------------------------------------------------------------
  void Image::childChanged(SGPropertyNode* child)
  {
    const std::string& name = child->getNameString();
    
    if (_source_rect == child) 
      _attributes_dirty |= SRC_RECT;
    else if (name == "file") {
        SGPath tpath = globals->resolve_ressource_path(child->getStringValue());
        if (tpath.isNull() || !tpath.exists()) {
            SG_LOG(SG_GL, SG_ALERT, "canvas::Image: No such image: " << child->getStringValue());
        } else {
            _texture->setImage(osgDB::readImageFile(tpath.c_str()));  
            _attributes_dirty |= SRC_RECT;              
        }
    } 
  }

  //----------------------------------------------------------------------------
  void Image::colorChanged(const osg::Vec4& color)
  {
      _colors->clear();
      for (int i=0; i<4; ++i) {
          _colors->push_back(color);
      }
      _colors->dirty();
  }

  //----------------------------------------------------------------------------
  void Image::colorFillChanged(const osg::Vec4& /*color*/)
  {
  }

} // namespace canvas
