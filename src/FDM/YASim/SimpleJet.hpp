#ifndef _SIMPLEJET_HPP
#define _SIMPLEJET_HPP

#include "Thruster.hpp"

namespace yasim {

// As simple a Thruster subclass as you can find. It makes thrust.  Period.
class SimpleJet : public Thruster
{
public:
    SimpleJet();
    void setThrust(float thrust);
    virtual bool isRunning();
    virtual bool isCranking();
    virtual void getThrust(float* out);
    virtual void getTorque(float* out);
    virtual void getGyro(float* out);
    virtual float getFuelFlow();
    virtual void integrate(float dt);
    virtual void stabilize();
private:
    float _thrust;
};

}; // namespace yasim
#endif // _SIMPLEJET_HPP
