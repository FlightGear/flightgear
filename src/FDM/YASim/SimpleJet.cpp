#include "Math.hpp"
#include "SimpleJet.hpp"

namespace yasim {

SimpleJet::SimpleJet()
{
    _thrust = 0;
}

void SimpleJet::setThrust(float thrust)
{
    _thrust = thrust;
}

bool SimpleJet::isRunning()
{
    return true;
}

bool SimpleJet::isCranking()
{
    return false;
}

void SimpleJet::getThrust(float* out)
{
    Math::mul3(_thrust * _throttle, _dir, out);
}

void SimpleJet::getTorque(float* out)
{
    out[0] = out[1] = out[2] = 0;
}

void SimpleJet::getGyro(float* out)
{
    out[0] = out[1] = out[2] = 0;
}

float SimpleJet::getFuelFlow()
{
    return 0;
}

void SimpleJet::integrate(float dt)
{
    return;
}

void SimpleJet::stabilize()
{
    return;
}

}; // namespace yasim
