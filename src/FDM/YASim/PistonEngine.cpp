#include "Atmosphere.hpp"
#include "Math.hpp"
#include "PistonEngine.hpp"
namespace yasim {

PistonEngine::PistonEngine(float power, float speed)
{
    // Presume a BSFC (in lb/hour per HP) of 0.45.  In SI that becomes
    // (2.2 lb/kg, 745.7 W/hp, 3600 sec/hour) 3.69e-07 kg/Ws.
    _f0 = power * 3.69e-07;

    _power0 = power;
    _omega0 = speed;

    // We must be at sea level under standard conditions
    _rho0 = Atmosphere::getStdDensity(0);

    // Further presume that takeoff is (duh) full throttle and
    // peak-power, that means that by our efficiency function, we are
    // at 11/8 of "ideal" fuel flow.
    float realFlow = _f0 * (11.0/8.0);
    _mixCoeff = realFlow * 1.1 / _omega0;

    _turbo = 1;
    _maxMP = 1e6; // No waste gate on non-turbo engines.
}

void PistonEngine::setTurboParams(float turbo, float maxMP)
{
    _turbo = turbo;
    _maxMP = maxMP;

    // This changes the "sea level" manifold air density
    float P0 = Atmosphere::getStdPressure(0);
    float P = P0 * _turbo;
    if(P > _maxMP) P = _maxMP;
    float T = Atmosphere::getStdTemperature(0) * Math::pow(P/P0, 2./7.);
    _rho0 = P / (287.1 * T);
}

float PistonEngine::getPower()
{
    return _power0;
}

void PistonEngine::setThrottle(float t)
{
    _throttle = t;
}

void PistonEngine::setMixture(float m)
{
    _mixture = m;
}

void PistonEngine::calc(float P, float T, float speed,
			float* torqueOut, float* fuelFlowOut)
{
    // The actual fuel flow
    float fuel = _mixture * _mixCoeff * speed;

    // manifold air density
    if(_turbo != 1) {
        float P1 = P * _turbo;
        if(P1 > _maxMP) P1 = _maxMP;
        T *= Math::pow(P1/P, 2./7.);
        P = P1;
    }
    float density = P / (287.1 * T);
    
    float rho = density * _throttle;

    // How much fuel could be burned with ideal (i.e. uncorrected!)
    // combustion.
    float burnable = _f0 * (rho/_rho0) * (speed/_omega0);

    // Calculate the fuel that actually burns to produce work.  The
    // idea is that less than 5/8 of ideal, we get complete
    // combustion.  We use up all the oxygen at 1 3/8 of ideal (that
    // is, you need to waste fuel to use all your O2).  In between,
    // interpolate.  This vaguely matches a curve I copied out of a
    // book for a single engine.  Shrug.
    float burned;
    float r = fuel/burnable;
    if     (burnable == 0) burned = 0;
    else if(r < .625)      burned = fuel;
    else if(r > 1.375)     burned = burnable;
    else                   burned = fuel + (burnable-fuel)*(r-.625)*(4.0/3.0);

    // And finally the power is just the reference power scaled by the
    // amount of fuel burned.
    float power = _power0 * burned/_f0;

    *torqueOut = power/speed;
    *fuelFlowOut = fuel;
}

}; // namespace yasim
