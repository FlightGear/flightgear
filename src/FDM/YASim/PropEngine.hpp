#ifndef _PROPENGINE_HPP
#define _PROPENGINE_HPP

#include "Thruster.hpp"

namespace yasim {

class Propeller;
class PistonEngine;

class PropEngine : public Thruster {
public:
    PropEngine(Propeller* prop, PistonEngine* eng, float moment);
    virtual ~PropEngine();

    void setMagnetos(int magnetos);
    void setAdvance(float advance);
    void setPropPitch(float proppitch);
    void setVariableProp(float min, float max);

    virtual PropEngine* getPropEngine() { return this; }
    virtual PistonEngine* getPistonEngine() { return _eng; }
    virtual Propeller* getPropeller() { return _prop; }

    // Dynamic output
    virtual bool isRunning();
    virtual bool isCranking();
    virtual void getThrust(float* out);
    virtual void getTorque(float* out);
    virtual void getGyro(float* out);
    virtual float getFuelFlow();

    // Runtime instructions
    virtual void init();
    virtual void integrate(float dt);
    virtual void stabilize();

    float getOmega();
    
private:
    float _moment;
    Propeller* _prop;
    PistonEngine* _eng;

    bool _variable;
    int _magnetos;  // 0=off, 1=right, 2=left, 3=both
    float _advance; // control input, 0-1
    float _maxOmega;
    float _minOmega;

    float _omega; // RPM, in radians/sec
    float _thrust[3];
    float _torque[3];
    float _gyro[3];
    float _fuelFlow;
};

}; // namespace yasim
#endif // _PROPENGINE_HPP
