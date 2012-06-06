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

#ifndef PROPERTY_HELPER_HXX_
#define PROPERTY_HELPER_HXX_

#include <simgear/props/props.hxx>
#include <osg/Vec4>

namespace canvas
{

  /**
   * Get property node with default value
   */
  template<typename T>
  SGPropertyNode* getChildDefault( SGPropertyNode* parent,
                                   const char* name,
                                   T def_val )
  {
    SGPropertyNode* node = parent->getNode(name);
    if( node )
      // also set value for existing nodes to enforce type...
      def_val = getValue<T>(node);
    else
      node = parent->getChild(name, 0, true);

    setValue(node, def_val);
    return node;
  }

  /**
   * Get vector of properties
   */
  template<typename T, typename T_get /* = T */> // TODO use C++11 or traits
  std::vector<T> getVectorFromChildren( const SGPropertyNode* parent,
                                        const char* child_name )
  {
    const simgear::PropertyList& props = parent->getChildren(child_name);
    std::vector<T> values( props.size() );

    for( size_t i = 0; i < props.size(); ++i )
      values[i] = getValue<T_get>(props[i]);

    return values;
  }

  /**
   * @param name    Name of color node
   * @param parent  Parent for color channel nodes
   * @param nodes   Vector to push color nodes into
   * @param def     Default color
   *
   * <name>
   *   <red type="float">def[0] or existing value</red>
   *   <green type="float">def[1] or existing value</green>
   *   <blue type="float">def[2] or existing value</blue>
   *   <alpha type="float">def[3] or existing value</alpha>
   * </name>
   */
  void linkColorNodes( const char* name,
                       SGPropertyNode* parent,
                       std::vector<SGPropertyNode_ptr>& nodes,
                       const osg::Vec4& def = osg::Vec4(0,0,0,1) );

} // namespace canvas

#endif /* PROPERTY_HELPER_HXX_ */
