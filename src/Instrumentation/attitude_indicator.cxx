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
    _serviceable_node =
        fgGetNode("/instrumentation/attitude-indicator/serviceable", true);
    _spin_node = fgGetNode("/instrumentation/attitude-indicator/spin", true);
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
}

void
AttitudeIndicator::unbind ()
{
}

void
AttitudeIndicator::update (double dt)
{
                                // First, calculate the bogo-spin from 0 to 1.
                                // All numbers are made up.

    double spin = _spin_node->getDoubleValue();
    spin -= 0.005 * dt;         // spin decays every 0.5% every second.

                                // spin increases up to 25% every second
                                // if suction is available and the gauge
                                // is serviceable.
    if (_serviceable_node->getBoolValue()) {
        double suction = _suction_node->getDoubleValue();
        double step = 0.25 * (suction / 5.0) * dt;
        if ((spin + step) <= (suction / 5.0))
            spin += step;
    }
    if (spin > 1.0)
        spin = 1.0;
    else if (spin < 0.0)
        spin = 0.0;
    _spin_node->setDoubleValue(spin);

                                // Next, calculate the indicated roll
                                // and pitch, introducing errors.
    double factor = 1.0 - ((1.0 - spin) * (1.0 - spin));
    double roll = _roll_in_node->getDoubleValue();
    double pitch = _pitch_in_node->getDoubleValue();
    roll = 35 + (factor * (roll - 35));
    pitch = 15 + (factor * (pitch - 15));

    _roll_out_node->setDoubleValue(roll);
    _pitch_out_node->setDoubleValue(pitch);
}

// end of attitude_indicator.cxx
