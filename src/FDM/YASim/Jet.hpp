#ifndef _JET_HPP
#define _JET_HPP

#include "Thruster.hpp"

namespace yasim {

// Incredibly dumb placeholder for a Jet implementation.  But it does
// what's important, which is provide thrust.
class Jet : public Thruster {
public:
    Jet();

    virtual Thruster* clone();

    void setDryThrust(float thrust);
    void setReheat(float reheat);

    virtual void getThrust(float* out);
    virtual void getTorque(float* out);
    virtual void getGyro(float* out);
    virtual float getFuelFlow();
    virtual void integrate(float dt);

private:
    float _thrust;
    float _rho0;
    float _reheat;
};

}; // namespace yasim
#endif // _JET_HPP
