#include "Math.hpp"
#include "Thruster.hpp"
namespace yasim {

Thruster::Thruster()
{
    _dir[0] = 1; _dir[1] = 0; _dir[2] = 0;
    for(int i=0; i<3; i++) _pos[i] = _wind[i] = 0;
    _throttle = 0;
    _mixture = 0;
    _P = _T = _rho = 0;
}

Thruster::~Thruster()
{
}

void Thruster::getPosition(float* out)
{
    for(int i=0; i<3; i++) out[i] = _pos[i];
}

void Thruster::setPosition(float* pos)
{
    for(int i=0; i<3; i++) _pos[i] = pos[i];
}

void Thruster::getDirection(float* out)
{
    for(int i=0; i<3; i++) out[i] = _dir[i];
}

void Thruster::setDirection(float* dir)
{
    Math::unit3(dir, _dir);
}

void Thruster::setThrottle(float throttle)
{
    _throttle = Math::clamp(throttle, 0, 1);
}

void Thruster::setMixture(float mixture)
{
    _mixture = Math::clamp(mixture, 0, 1);
}

void Thruster::setWind(float* wind)
{
    for(int i=0; i<3; i++) _wind[i] = wind[i];
}

void Thruster::setAir(float pressure, float temp)
{
    _P = pressure;
    _T = temp;
    _rho = _P / (287.1 * _T);
}

}; // namespace yasim
