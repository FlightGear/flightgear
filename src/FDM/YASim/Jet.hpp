#ifndef _JET_HPP
#define _JET_HPP

#include "Thruster.hpp"

namespace yasim {

// Incredibly dumb placeholder for a Jet implementation.  But it does
// what's important, which is provide thrust.
class Jet : public Thruster {
public:
    Jet();

    virtual Jet* getJet() { return this; }

    void setDryThrust(float thrust);
    void setReheatThrust(float thrust);
    void setReheat(float reheat);

    virtual void getThrust(float* out);
    virtual void getTorque(float* out);
    virtual void getGyro(float* out);
    virtual float getFuelFlow();
    virtual void integrate(float dt);
    virtual void stabilize();

private:
    float _thrust;
    float _abThrust;
    float _rho0;
    float _reheat;
};

}; // namespace yasim
#endif // _JET_HPP
