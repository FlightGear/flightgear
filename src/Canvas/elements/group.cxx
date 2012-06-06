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
#include "path.hxx"
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
    for( ChildMap::iterator child = _children.begin();
         child != _children.end();
         ++child )
      child->second->update(dt);

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
    else if( child->getNameString() == "path" )
      element.reset( new Path(child) );

    if( !element )
      return;

    // Add to osg scene graph...
    _transform->addChild( element->getMatrixTransform() );
    _children[ child ] = element;
  }

  //----------------------------------------------------------------------------
  void Group::childRemoved(SGPropertyNode* node)
  {
    if(    node->getNameString() == "text"
        || node->getNameString() == "group"
        || node->getNameString() == "path")
    {
      ChildMap::iterator child = _children.find(node);

      if( child == _children.end() )
        SG_LOG
        (
          SG_GL,
          SG_WARN,
          "can't removed unknown child " << node->getDisplayName()
        );
      else
      {
        _transform->removeChild( child->second->getMatrixTransform() );
        _children.erase(child);
      }
    }
  }

} // namespace canvas
