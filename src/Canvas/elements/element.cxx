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
#include <Canvas/MouseEvent.hxx>
#include <Canvas/property_helper.hxx>

#include <osg/Drawable>
#include <osg/Geode>

#include <boost/foreach.hpp>

#include <cassert>
#include <cstring>

namespace canvas
{
  const std::string NAME_TRANSFORM = "tf";

  //----------------------------------------------------------------------------
  void Element::removeListener()
  {
    _node->removeChangeListener(this);
  }

  //----------------------------------------------------------------------------
  Element::~Element()
  {
    removeListener();

    BOOST_FOREACH(osg::Group* parent, _transform->getParents())
    {
      parent->removeChild(_transform);
    }
  }

  //----------------------------------------------------------------------------
  void Element::update(double dt)
  {
    if( !_transform->getNodeMask() )
      // Don't do anything if element is hidden
      return;

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
  }

  //----------------------------------------------------------------------------
  bool Element::handleMouseEvent(const canvas::MouseEvent& event)
  {
    // Transform event to local coordinates
    const osg::Matrixd& m = _transform->getInverseMatrix();
    canvas::MouseEvent local_event = event;
    local_event.x = m(0, 0) * event.x + m(1, 0) * event.y + m(3, 0);
    local_event.y = m(0, 1) * event.x + m(1, 1) * event.y + m(3, 1);

    // Drawables have a bounding box...
    if( _drawable )
    {
      if( !_drawable->getBound().contains(local_event.getPos3()) )
        return false;
    }
    // ... for other elements, i.e. groups only a bounding sphere is available
    else if( !_transform->getBound().contains(local_event.getPos3()) )
      return false;

    local_event.dx = m(0, 0) * event.dx + m(1, 0) * event.dy;
    local_event.dy = m(0, 1) * event.dx + m(1, 1) * event.dy;

    return handleLocalMouseEvent(local_event);
  }

  //----------------------------------------------------------------------------
  osg::ref_ptr<osg::MatrixTransform> Element::getMatrixTransform()
  {
    return _transform;
  }

  //----------------------------------------------------------------------------
  void Element::childAdded(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if(    parent == _node
        && child->getNameString() == NAME_TRANSFORM )
    {
      if( child->getIndex() >= static_cast<int>(_transform_types.size()) )
        _transform_types.resize( child->getIndex() + 1 );

      _transform_types[ child->getIndex() ] = TT_NONE;
      _transform_dirty = true;
      return;
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

      _transform_dirty = true;
      return;
    }

    childAdded(child);
  }

  //----------------------------------------------------------------------------
  void Element::childRemoved(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if( parent == _node && child->getNameString() == NAME_TRANSFORM )
    {
      assert(child->getIndex() < static_cast<int>(_transform_types.size()));
      _transform_types[ child->getIndex() ] = TT_NONE;

      while( !_transform_types.empty() && _transform_types.back() == TT_NONE )
        _transform_types.pop_back();

      _transform_dirty = true;
      return;
    }

    childRemoved(child);
  }

  //----------------------------------------------------------------------------
  void Element::valueChanged(SGPropertyNode* child)
  {
    SGPropertyNode *parent = child->getParent();
    if( parent == _node )
    {
      if( setStyle(child) )
        return;
      else if( child->getNameString() == "update" )
        return update(0);
      else if( child->getNameString() == "visible" )
        // TODO check if we need another nodemask
        return _transform->setNodeMask( child->getBoolValue() ? 0xffffffff : 0 );
    }
    else if(   parent->getParent() == _node
            && parent->getNameString() == NAME_TRANSFORM )
    {
      _transform_dirty = true;
      return;
    }

    childChanged(child);
  }

  //----------------------------------------------------------------------------
  void Element::setBoundingBox(const osg::BoundingBox& bb)
  {
    if( _bounding_box.empty() )
    {
      SGPropertyNode* bb_node = _node->getChild("bounding-box", 0, true);
      _bounding_box.resize(4);
      _bounding_box[0] = bb_node->getChild("min-x", 0, true);
      _bounding_box[1] = bb_node->getChild("min-y", 0, true);
      _bounding_box[2] = bb_node->getChild("max-x", 0, true);
      _bounding_box[3] = bb_node->getChild("max-y", 0, true);
    }

    _bounding_box[0]->setFloatValue(bb._min.x());
    _bounding_box[1]->setFloatValue(bb._min.y());
    _bounding_box[2]->setFloatValue(bb._max.x());
    _bounding_box[3]->setFloatValue(bb._max.y());
  }

  //----------------------------------------------------------------------------
  Element::Element( SGPropertyNode_ptr node,
                    const Style& parent_style ):
    _transform_dirty( false ),
    _transform( new osg::MatrixTransform ),
    _node( node ),
    _style( parent_style ),
    _drawable( 0 )
  {
    assert( _node );
    _node->addChangeListener(this);

    SG_LOG
    (
      SG_GL,
      SG_DEBUG,
      "New canvas element " << node->getPath()
    );
  }

  //----------------------------------------------------------------------------
  bool Element::handleLocalMouseEvent(const canvas::MouseEvent& event)
  {
    std::cout << _node->getPath()
              << " local: pos=(" << event.x << "|" << event.y << ") "
              <<         "d=(" << event.dx << "|" << event.dx << ")"
              << std::endl;
    return true;
  }

  //----------------------------------------------------------------------------
  void Element::setDrawable( osg::Drawable* drawable )
  {
    _drawable = drawable;
    assert( _drawable );

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(_drawable);
    _transform->addChild(geode);
  }

  //----------------------------------------------------------------------------
  void Element::setupStyle()
  {
    BOOST_FOREACH( Style::value_type style, _style )
      setStyle(style.second);
  }

  //----------------------------------------------------------------------------
  bool Element::setStyle(const SGPropertyNode* child)
  {
    StyleSetters::const_iterator setter =
      _style_setters.find(child->getNameString());
    if( setter == _style_setters.end() )
      return false;

    setter->second(child);
    return true;
  }

} // namespace canvas
