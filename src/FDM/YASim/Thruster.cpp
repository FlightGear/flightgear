#include "Math.hpp"
#include "Thruster.hpp"
namespace yasim {

Thruster::Thruster()
{
    _dir[0] = 1; _dir[1] = 0; _dir[2] = 0;
    int i;
    for(i=0; i<3; i++) _pos[i] = _wind[i] = 0;
    _throttle = 0;
    _mixture = 0;
    _starter = false;
    _pressure = _temp = _rho = 0;
}

Thruster::~Thruster()
{
}

void Thruster::getPosition(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _pos[i];
}

void Thruster::setPosition(float* pos)
{
    int i;
    for(i=0; i<3; i++) _pos[i] = pos[i];
}

void Thruster::getDirection(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _dir[i];
}

void Thruster::setDirection(float* dir)
{
    Math::unit3(dir, _dir);
}

void Thruster::setThrottle(float throttle)
{
    _throttle = Math::clamp(throttle, -1, 1);
}

void Thruster::setMixture(float mixture)
{
    _mixture = Math::clamp(mixture, 0, 1);
}


void Thruster::setStarter(bool starter)
{
    _starter = starter;
}

void Thruster::setWind(float* wind)
{
    int i;
    for(i=0; i<3; i++) _wind[i] = wind[i];
}

void Thruster::setAir(float pressure, float temp, float density)
{
    _pressure = pressure;
    _temp = temp;
    _rho = density;
}

}; // namespace yasim
