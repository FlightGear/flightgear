#ifndef _TURBINEENGINE_HPP
#define _TURBINEENGINE_HPP

#include "Engine.hpp"

namespace yasim {

class TurbineEngine : public Engine {
public:
    virtual TurbineEngine* isTurbineEngine() { return this; }

    TurbineEngine(float power, float omega, float alt, float flatRating);
    void setN2Range(float low_idle, float high_idle, float max) {
        _n2LowIdle = low_idle;
        _n2HighIdle = high_idle;
        _n2Max = max;
    }
    void setFuelConsumption(float bsfc) { _bsfc = bsfc; }

    virtual void calc(float pressure, float temp, float speed);
    virtual void stabilize();
    virtual void integrate(float dt);

    void setCondLever( float lever ) {
        _cond_lever = lever;
    }
    virtual float getTorque() { return _torque; }
    virtual float getFuelFlow() { return _fuelFlow; }
    float getN2() { return _n2; }

private:
    void setOutputFromN2();

    float _cond_lever;

    float _maxTorque;
    float _flatRating;
    float _rho0;
    float _bsfc; // SI units! kg/s per watt
    float _n2LowIdle;
    float _n2HighIdle;
    float _n2Max;

    float _n2Min;
    float _n2Target;
    float _torqueTarget;
    float _fuelFlowTarget;

    float _n2;
    float _rho;
    float _omega;
    float _torque;
    float _fuelFlow;
};

}; // namespace yasim
#endif // _TURBINEENGINE_HPP
