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
    
    virtual Thruster* clone();

    // Dynamic output
    virtual void getThrust(float* out);
    virtual void getTorque(float* out);
    virtual void getGyro(float* out);
    virtual float getFuelFlow();

    // Runtime instructions
    virtual void integrate(float dt);

    float getOmega();
    
private:
    float _moment;
    Propeller* _prop;
    PistonEngine* _eng;

    float _omega; // RPM, in radians/sec
    float _thrust[3];
    float _torque[3];
    float _gyro[3];
    float _fuelFlow;
};

}; // namespace yasim
#endif // _PROPENGINE_HPP
