// A group of 2D canvas elements
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

#include "group.hxx"
#include "text.hxx"

namespace canvas
{

  //----------------------------------------------------------------------------
  Group::Group(SGPropertyNode_ptr node):
    Element(node)
  {

  }

  //----------------------------------------------------------------------------
  Group::~Group()
  {

  }

  //----------------------------------------------------------------------------
  void Group::update(double dt)
  {
    for( size_t i = 0; i < _children.size(); ++i )
    {
      if( _children[i] )
        _children[i]->update(dt);
    }

    Element::update(dt);
  }

  //----------------------------------------------------------------------------
  void Group::childAdded(SGPropertyNode* child)
  {
    boost::shared_ptr<Element> element;

    if( child->getNameString() == "text" )
      element.reset( new Text(child) );
    else if( child->getNameString() == "group" )
      element.reset( new Group(child) );
    else
      SG_LOG
      (
        SG_GL,
        SG_WARN,
        "canvas::Group unknown child: " << child->getDisplayName()
      );

    if( !element )
      return;

    // Add to osg scene graph...
    _transform->addChild( element->getMatrixTransform() );

    // ...and build up canvas hierarchy
    size_t index = child->getIndex();

    if( index >= _children.size() )
      _children.resize(index + 1);

    _children[index] = element;
  }

  //----------------------------------------------------------------------------
  void Group::childRemoved(SGPropertyNode* child)
  {
    if(    child->getNameString() == "text"
        || child->getNameString() == "group" )
    {
      size_t index = child->getIndex();

      if( index >= _children.size() )
        SG_LOG
        (
          SG_GL,
          SG_WARN,
          "can't removed unknown child " << child->getDisplayName()
        );
      else
      {
        boost::shared_ptr<Element>& element = _children[index];
        if( element )
          _transform->removeChild(element->getMatrixTransform());
        element.reset();
      }
    }
  }

} // namespace canvas
