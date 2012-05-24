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
#include <cassert>

#include <osg/Drawable>

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
  SGPropertyNode* Element::getPropertyNode()
  {
    return _node;
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
            tf = osg::Matrix( tf_node->getDoubleValue("a", 1),
                              tf_node->getDoubleValue("b", 0), 0, 0,

                              tf_node->getDoubleValue("c", 0),
                              tf_node->getDoubleValue("d", 1), 0, 0,

                              0,    0,    1, 0,
                              tf_node->getDoubleValue("e", 0),
                              tf_node->getDoubleValue("f", 0), 0, 1 );
            break;
          case TT_TRANSLATE:
            tf.makeTranslate( osg::Vec3f( tf_node->getDoubleValue("tx", 0),
                                          tf_node->getDoubleValue("ty", 0),
                                          0 ) );
            break;
          case TT_ROTATE:
            tf.makeRotate( tf_node->getDoubleValue("rot", 0), 0, 0, 1 );
            break;
          case TT_SCALE:
          {
            float sx = tf_node->getDoubleValue("sx", 1);
            // sy defaults to sx...
            tf.makeScale( sx, tf_node->getDoubleValue("sy", sx), 1 );
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
                               1 ) );
      _attributes_dirty &= ~COLOR;
    }

    if( _attributes_dirty & COLOR_FILL )
    {
      colorFillChanged( osg::Vec4( _color_fill[0]->getFloatValue(),
                                   _color_fill[1]->getFloatValue(),
                                   _color_fill[2]->getFloatValue(),
                                   1 ) );
      _attributes_dirty &= ~COLOR_FILL;
    }

    if( _drawable && (_attributes_dirty & BOUNDING_BOX) )
    {
      _bounding_box[0]->setFloatValue(_drawable->getBound()._min.x());
      _bounding_box[1]->setFloatValue(_drawable->getBound()._min.y());
      _bounding_box[2]->setFloatValue(_drawable->getBound()._max.x());
      _bounding_box[3]->setFloatValue(_drawable->getBound()._max.y());

      _attributes_dirty &= ~BOUNDING_BOX;
    }
  }

  //----------------------------------------------------------------------------
  osg::ref_ptr<osg::MatrixTransform> Element::getMatrixTransform()
  {
    return _transform;
  }

  //----------------------------------------------------------------------------
  Element::Element(SGPropertyNode* node, uint32_t attributes_used):
    _node( node ),
    _drawable( 0 ),
    _attributes_used( attributes_used ),
    _attributes_dirty( 0 ),
    _transform_dirty( false ),
    _transform( new osg::MatrixTransform )
  {
    assert( _node );
    _node->addChangeListener(this);

    if( _attributes_used & COLOR )
      linkColorNodes("color", _color, osg::Vec4f(0,1,0,1));
    if( _attributes_used & COLOR_FILL )
      linkColorNodes("color-fill", _color_fill);
    if( _attributes_used & BOUNDING_BOX )
    {
      SGPropertyNode* bb = _node->getChild("bounding-box", 0, true);
      _bounding_box[0] = bb->getChild("x-min", 0, true);
      _bounding_box[1] = bb->getChild("y-min", 0, true);
      _bounding_box[2] = bb->getChild("x-max", 0, true);
      _bounding_box[3] = bb->getChild("y-max", 0, true);
    }

    SG_LOG
    (
      SG_GL,
      SG_DEBUG,
      "New canvas element " << node->getPath()
    );
  }

  //----------------------------------------------------------------------------
  void Element::linkColorNodes( const char* name,
                                SGPropertyNode** nodes,
                                const osg::Vec4& def )
  {
    // Don't tie to allow the usage of aliases
    SGPropertyNode* color = _node->getChild(name, 0, true);

    static const char* color_names[] = {"red", "green", "blue"};
    for( size_t i = 0; i < sizeof(color_names)/sizeof(color_names[0]); ++i )
    {
      color->setFloatValue
      (
        color_names[i],
        color->getFloatValue(color_names[i], def[i])
      );
      nodes[i] = color->getChild(color_names[i]);
    }
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

      if(    name == "a" || name == "b" || name == "c"
          || name == "d" || name == "e" || name == "f" )
        type = TT_MATRIX;
      else if( name == "tx" || name == "ty" )
        type = TT_TRANSLATE;
      else if( name == "rot" )
        type = TT_ROTATE;
      else if( name == "sx" || name == "sy" )
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
      else if( parent->getNameString() == NAME_COLOR )
        _attributes_dirty |= COLOR;
      else if( parent->getNameString() == NAME_COLOR_FILL )
        _attributes_dirty |= COLOR_FILL;
    }
    else
      childChanged(child);
  }

} // namespace canvas
