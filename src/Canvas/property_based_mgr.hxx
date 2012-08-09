// Base class for all property controlled subsystems
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

#ifndef PROPERTY_BASED_MGR_HXX_
#define PROPERTY_BASED_MGR_HXX_

#include "property_based_element.hxx"
#include <simgear/structure/subsystem_mgr.hxx>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

class PropertyBasedMgr:
  public SGSubsystem,
  public SGPropertyChangeListener
{
  public:
    virtual void init();
    virtual void shutdown();

    virtual void update (double delta_time_sec);

    virtual void childAdded( SGPropertyNode * parent,
                             SGPropertyNode * child );
    virtual void childRemoved( SGPropertyNode * parent,
                               SGPropertyNode * child );

    virtual void elementCreated(PropertyBasedElementPtr element) {}

    virtual const SGPropertyNode* getPropertyRoot() const;

  protected:

    typedef boost::function<PropertyBasedElementPtr(SGPropertyNode*)>
            ElementFactory;

    /** Branch in the property tree for this property managed subsystem */
    SGPropertyNode*      _props;

    /** Property name of managed elements */
    const std::string       _name_elements;

    /** The actually managed elements */
    std::vector<PropertyBasedElementPtr> _elements;

    /** Function object which creates a new element */
    ElementFactory          _element_factory;

    /**
     * @param path_root     Path to property branch used for controlling this
     *                      subsystem
     * @param name_elements The name of the nodes for the managed elements
     */
    PropertyBasedMgr( const std::string& path_root,
                      const std::string& name_elements,
                      ElementFactory element_factory );
    virtual ~PropertyBasedMgr() = 0;

};

#endif /* PROPERTY_BASED_MGR_HXX_ */
