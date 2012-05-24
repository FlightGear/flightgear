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
  Group::Group(SGPropertyNode* node):
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
      _children[i]->update(dt);

    Element::update(dt);
  }

  //----------------------------------------------------------------------------
  void Group::childAdded(SGPropertyNode* child)
  {
    if( child->getNameString() == "text" )
    {
      _children.push_back( boost::shared_ptr<Element>(new Text(child)) );
      _transform->addChild( _children.back()->getMatrixTransform() );
    }
    else
      std::cout << "New unknown child: " << child->getDisplayName() << std::endl;
  }

  //----------------------------------------------------------------------------
  void Group::childRemoved(SGPropertyNode* child)
  {

  }

} // namespace canvas
