#ifndef _PISTONENGINE_HPP
#define _PISTONENGINE_HPP

namespace yasim {

class PistonEngine {
public:
    // Initializes an engine from known "takeoff" parameters.
    PistonEngine(float power, float spd);
    void setTurboParams(float mul, float maxMP);
    void setDisplacement(float d);
    void setCompression(float c);

    void setThrottle(float throttle);
    void setMixture(float mixture);
    void setBoost(float boost); // fraction of turbo-mul used

    float getMaxPower(); // max sea-level power

    void calc(float pressure, float temp, float speed);
    float getTorque();
    float getFuelFlow();
    float getMP();
    float getEGT();

private:
    // Static configuration:
    float _power0;   // reference power setting
    float _omega0;   //   "       engine speed
    float _rho0;     //   "       manifold air density
    float _f0;       // "ideal" fuel flow at P0/omega0
    float _mixCoeff; // fuel flow per omega at full mixture
    float _turbo;    // (or super-)charger pressure multiplier
    float _maxMP;    // wastegate setting
    float _displacement; // piston stroke volume
    float _compression;  // compression ratio (>1)

    // Runtime settables:
    float _throttle;
    float _mixture;
    float _boost;

    // Runtime state/output:
    float _mp;
    float _torque;
    float _fuelFlow;
    float _egt;
};

}; // namespace yasim
#endif // _PISTONENGINE_HPP
