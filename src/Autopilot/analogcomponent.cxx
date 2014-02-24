// analogcomponent.cxx - Base class for analog autopilot components
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
#include "analogcomponent.hxx"
#include <Main/fg_props.hxx>

using namespace FGXMLAutopilot;

AnalogComponent::AnalogComponent() :
  Component(),
  _feedback_if_disabled(false),
  _passive_mode( fgGetNode("/autopilot/locks/passive-mode", true) )
{
}

double AnalogComponent::clamp( double value ) const
{
    //If this is a periodical value, normalize it into our domain 
    // before clamping
    if( _periodical )
      value = _periodical->normalize( value );

    // clamp, if either min or max is defined
    if( _minInput.size() + _maxInput.size() > 0 ) {
        double d = _maxInput.get_value();
        if( value > d ) value = d;
        d = _minInput.get_value();
        if( value < d ) value = d;
    }
    return value;
}

bool AnalogComponent::configure( SGPropertyNode& cfg_node,
                                 const std::string& cfg_name,
                                 SGPropertyNode& prop_root )
{
  if( cfg_name == "feedback-if-disabled" )
  {
    _feedback_if_disabled = cfg_node.getBoolValue();
    return true;
  } 

  if( cfg_name == "output" )
  {
    // grab all <prop> and <property> childs.
    bool found = false;
    for( int i = 0; i < cfg_node.nChildren(); ++i )
    {
      SGPropertyNode& child = *cfg_node.getChild(i);
      const std::string& name = child.getNameString();

      // Allow "prop" for backwards compatiblity
      if( name != "property" && name != "prop" )
        continue;

      _output_list.push_back( prop_root.getNode(child.getStringValue(), true) );
      found = true;
    }

    // no <prop> elements, text node of <output> is property name
    if( !found )
      _output_list.push_back
      (
        prop_root.getNode(cfg_node.getStringValue(), true)
      );

    return true;
  }

  if( cfg_name == "input" )
  {
    _valueInput.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  }

  if( cfg_name == "reference" )
  {
    _referenceInput.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  }

  if( cfg_name == "min" || cfg_name == "u_min" )
  {
    _minInput.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  }

  if( cfg_name == "max" || cfg_name == "u_max" )
  {
    _maxInput.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  }

  if( cfg_name == "period" )
  {
    _periodical = new PeriodicalValue(prop_root, cfg_node);
    return true;
  }

  return Component::configure(cfg_node, cfg_name, prop_root);
}
