#ifndef _PROPENGINE_HPP
#define _PROPENGINE_HPP

#include "Thruster.hpp"
#include "Engine.hpp"

namespace yasim {

class Propeller;

class PropEngine : public Thruster {
public:
    PropEngine(Propeller* prop, Engine* eng, float moment);
    virtual ~PropEngine();

    void setEngine(Engine* eng) { delete _eng; _eng = eng; }

    void setMagnetos(int magnetos);
    void setAdvance(float advance);
    void setPropPitch(float proppitch);
    void setVariableProp(float min, float max);
    void setPropFeather(int state);
    void setGearRatio(float ratio) { _gearRatio = ratio; }
    void setContraPair(bool contra) { _contra = contra; }

    virtual PropEngine* getPropEngine() { return this; }
    virtual Engine* getEngine() { return _eng; }
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
    void setOmega (float omega);
    
private:
    float _moment;
    Propeller* _prop;
    Engine* _eng;

    bool _variable;
    bool _contra; // contra-rotating propeller pair
    int _magnetos;  // 0=off, 1=right, 2=left, 3=both
    float _gearRatio;
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
