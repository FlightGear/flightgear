#include "Atmosphere.hpp"

#include "TurbineEngine.hpp"

namespace yasim {

TurbineEngine::TurbineEngine(float power, float omega, float alt,
                             float flatRating)
{
    _rho0 = Atmosphere::getStdDensity(0);
    _maxTorque = (power/omega) * _rho0 / Atmosphere::getStdDensity(alt);
    _flatRating = flatRating;
    _bsfc = 0.047; // == 0.5 lb/hr per hp
    _n2Min = 65;
    _n2Max = 100;

    _rho = _rho0;
    _omega = 0;
    _n2 = _n2Target = _n2Min;
    _torque = 0;
    _fuelFlow = 0;
}

void TurbineEngine::setOutputFromN2()
{
    float frac = (_n2 - _n2Min) / (_n2Max - _n2Min);
    _torque = frac * _maxTorque * (_rho / _rho0);
    _fuelFlow = _bsfc * _torque * _omega;
}

void TurbineEngine::stabilize()
{
    _n2 = _n2Target;
    setOutputFromN2();
}

void TurbineEngine::integrate(float dt)
{
    // Low-pass the N2 speed to give a realistic spooling time.  See
    // the notes in Jet::setSpooling() for details; this corresponds
    // to a hard-coded spool time of 2 seconds.
    const float DECAY = 1.15;
    _n2 = (_n2 + dt * DECAY * _n2Target)/(1 + dt * DECAY);
    setOutputFromN2();
}

void TurbineEngine::calc(float pressure, float temp, float omega)
{
    _omega = omega;
    _rho = Atmosphere::calcStdDensity(pressure, temp);

    float torque = _throttle * _maxTorque * _rho / _rho0;
    float power = torque * omega;
    if(power > _flatRating)
        torque = _flatRating / omega;

    float frac = torque / (_maxTorque * (_rho / _rho0));
    _n2Target = _n2Min + (_n2Max - _n2Min) * frac;
}

}; // namespace yasim
