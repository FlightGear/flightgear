// predictor.cxx - predict future values
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

#include "predictor.hxx"

using namespace FGXMLAutopilot;

//------------------------------------------------------------------------------
Predictor::Predictor () :
  AnalogComponent(),
  _last_value(0),
  _average(0)
{
}

//------------------------------------------------------------------------------
bool Predictor::configure( SGPropertyNode& cfg_node,
                           const std::string& cfg_name,
                           SGPropertyNode& prop_root )
{
  if( cfg_name == "config" ) {
    Component::configure(prop_root, cfg_node);
    return true;
  }
  
  if (cfg_name == "seconds") {
    _seconds.push_back( new InputValue(prop_root, cfg_node, 0) );
    return true;
  } 

  if (cfg_name == "filter-gain") {
    _filter_gain.push_back( new InputValue(prop_root, cfg_node, 0) );
    return true;
  }

  return AnalogComponent::configure(cfg_node, cfg_name, prop_root);
}

//------------------------------------------------------------------------------
void Predictor::update( bool firstTime, double dt ) 
{
    double ivalue = _valueInput.get_value();

    if ( firstTime ) {
        _last_value = ivalue;
    }

    double current = (ivalue - _last_value)/dt; // calculate current error change (per second)
    _average = dt < 1.0 ? ((1.0 - dt) * _average + current * dt) : current;

    // calculate output with filter gain adjustment
    double output = ivalue + 
           (1.0 - _filter_gain.get_value()) * (_average * _seconds.get_value()) + 
           _filter_gain.get_value() * (current * _seconds.get_value());
    output = clamp( output );
    set_output_value( output );

    _last_value = ivalue;
}
