#include "Atmosphere.hpp"
#include "Math.hpp"
#include "Jet.hpp"
namespace yasim {

Jet::Jet()
{
    _rho0 = Atmosphere::getStdDensity(0);
    _thrust = 0;
    _reheat = 0;
}

Thruster* Jet::clone()
{
    Jet* j = new Jet();
    j->_thrust = _thrust;
    j->_rho0 = _rho0;
    return j;
}

void Jet::setDryThrust(float thrust)
{
    _thrust = thrust;
}

void Jet::setReheat(float reheat)
{
    _reheat = reheat;
}

void Jet::getThrust(float* out)
{
    float t = _thrust * _throttle * (_rho / _rho0);
    Math::mul3(t, _dir, out);
}

void Jet::getTorque(float* out)
{
    out[0] = out[1] = out[2] = 0;
    return;
}

void Jet::getGyro(float* out)
{
    out[0] = out[1] = out[2] = 0;
    return;
}

float Jet::getFuelFlow()
{
    return 0;
}

void Jet::integrate(float dt)
{
    return;
}

}; // namespace yasim
