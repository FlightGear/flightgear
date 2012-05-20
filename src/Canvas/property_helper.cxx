// Some helper functions for accessing the property tree
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

#include "property_helper.hxx"
#include <cassert>

namespace canvas
{
  //----------------------------------------------------------------------------
  void linkColorNodes( const char* name,
                       SGPropertyNode* parent,
                       std::vector<SGPropertyNode_ptr>& nodes,
                       const osg::Vec4& def )
  {
    static const char* channels[] = {"red", "green", "blue", "alpha"};
    static const size_t num_channels = sizeof(channels)/sizeof(channels[0]);

    assert(name);
    assert(parent);

    // Don't tie to allow the usage of aliases
    SGPropertyNode_ptr color = parent->getChild(name, 0, true);

    // We need to be carefull do not get any unitialized nodes or null pointers
    // because while creating the node a valueChanged event will be triggered.
    nodes.clear();
    nodes.reserve(num_channels);

    for( size_t i = 0; i < num_channels; ++i )
      nodes.push_back( getChildDefault(color, channels[i], def[i]) );
  }
}
