// attitude_indicator.cxx - a vacuum-powered attitude indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

// TODO:
// - better spin-up

#include <math.h>	// fabs()

#include "attitude_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


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
    _tumble_flag_node =
        fgGetNode("/instrumentation/attitude-indicator/config/tumble-flag",
                  true);
    _caged_node =
        fgGetNode("/instrumentation/attitude-indicator/caged-flag", true);
    _tumble_node =
        fgGetNode("/instrumentation/attitude-indicator/tumble-norm", true);
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
                                // If it's caged, it doesn't indicate
    if (_caged_node->getBoolValue()) {
        _roll_out_node->setDoubleValue(0.0);
        _pitch_out_node->setDoubleValue(0.0);
        return;
    }

                                // Get the spin from the gyro
    _gyro.set_power_norm(_suction_node->getDoubleValue()/5.0);
    _gyro.update(dt);
    double spin = _gyro.get_spin_norm();

                                // Calculate the responsiveness
    double responsiveness = spin * spin * spin * spin * spin * spin;

                                // Get the indicated roll and pitch
    double roll = _roll_in_node->getDoubleValue();
    double pitch = _pitch_in_node->getDoubleValue();

                                // Calculate the tumble for the
                                // next pass.
    if (_tumble_flag_node->getBoolValue()) {
        double tumble = _tumble_node->getDoubleValue();
        if (fabs(roll) > 45.0) {
            double target = (fabs(roll) - 45.0) / 45.0;
            target *= target;   // exponential past +-45 degrees
            if (roll < 0)
                target = -target;

            if (fabs(target) > fabs(tumble))
                tumble = target;

            if (tumble > 1.0)
                tumble = 1.0;
            else if (tumble < -1.0)
                tumble = -1.0;
        }
                                    // Reerect in 5 minutes
        double step = dt/300.0;
        if (tumble < -step)
            tumble += step;
        else if (tumble > step)
            tumble -= step;

        roll += tumble * 45;
        _tumble_node->setDoubleValue(tumble);
    }

    roll = fgGetLowPass(_roll_out_node->getDoubleValue(), roll,
                        responsiveness);
    pitch = fgGetLowPass(_pitch_out_node->getDoubleValue(), pitch,
                         responsiveness);

                                // Assign the new values
    _roll_out_node->setDoubleValue(roll);
    _pitch_out_node->setDoubleValue(pitch);
}

// end of attitude_indicator.cxx
