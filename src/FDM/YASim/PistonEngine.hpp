#ifndef _PISTONENGINE_HPP
#define _PISTONENGINE_HPP

#include "Engine.hpp"

namespace yasim {

class PistonEngine : public Engine {
public:
    virtual PistonEngine* isPistonEngine() { return this; }

    // Initializes an engine from known "takeoff" parameters.
    PistonEngine(float power, float spd);
    void setTurboParams(float mul, float maxMP);
    void setDisplacement(float d);
    void setCompression(float c);

    bool isCranking();
    float getMP();
    float getEGT();
    float getMaxPower(); // max sea-level power
    float getBoost() { return _boostPressure; }
    float getOilTemp() { return _oilTemp; }

    virtual void calc(float pressure, float temp, float speed);
    virtual void stabilize();
    virtual void integrate(float dt);
    virtual float getTorque();
    virtual float getFuelFlow();

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

    // Runtime state/output:
    float _mp;
    float _torque;
    float _fuelFlow;
    float _egt;
    float _boostPressure;
    float _oilTemp;
    float _oilTempTarget;
    float _dOilTempdt;
};

}; // namespace yasim
#endif // _PISTONENGINE_HPP
