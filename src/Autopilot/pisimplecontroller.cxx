// pisimplecontroller.cxx - implementation of a simple PI controller
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

#include "pisimplecontroller.hxx"

using namespace FGXMLAutopilot;

//------------------------------------------------------------------------------
PISimpleController::PISimpleController() :
    AnalogComponent(),
    _int_sum( 0.0 )
{
}

//------------------------------------------------------------------------------
bool PISimpleController::configure( SGPropertyNode& cfg_node,
                                    const std::string& cfg_name,
                                    SGPropertyNode& prop_root )
{
  if( cfg_name == "config" ) {
    Component::configure(prop_root, cfg_node);
    return true;
  }

  if (cfg_name == "Kp") {
    _Kp.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  } 

  if (cfg_name == "Ki") {
    _Ki.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  }

  return AnalogComponent::configure(cfg_node, cfg_name, prop_root);
}

void PISimpleController::update( bool firstTime, double dt )
{
    if ( firstTime ) {
        // we have just been enabled, zero out int_sum
        _int_sum = 0.0;
    }

    if ( _debug ) std::cout << "Updating " << get_name() << std::endl;
    double y_n = _valueInput.get_value();
    double r_n = _referenceInput.get_value();
                  
    double error = r_n - y_n;
    if ( _debug ) std::cout << "input = " << y_n
                      << " reference = " << r_n
                      << " error = " << error
                      << std::endl;

    double prop_comp = clamp(error * _Kp.get_value());
    _int_sum += error * _Ki.get_value() * dt;


    double output = prop_comp + _int_sum;
    double clamped_output = clamp( output );
    if( output != clamped_output ) // anti-windup
      _int_sum = clamped_output - prop_comp;

    if ( _debug ) std::cout << "prop_comp = " << prop_comp
                      << " int_sum = " << _int_sum << std::endl;

    set_output_value( clamped_output );
    if ( _debug ) std::cout << "output = " << clamped_output << std::endl;
}
