// gyro.cxx - simple implementation of a spinning gyro model.

#include "gyro.hxx"

Gyro::Gyro ()
    : _serviceable(true),
      _power_norm(0.0),
      _spin_norm(0.0)
{
}

Gyro::~Gyro ()
{
}

void
Gyro::update (double delta_time_sec)
{
                                // spin decays 0.5% every second
    _spin_norm -= 0.005 * delta_time_sec;

                                // power can increase spin by 25%
                                // every second, but only up to the
                                // level of power available
    if (_serviceable) {
        double step = 0.25 * _power_norm * delta_time_sec;
        if ((_spin_norm + step) <= _power_norm)
            _spin_norm += step;
    } else {
        _spin_norm = 0;         // stop right away if the gyro breaks
    }

                                // clamp the spin to 0.0:1.0
    if (_spin_norm < 0.0)
        _spin_norm = 0.0;
    else if (_spin_norm > 1.0)
        _spin_norm = 1.0;
}

void
Gyro::set_power_norm (double power_norm)
{
    _power_norm = power_norm;
}

double
Gyro::get_spin_norm () const
{
    return _spin_norm;
}

void
Gyro::set_spin_norm (double spin_norm)
{
    _spin_norm = spin_norm;
}

bool
Gyro::is_serviceable () const
{
    return _serviceable;
}

void
Gyro::set_serviceable (bool serviceable)
{
    _serviceable = serviceable;
}

// end of gyro.cxx

