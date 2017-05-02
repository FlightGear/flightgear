#ifndef _THRUSTER_HPP
#define _THRUSTER_HPP

#include "Atmosphere.hpp"
#include "Math.hpp"

namespace yasim {

class Jet;
class PropEngine;
class Propeller;
class Engine;

class Thruster {
public:
    Thruster() {};
    virtual ~Thruster() {};

    // Typing info, these are the possible sub-type (or sub-parts)
    // that a thruster might have.  Any might return null.  A little
    // clumsy, but much simpler than an RTTI-based implementation.
    virtual Jet* getJet() { return 0; }
    virtual PropEngine* getPropEngine() { return 0; }
    virtual Propeller* getPropeller() { return 0; }
    virtual Engine* getEngine() { return 0; }
    
    // Static data
    void getPosition(float* out) const { Math::set3(_pos, out); }
    void setPosition(const float* pos) { Math::set3(pos, _pos); }
    void getDirection(float* out) const { Math::set3(_dir, out); }
    void setDirection(const float* dir) { Math::unit3(dir, _dir); }

    // Controls
    void setThrottle(float throttle) { _throttle = Math::clamp(throttle, -1, 1); }
    void setMixture(float mixture) { _mixture = Math::clamp(mixture, 0, 1); }
    void setStarter(bool starter) { _starter = starter; }
    void setFuelState(bool hasFuel) { _fuel = hasFuel; }

    // Dynamic output
    virtual bool isRunning()=0;
    virtual bool isCranking()=0;
    virtual void getThrust(float* out)=0;
    virtual void getTorque(float* out)=0;
    virtual void getGyro(float* out)=0;
    virtual float getFuelFlow()=0; // in kg/s

    // Runtime instructions
    void setWind(const float* wind) { Math::set3(wind, _wind); };
    void setAtmosphere(Atmosphere a) { _atmo = a; };
    void setStandardAtmosphere(float altitude) { _atmo.setStandard(altitude); };
    virtual void init() {}
    virtual void integrate(float dt)=0;
    virtual void stabilize()=0;

protected:
    float _pos[3] {0, 0, 0};
    float _dir[3] {1, 0, 0};
    float _throttle = 0;
    float _mixture = 0;
    bool _starter = false; // true=engaged, false=disengaged
    bool _fuel; // true=available, false=out

    float _wind[3] {0, 0, 0};
    Atmosphere _atmo;
};

}; // namespace yasim
#endif // _THRUSTER_HPP
