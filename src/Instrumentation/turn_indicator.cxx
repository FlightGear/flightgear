// turn_indicator.cxx - an electric-powered turn indicator.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#include "turn_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


// Use a bigger number to be more responsive, or a smaller number
// to be more sluggish.
#define RESPONSIVENESS 1.0


TurnIndicator::TurnIndicator () :
    _last_rate(0)
{
}

TurnIndicator::~TurnIndicator ()
{
}

void
TurnIndicator::init ()
{
    _roll_rate_node = fgGetNode("/orientation/roll-rate-degps", true);
    _yaw_rate_node = fgGetNode("/orientation/yaw-rate-degps", true);
    _electric_current_node = 
        fgGetNode("/systems/electrical/outputs/turn-coordinator", true);
    _rate_out_node = 
        fgGetNode("/instrumentation/turn-indicator/indicated-turn-rate", true);
}

void
TurnIndicator::bind ()
{
    fgTie("/instrumentation/turn-indicator/serviceable",
          &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    fgTie("/instrumentation/turn-indicator/spin",
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
TurnIndicator::unbind ()
{
    fgUntie("/instrumentation/turn-indicator/serviceable");
    fgUntie("/instrumentation/turn-indicator/spin");
}

void
TurnIndicator::update (double dt)
{
                                // Get the spin from the gyro
    _gyro.set_power_norm(_electric_current_node->getDoubleValue()/60.0);
    _gyro.update(dt);
    double spin = _gyro.get_spin_norm();

                                // Calculate the indicated rate
    double factor = 1.0 - ((1.0 - spin) * (1.0 - spin) * (1.0 - spin));
    double rate = ((_roll_rate_node->getDoubleValue() / 20.0) +
                   (_yaw_rate_node->getDoubleValue() / 3.0));

                                // Clamp the rate
    if (rate < -2.5)
        rate = -2.5;
    else if (rate > 2.5)
        rate = 2.5;

                                // Lag left, based on gyro spin
    rate = -2.5 + (factor * (rate + 2.5));
    rate = fgGetLowPass(_last_rate, rate, dt*RESPONSIVENESS);
    _last_rate = rate;
    
                                // Publish the indicated rate
    _rate_out_node->setDoubleValue(rate);
}

// end of turn_indicator.cxx
