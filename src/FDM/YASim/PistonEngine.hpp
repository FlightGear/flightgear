#ifndef _PISTONENGINE_HPP
#define _PISTONENGINE_HPP

namespace yasim {

class PistonEngine {
public:
    // Initializes an engine from known "takeoff" parameters.
    PistonEngine(float power, float spd);
    void setTurboParams(float mul, float maxMP);

    void setThrottle(float throttle);
    void setMixture(float mixture);

    float getPower();

    // Calculates power output and fuel flow, based on a given
    // throttle setting (0-1 corresponding to the fraction of
    // "available" manifold pressure), mixture (fuel flux per rpm,
    // 0-1, where 1 is "max rich", or a little bit more than needed
    // for rated power at sea level)
    void calc(float pressure, float temp, float speed,
	      float* powerOut, float* fuelFlowOut);

private:
    float _power0;   // reference power setting
    float _omega0;   //   "       engine speed
    float _rho0;     //   "       manifold air density
    float _f0;       // "ideal" fuel flow at P0/omega0
    float _mixCoeff; // fuel flow per omega at full mixture

    // Runtime settables:
    float _throttle;
    float _mixture;

    float _turbo;
    float _maxMP;
};

}; // namespace yasim
#endif // _PISTONENGINE_HPP
