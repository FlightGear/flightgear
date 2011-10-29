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

using std::endl;
using std::cout;

Predictor::Predictor () :
    AnalogComponent(),
    _average(0.0)
{
}

bool Predictor::configure(const string& nodeName, SGPropertyNode_ptr configNode)
{
  SG_LOG( SG_AUTOPILOT, SG_BULK, "Predictor::configure(" << nodeName << ")" << endl );

  if( AnalogComponent::configure( nodeName, configNode ) )
    return true;

  if( nodeName == "config" ) {
    Component::configure( configNode );
    return true;
  }
  
  if (nodeName == "seconds") {
    _seconds.push_back( new InputValue( configNode, 0 ) );
    return true;
  } 

  if (nodeName == "filter-gain") {
    _filter_gain.push_back( new InputValue( configNode, 0 ) );
    return true;
  }

  SG_LOG( SG_AUTOPILOT, SG_BULK, "Predictor::configure(" << nodeName << ") [unhandled]" << endl );
  return false;
}

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


