// attitude_indicator.cxx - a vacuum-powered attitude indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

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

    _spin -= 0.01 * dt;         // spin decays every 1% every second.

                                // spin increases up to 10% every second
                                // if suction is available and the gauge
                                // is serviceable.
    if (_serviceable_node->getBoolValue()) {
        double suction = _suction_node->getDoubleValue();
        double step = 0.10 * (suction / 5.0) * dt;
        if ((_spin + step) <= (suction / 5.0))
            _spin += step;
    }

                                // Next, calculate the indicated roll
                                // and pitch, introducing errors if
                                // the spin is less than 0.8 (80%).
    double roll = _roll_in_node->getDoubleValue();
    double pitch = _pitch_in_node->getDoubleValue();
    if (_spin < 0.8) {
        // TODO
    }
    _roll_out_node->setDoubleValue(roll);
    _pitch_out_node->setDoubleValue(pitch);
}

// end of attitude_indicator.cxx
