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
    void setStarter(bool starter);
    void setMagnetos(int magnetos);
    void setMixture(float mixture);
    void setBoost(float boost); // fraction of turbo-mul used
    void setFuelState(bool hasFuel) { _fuel = hasFuel; }

    // For solver use
    void setRunning(bool r);

    float getMaxPower(); // max sea-level power

    void calc(float pressure, float temp, float speed);
    bool isRunning();
    bool isCranking();
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
    bool _starter; // true=engaged, false=disengaged
    int _magnetos; // 0=off, 1=right, 2=left, 3=both
    float _mixture;
    float _boost;
    bool _fuel;

    // Runtime state/output:
    bool _running;
    bool _cranking;
    float _mp;
    float _torque;
    float _fuelFlow;
    float _egt;
};

}; // namespace yasim
#endif // _PISTONENGINE_HPP
