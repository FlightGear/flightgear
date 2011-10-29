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

bool AnalogComponent::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  SG_LOG( SG_AUTOPILOT, SG_BULK, "AnalogComponent::configure(" << nodeName << ")" );
  if( Component::configure( nodeName, configNode ) )
    return true;

  if ( nodeName == "feedback-if-disabled" ) {
    _feedback_if_disabled = configNode->getBoolValue();
    return true;
  } 

  if ( nodeName == "output" ) {
    // grab all <prop> and <property> childs
    int found = 0;
    // backwards compatibility: allow <prop> elements
    SGPropertyNode_ptr prop;
    for( int i = 0; (prop = configNode->getChild("prop", i)) != NULL; i++ ) { 
      SGPropertyNode *tmp = fgGetNode( prop->getStringValue(), true );
      _output_list.push_back( tmp );
      found++;
    }
    for( int i = 0; (prop = configNode->getChild("property", i)) != NULL; i++ ) { 
      SGPropertyNode *tmp = fgGetNode( prop->getStringValue(), true );
      _output_list.push_back( tmp );
      found++;
    }

    // no <prop> elements, text node of <output> is property name
    if( found == 0 )
      _output_list.push_back( fgGetNode(configNode->getStringValue(), true ) );

    return true;
  }

  if( nodeName == "input" ) {
    _valueInput.push_back( new InputValue( configNode ) );
    return true;
  }

  if( nodeName == "reference" ) {
    _referenceInput.push_back( new InputValue( configNode ) );
    return true;
  }

  if( nodeName == "min" || nodeName == "u_min" ) {
    _minInput.push_back( new InputValue( configNode ) );
    return true;
  }

  if( nodeName == "max" || nodeName == "u_max" ) {
    _maxInput.push_back( new InputValue( configNode ) );
    return true;
  }

  if( nodeName == "period" ) {
    _periodical = new PeriodicalValue( configNode );
    return true;
  }

  SG_LOG( SG_AUTOPILOT, SG_BULK, "AnalogComponent::configure(" << nodeName << ") [unhandled]" );
  return false;
}
