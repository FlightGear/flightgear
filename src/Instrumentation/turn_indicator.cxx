// turn_indicator.cxx - an electric-powered turn indicator.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/compiler.h>
#include <iostream>
#include <string>
#include <sstream>

#include "turn_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>

using std::string;

// Use a bigger number to be more responsive, or a smaller number
// to be more sluggish.
#define RESPONSIVENESS 0.5


TurnIndicator::TurnIndicator ( SGPropertyNode *node) :
    _last_rate(0),
    _name(node->getStringValue("name", "turn-indicator")),
    _num(node->getIntValue("number", 0))
{
}

TurnIndicator::~TurnIndicator ()
{
}

void
TurnIndicator::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _roll_rate_node = fgGetNode("/orientation/roll-rate-degps", true);
    _yaw_rate_node = fgGetNode("/orientation/yaw-rate-degps", true);
    _electric_current_node = 
        fgGetNode("/systems/electrical/outputs/turn-coordinator", true);
    _rate_out_node = node->getChild("indicated-turn-rate", 0, true);
}

void
TurnIndicator::bind ()
{
    std::ostringstream temp;
    string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";

    fgTie((branch + "/serviceable").c_str(),
          &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    fgTie((branch + "/spin").c_str(),
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
TurnIndicator::unbind ()
{
    std::ostringstream temp;
    string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";

    fgUntie((branch + "/serviceable").c_str());
    fgUntie((branch + "/serviceable").c_str());
}

void
TurnIndicator::update (double dt)
{
                                // Get the spin from the gyro
    double power = _electric_current_node->getDoubleValue() / 12.0;
    _gyro.set_power_norm(power);
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
