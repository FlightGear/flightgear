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
    elapsedTime( 0.0 ),
    startup_current( false ),
    startup_its( 0 ),
    iteration( 0 )
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
    elapsedTime += dt;
    if (firstTime) {
        iteration = 0;
        /* We always initialise edf_n_1 to zero, regardless of startup_its. */
        edf_n_1 = 0;
    }
    else if (elapsedTime <= desiredTs ) {
        // do nothing if not enough time has passed.
        return;
    }

    iteration += 1;

    /* We are going to do an iteration so reset elapsedTime. */
    double Ts = elapsedTime; 
    elapsedTime = 0.0;

    /* Read generic things from our AnalogComponent base class. */
    double y_n      = _valueInput.get_value();      // input.
    double r_n      = _referenceInput.get_value();  // reference.
    double u_min    = _minInput.get_value();        // minimum output.
    double u_max    = _maxInput.get_value();        // maximum output.

    /* Read things specific to PIDController. */
    double td       = Td.get_value();               // derivative time.
    double ti       = Ti.get_value();               // (reciprocal?) integral time.

    /*
    Now do the PID calculations.
    */
    
    double ep_n     = beta * r_n - y_n;             // proportional error.
    double e_n      = r_n - y_n;                    // error.
    double ed_n     = gamma * r_n - y_n;            // derivate error.
    double Tf       = alpha * td;                   // filter time.

    double edf_n = 0.0;                             // derivate error.
    if (td > 0.0) {
        edf_n = edf_n_1 / (Ts/Tf + 1)
            + ed_n * (Ts/Tf) / (Ts/Tf + 1);
    }

    if (firstTime) {
        if (startup_current) {
            /* Seed our historical state with current values. This avoids spurious
            large terms in calculation of delta_u_n below. */
            ep_n_1 = ep_n;
            edf_n_2 = edf_n;
            edf_n_1 = edf_n;
        }
        else {
            // Default behaviour.
            ep_n_1 = 0;
            edf_n_2 = 0;
            // edf_n_1 is already set to zero above.
        }
        u_n_1 = get_output_value();
    }

    double delta_u_n = 0.0;                         // incremental output
    if ( ti > 0.0 ) {
        delta_u_n = Kp.get_value() * (
                (ep_n - ep_n_1)
                + ((Ts/ti) * e_n)
                + ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2))
                );
    }

    const char* saturation = "";
    double u_n;                                     // absolute output

    if (iteration > startup_its) {
        
        /* Update the output, clipping to u_min..u_max. */

        u_n = u_n_1 + delta_u_n;

        if (u_n > u_max) {
            u_n = u_max;
            saturation = " max_saturation";
        }
        else if (u_n < u_min) {
            u_n = u_min;
            saturation = " min_saturation";
        }
        set_output_value( u_n );
    }
    else {
        /* Do not change the output. Instead get the current output value for
        use in setting our historical state below. */
        if (_debug) {
            cout << subsystemId()
                    << ": doing nothing."
                    << " startup_its=" << startup_its
                    << " iteration=" << iteration
                    << std::endl;
        }
        u_n = get_output_value();
    }

    if ( _debug ) {
        cout
                << "Updating " << subsystemId()
                << " startup_its=" << startup_its
                << " startup_current=" << startup_current
                << " firstTime=" << firstTime
                << " iteration=" << iteration
                << " Ts=" << Ts
                << " input=" << y_n
                << " ref=" << r_n
                << " ep_n=" << ep_n
                << " ep_n_1=" << ep_n_1
                << " e_n=" << e_n
                << " ed_n=" << ed_n
                << " Tf=" << Tf
                << " edf_n=" << edf_n
                << " edf_n_1=" << edf_n_1
                << " edf_n_2=" << edf_n_2
                << " ti=" << ti
                << " delta_u_n=" << delta_u_n
                << " P=" << Kp.get_value() * (ep_n - ep_n_1)
                << " I=" << Kp.get_value() * ((Ts/ti) * e_n)
                << " D=" << Kp.get_value() * ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2))
                << saturation
                << " u_n_1=" << u_n_1
                << " delta_u_n=" << delta_u_n
                << " output=" << u_n
                << std::endl;
    }

    // Updates indexed values;
    u_n_1   = u_n;
    ep_n_1  = ep_n;
    edf_n_2 = edf_n_1;
    edf_n_1 = edf_n;
}

//------------------------------------------------------------------------------
bool PIDController::configure( SGPropertyNode& cfg_node,
                               const std::string& cfg_name,
                               SGPropertyNode& prop_root )
{
  if( cfg_name == "config" ) {
    Component::configure(prop_root, cfg_node);
    return true;
  }

  if (cfg_name == "Ts") {
    desiredTs = cfg_node.getDoubleValue();
    return true;
  } 

  if (cfg_name == "Kp") {
    Kp.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  } 

  if (cfg_name == "Ti") {
    Ti.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  } 

  if (cfg_name == "Td") {
    Td.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  } 

  if (cfg_name == "beta") {
    beta = cfg_node.getDoubleValue();
    return true;
  } 

  if (cfg_name == "alpha") {
    alpha = cfg_node.getDoubleValue();
    return true;
  } 

  if (cfg_name == "gamma") {
    gamma = cfg_node.getDoubleValue();
    return true;
  }
  
  if (cfg_name == "startup-its") {
    startup_its = cfg_node.getIntValue();
    return true;
  }

  if (cfg_name == "startup-current") {
    startup_current = cfg_node.getBoolValue();
    return true;
  }

  return AnalogComponent::configure(cfg_node, cfg_name, prop_root);
}


// Register the subsystem.
SGSubsystemMgr::Registrant<PIDController> registrantPIDController;
