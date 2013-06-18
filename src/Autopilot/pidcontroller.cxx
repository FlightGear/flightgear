// pidcontroller.cxx - implementation of PID controller
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

#include "pidcontroller.hxx"

using namespace FGXMLAutopilot;

using std::endl;
using std::cout;

PIDController::PIDController():
    AnalogComponent(),
    alpha( 0.1 ),
    beta( 1.0 ),
    gamma( 0.0 ),
    ep_n_1( 0.0 ),
    edf_n_1( 0.0 ),
    edf_n_2( 0.0 ),
    u_n_1( 0.0 ),
    desiredTs( 0.0 ),
    elapsedTime( 0.0 )
{
}

/*
 * Roy Vegard Ovesen:
 *
 * Ok! Here is the PID controller algorithm that I would like to see
 * implemented:
 *
 *   delta_u_n = Kp * [ (ep_n - ep_n-1) + ((Ts/Ti)*e_n)
 *               + (Td/Ts)*(edf_n - 2*edf_n-1 + edf_n-2) ]
 *
 *   u_n = u_n-1 + delta_u_n
 *
 * where:
 *
 * delta_u : The incremental output
 * Kp      : Proportional gain
 * ep      : Proportional error with reference weighing
 *           ep = beta * r - y
 *           where:
 *           beta : Weighing factor
 *           r    : Reference (setpoint)
 *           y    : Process value, measured
 * e       : Error
 *           e = r - y
 * Ts      : Sampling interval
 * Ti      : Integrator time
 * Td      : Derivator time
 * edf     : Derivate error with reference weighing and filtering
 *           edf_n = edf_n-1 / ((Ts/Tf) + 1) + ed_n * (Ts/Tf) / ((Ts/Tf) + 1)
 *           where:
 *           Tf : Filter time
 *           Tf = alpha * Td , where alpha usually is set to 0.1
 *           ed : Unfiltered derivate error with reference weighing
 *             ed = gamma * r - y
 *             where:
 *             gamma : Weighing factor
 * 
 * u       : absolute output
 * 
 * Index n means the n'th value.
 * 
 * 
 * Inputs:
 * enabled ,
 * y_n , r_n , beta=1 , gamma=0 , alpha=0.1 ,
 * Kp , Ti , Td , Ts (is the sampling time available?)
 * u_min , u_max
 * 
 * Output:
 * u_n
 */

void PIDController::update( bool firstTime, double dt ) 
{
    double edf_n = 0.0;
    double u_n = 0.0;       // absolute output

    double u_min = _minInput.get_value();
    double u_max = _maxInput.get_value();

    elapsedTime += dt;
    if( elapsedTime <= desiredTs ) {
        // do nothing if time step is not positive (i.e. no time has
        // elapsed)
        return;
    }
    double Ts = elapsedTime; // sampling interval (sec)
    elapsedTime = 0.0;

    if( firstTime ) {
      // first time being enabled, seed u_n with current
      // property tree value
      ep_n_1 = 0.0;
      edf_n_2 = edf_n_1 = edf_n = 0.0;
      u_n = get_output_value();
      u_n_1 = u_n;
    }

    if( Ts > SGLimitsd::min()) {
        if( _debug ) cout <<  "Updating " << get_name()
                          << " Ts " << Ts << endl;

        double y_n = _valueInput.get_value();
        double r_n = _referenceInput.get_value();
                      
        if ( _debug ) cout << "  input = " << y_n << " ref = " << r_n << endl;

        // Calculates proportional error:
        double ep_n = beta * r_n - y_n;
        if ( _debug ) cout << "  ep_n = " << ep_n;
        if ( _debug ) cout << "  ep_n_1 = " << ep_n_1;

        // Calculates error:
        double e_n = r_n - y_n;
        if ( _debug ) cout << " e_n = " << e_n;

        double td = Td.get_value();
        if ( td > 0.0 ) { // do we need to calcluate derivative error?

          // Calculates derivate error:
            double ed_n = gamma * r_n - y_n;
            if ( _debug ) cout << " ed_n = " << ed_n;

            // Calculates filter time:
            double Tf = alpha * td;
            if ( _debug ) cout << " Tf = " << Tf;

            // Filters the derivate error:
            edf_n = edf_n_1 / (Ts/Tf + 1)
                + ed_n * (Ts/Tf) / (Ts/Tf + 1);
            if ( _debug ) cout << " edf_n = " << edf_n;
        } else {
            edf_n_2 = edf_n_1 = edf_n = 0.0;
        }

        // Calculates the incremental output:
        double ti = Ti.get_value();
        double delta_u_n = 0.0; // incremental output
        if ( ti > 0.0 ) {
            delta_u_n = Kp.get_value() * ( (ep_n - ep_n_1)
                               + ((Ts/ti) * e_n)
                               + ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2)) );

          if ( _debug ) {
              cout << " delta_u_n = " << delta_u_n << endl;
              cout << "P:" << Kp.get_value() * (ep_n - ep_n_1)
                   << " I:" << Kp.get_value() * ((Ts/ti) * e_n)
                   << " D:" << Kp.get_value() * ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2))
                   << endl;
        }
        }

        // Integrator anti-windup logic:
        if ( delta_u_n > (u_max - u_n_1) ) {
            delta_u_n = u_max - u_n_1;
            if ( _debug ) cout << " max saturation " << endl;
        } else if ( delta_u_n < (u_min - u_n_1) ) {
            delta_u_n = u_min - u_n_1;
            if ( _debug ) cout << " min saturation " << endl;
        }

        // Calculates absolute output:
        u_n = u_n_1 + delta_u_n;
        if ( _debug ) cout << "  output = " << u_n << endl;

        // Updates indexed values;
        u_n_1   = u_n;
        ep_n_1  = ep_n;
        edf_n_2 = edf_n_1;
        edf_n_1 = edf_n;

        set_output_value( u_n );
    }
}

bool PIDController::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  SG_LOG( SG_AUTOPILOT, SG_BULK, "PIDController::configure(" << nodeName << ")" << endl );

  if( AnalogComponent::configure( nodeName, configNode ) )
    return true;

  if( nodeName == "config" ) {
    Component::configure( configNode );
    return true;
  }

  if (nodeName == "Ts") {
    desiredTs = configNode->getDoubleValue();
    return true;
  } 

  if (nodeName == "Kp") {
    Kp.push_back( new InputValue(configNode) );
    return true;
  } 

  if (nodeName == "Ti") {
    Ti.push_back( new InputValue(configNode) );
    return true;
  } 

  if (nodeName == "Td") {
    Td.push_back( new InputValue(configNode) );
    return true;
  } 

  if (nodeName == "beta") {
    beta = configNode->getDoubleValue();
    return true;
  } 

  if (nodeName == "alpha") {
    alpha = configNode->getDoubleValue();
    return true;
  } 

  if (nodeName == "gamma") {
    gamma = configNode->getDoubleValue();
    return true;
  } 

  SG_LOG( SG_AUTOPILOT, SG_BULK, "PIDController::configure(" << nodeName << ") [unhandled]" << endl );
  return false;
}

