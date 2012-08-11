// Base class for elements of property controlled subsystems
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

#ifndef PROPERTY_BASED_ELEMENT_HXX_
#define PROPERTY_BASED_ELEMENT_HXX_

#include <Canvas/canvas_fwd.hpp>
#include <simgear/props/props.hxx>

/**
 * Base class for a property controlled element
 */
class PropertyBasedElement:
  public SGPropertyChangeListener
{
  public:
    PropertyBasedElement(SGPropertyNode* node);
    virtual ~PropertyBasedElement();

    virtual void update(double delta_time_sec) = 0;

    SGConstPropertyNode_ptr getProps() const;
    SGPropertyNode_ptr getProps();

  protected:

    friend class PropertyBasedMgr;

    SGPropertyNode_ptr _node;
    PropertyBasedElementWeakPtr _self;
};


#endif /* PROPERTY_BASED_ELEMENT_HXX_ */
