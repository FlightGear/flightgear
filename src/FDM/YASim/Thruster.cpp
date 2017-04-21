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

void Thruster::setAir(float pressure, float temp, float density)
{
    _pressure = pressure;
    _temp = temp;
    _rho = density;
}

}; // namespace yasim
