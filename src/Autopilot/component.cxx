// component.cxx - Base class for autopilot components
//
// Written by Torsten Dreyer
// Based heavily on work created by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2010  Torsten Dreyer - Torsten (at) t3r (dot) de
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
//
#include "component.hxx"
#include <Main/fg_props.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/props/condition.hxx>

using namespace FGXMLAutopilot;

Component::Component() :
  _enable_value(NULL),
  _enabled(false),
  _debug(false),
  _honor_passive(false)
{
}

Component::~Component()
{
  delete _enable_value;
}

bool Component::configure( SGPropertyNode_ptr configNode )
{
  for (int i = 0; i < configNode->nChildren(); ++i ) {
    SGPropertyNode_ptr prop;

    SGPropertyNode_ptr child = configNode->getChild(i);
    string cname(child->getName());

    if( configure( cname, child ) )
      continue;

  } // for configNode->nChildren()

  return true;
}

bool Component::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  SG_LOG( SG_AUTOPILOT, SG_BULK, "Component::configure(" << nodeName << ")" << std::endl );

  if ( nodeName == "name" ) {
    _name = configNode->getStringValue();
    return true;
  } 

  if ( nodeName == "debug" ) {
    _debug = configNode->getBoolValue();
    return true;
  }

  if ( nodeName == "enable" ) {
    SGPropertyNode_ptr prop;

    if( (prop = configNode->getChild("condition")) != NULL ) {
      _condition = sgReadCondition(fgGetNode("/"), prop);
      return true;
    } 
    if ( (prop = configNode->getChild( "property" )) != NULL ) {
      _enable_prop = fgGetNode( prop->getStringValue(), true );
    }
       
    if ( (prop = configNode->getChild( "prop" )) != NULL ) {
      _enable_prop = fgGetNode( prop->getStringValue(), true );
    }

    if ( (prop = configNode->getChild( "value" )) != NULL ) {
      delete _enable_value;
      _enable_value = new string(prop->getStringValue());
    }

    if ( (prop = configNode->getChild( "honor-passive" )) != NULL ) {
      _honor_passive = prop->getBoolValue();
    }

    return true;
  } // enable

  SG_LOG( SG_AUTOPILOT, SG_BULK, "Component::configure(" << nodeName << ") [unhandled]" << std::endl );
  return false;
}

bool Component::isPropertyEnabled()
{
    if( _condition )
        return _condition->test();

    if( _enable_prop ) {
        if( _enable_value ) {
            return *_enable_value == _enable_prop->getStringValue();
        } else {
            return _enable_prop->getBoolValue();
        }
    }
    return true;
}

void Component::update( double dt )
{
  bool firstTime = false;
  if( isPropertyEnabled() ) {
    firstTime = !_enabled;
    _enabled = true;
  } else {
    _enabled = false;
  }

  if( _enabled ) update( firstTime, dt );
  else disabled( dt );
}
