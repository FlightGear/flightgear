// Interface for 2D canvas element
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

#include "element.hxx"
#include <Canvas/property_helper.hxx>

#include <osg/Drawable>
#include <osg/Geode>

#include <cassert>
#include <cstring>

namespace canvas
{
  const std::string NAME_TRANSFORM = "tf";
  const std::string NAME_COLOR = "color";
  const std::string NAME_COLOR_FILL = "color-fill";

  //----------------------------------------------------------------------------
  Element::~Element()
  {

  }

  //----------------------------------------------------------------------------
  void Element::update(double dt)
  {
    if( _transform_dirty )
    {
      osg::Matrix m;
      for( size_t i = 0; i < _transform_types.size(); ++i )
      {
        // Skip unused indizes...
        if( _transform_types[i] == TT_NONE )
          continue;

        SGPropertyNode* tf_node = _node->getChild("tf", i, true);

        // Build up the matrix representation of the current transform node
        osg::Matrix tf;
        switch( _transform_types[i] )
        {
          case TT_MATRIX:
            tf = osg::Matrix( tf_node->getDoubleValue("m[0]", 1),
                              tf_node->getDoubleValue("m[1]", 0), 0, 0,

                              tf_node->getDoubleValue("m[2]", 0),
                              tf_node->getDoubleValue("m[3]", 1), 0, 0,

                              0,    0,    1, 0,
                              tf_node->getDoubleValue("m[4]", 0),
                              tf_node->getDoubleValue("m[5]", 0), 0, 1 );
            break;
          case TT_TRANSLATE:
            tf.makeTranslate( osg::Vec3f( tf_node->getDoubleValue("t[0]", 0),
                                          tf_node->getDoubleValue("t[1]", 0),
                                          0 ) );
            break;
          case TT_ROTATE:
            tf.makeRotate( tf_node->getDoubleValue("rot", 0), 0, 0, 1 );
            break;
          case TT_SCALE:
          {
            float sx = tf_node->getDoubleValue("s[0]", 1);
            // sy defaults to sx...
            tf.makeScale( sx, tf_node->getDoubleValue("s[1]", sx), 1 );
            break;
          }
          default:
            break;
        }
        m.postMult( tf );
      }
      _transform->setMatrix(m);
      _transform_dirty = false;
    }

    if( _attributes_dirty & COLOR )
    {
      colorChanged( osg::Vec4( _color[0]->getFloatValue(),
                               _color[1]->getFloatValue(),
                               _color[2]->getFloatValue(),
                               _color[3]->getFloatValue() ) );
      _attributes_dirty &= ~COLOR;
    }

    if( _attributes_dirty & COLOR_FILL )
    {
      colorFillChanged( osg::Vec4( _color_fill[0]->getFloatValue(),
                                   _color_fill[1]->getFloatValue(),
                                   _color_fill[2]->getFloatValue(),
                                   _color_fill[3]->getFloatValue() ) );
      _attributes_dirty &= ~COLOR_FILL;
    }

    if( !_bounding_box.empty() )
    {
      assert( _drawable );

      const osg::BoundingBox& bb = _drawable->getBound();
      _bounding_box[0]->setFloatValue(bb._min.x());
      _bounding_box[1]->setFloatValue(bb._min.y());
      _bounding_box[2]->setFloatValue(bb._max.x());
      _bounding_box[3]->setFloatValue(bb._max.y());
    }
  }

  //----------------------------------------------------------------------------
  osg::ref_ptr<osg::MatrixTransform> Element::getMatrixTransform()
  {
    return _transform;
  }

  //----------------------------------------------------------------------------
  void Element::childAdded(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if( parent == _node )
    {
      if( child->getNameString() == NAME_TRANSFORM )
      {
        if( child->getIndex() >= static_cast<int>(_transform_types.size()) )
          _transform_types.resize( child->getIndex() + 1 );

        _transform_types[ child->getIndex() ] = TT_NONE;
        _transform_dirty = true;
      }
      else
        childAdded(child);
    }
    else if(    parent->getParent() == _node
             && parent->getNameString() == NAME_TRANSFORM )
    {
      assert(parent->getIndex() < static_cast<int>(_transform_types.size()));

      const std::string& name = child->getNameString();

      TransformType& type = _transform_types[parent->getIndex()];

      if(      name == "m" )
        type = TT_MATRIX;
      else if( name == "t" )
        type = TT_TRANSLATE;
      else if( name == "rot" )
        type = TT_ROTATE;
      else if( name == "s" )
        type = TT_SCALE;
      else
        SG_LOG
        (
          SG_GL,
          SG_WARN,
          "Unknown transform element " << child->getPath()
        );

      _transform_dirty = true;
    }
  }

  //----------------------------------------------------------------------------
  void Element::childRemoved(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if( parent != _node )
      return;

    if( child->getNameString() == NAME_TRANSFORM )
    {
      assert(child->getIndex() < static_cast<int>(_transform_types.size()));
      _transform_types[ child->getIndex() ] = TT_NONE;

      while( !_transform_types.empty() && _transform_types.back() == TT_NONE )
        _transform_types.pop_back();

      _transform_dirty = true;
    }
    else
      childRemoved(child);
  }

  //----------------------------------------------------------------------------
  void Element::valueChanged(SGPropertyNode* child)
  {
    SGPropertyNode *parent = child->getParent();
    if( parent->getParent() == _node )
    {
      if( parent->getNameString() == NAME_TRANSFORM )
        _transform_dirty = true;
      else if( !_color.empty() && _color[0]->getParent() == parent )
        _attributes_dirty |= COLOR;
      else if( !_color_fill.empty() && _color_fill[0]->getParent() == parent )
        _attributes_dirty |= COLOR_FILL;
    }
    else if( parent == _node )
    {
      if( child->getNameString() == "update" )
        update(0);
      else
        childChanged(child);
    }
  }

  //----------------------------------------------------------------------------
  Element::Element(SGPropertyNode_ptr node, uint32_t attributes_used):
    _attributes_used( attributes_used ),
    _attributes_dirty( attributes_used ),
    _transform_dirty( false ),
    _transform( new osg::MatrixTransform ),
    _node( node ),
    _drawable( 0 )
  {
    assert( _node );
    _node->addChangeListener(this);

    if( _attributes_used & COLOR )
      linkColorNodes("color", _node, _color, osg::Vec4f(0,1,0,1));

    if( _attributes_used & COLOR_FILL )
      linkColorNodes("color-fill", _node, _color_fill, osg::Vec4f(1,0,1,1));

    SG_LOG
    (
      SG_GL,
      SG_DEBUG,
      "New canvas element " << node->getPath()
    );
  }

  //----------------------------------------------------------------------------
  void Element::setDrawable( osg::Drawable* drawable )
  {
    _drawable = drawable;
    assert( _drawable );

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(_drawable);
    _transform->addChild(geode);

    if( _attributes_used & BOUNDING_BOX )
    {
      SGPropertyNode* bb_node = _node->getChild("bounding-box", 0, true);
      _bounding_box.resize(4);
      _bounding_box[0] = bb_node->getChild("min-x", 0, true);
      _bounding_box[1] = bb_node->getChild("min-y", 0, true);
      _bounding_box[2] = bb_node->getChild("max-x", 0, true);
      _bounding_box[3] = bb_node->getChild("max-y", 0, true);
    }
  }

} // namespace canvas
