// attitude_indicator.cxx - a vacuum-powered attitude indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

// TODO:
// - tumble
// - better spin-up

#include "attitude_indicator.hxx"
#include <Main/fg_props.hxx>


AttitudeIndicator::AttitudeIndicator ()
{
}

AttitudeIndicator::~AttitudeIndicator ()
{
}

void
AttitudeIndicator::init ()
{
                                // TODO: allow index of pump and AI
                                // to be configured.
    _pitch_in_node = fgGetNode("/orientation/pitch-deg", true);
    _roll_in_node = fgGetNode("/orientation/roll-deg", true);
    _suction_node = fgGetNode("/systems/vacuum[0]/suction-inhg", true);
    _pitch_out_node =
        fgGetNode("/instrumentation/attitude-indicator/indicated-pitch-deg",
                  true);
    _roll_out_node =
        fgGetNode("/instrumentation/attitude-indicator/indicated-roll-deg",
                  true);
}

void
AttitudeIndicator::bind ()
{
    fgTie("/instrumentation/attitude-indicator/serviceable",
          &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    fgTie("/instrumentation/attitude-indicator/spin",
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
AttitudeIndicator::unbind ()
{
    fgUntie("/instrumentation/attitude-indicator/serviceable");
    fgUntie("/instrumentation/attitude-indicator/spin");
}

void
AttitudeIndicator::update (double dt)
{
                                // Get the spin from the gyro
    _gyro.set_power_norm(_suction_node->getDoubleValue()/5.0);
    _gyro.update(dt);
    double spin = _gyro.get_spin_norm();

                                // Next, calculate the indicated roll
                                // and pitch, introducing errors.
    double factor = 1.0 - ((1.0 - spin) * (1.0 - spin) * (1.0 - spin));
    double roll = _roll_in_node->getDoubleValue();
    double pitch = _pitch_in_node->getDoubleValue();
    roll = 35 + (factor * (roll - 35));
    pitch = 15 + (factor * (pitch - 15));

    _roll_out_node->setDoubleValue(roll);
    _pitch_out_node->setDoubleValue(pitch);
}

// end of attitude_indicator.cxx
