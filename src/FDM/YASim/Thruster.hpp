#ifndef _THRUSTER_HPP
#define _THRUSTER_HPP

namespace yasim {

class Jet;
class PropEngine;
class Propeller;
class Engine;

class Thruster {
public:
    Thruster();
    virtual ~Thruster();

    // Typing info, these are the possible sub-type (or sub-parts)
    // that a thruster might have.  Any might return null.  A little
    // clumsy, but much simpler than an RTTI-based implementation.
    virtual Jet* getJet() { return 0; }
    virtual PropEngine* getPropEngine() { return 0; }
    virtual Propeller* getPropeller() { return 0; }
    virtual Engine* getEngine() { return 0; }
    
    // Static data
    void getPosition(float* out);
    void setPosition(float* pos);
    void getDirection(float* out);
    void setDirection(float* dir);

    // Controls
    void setThrottle(float throttle);
    void setMixture(float mixture);
    void setStarter(bool starter);
    void setFuelState(bool hasFuel) { _fuel = hasFuel; }

    // Dynamic output
    virtual bool isRunning()=0;
    virtual bool isCranking()=0;
    virtual void getThrust(float* out)=0;
    virtual void getTorque(float* out)=0;
    virtual void getGyro(float* out)=0;
    virtual float getFuelFlow()=0; // in kg/s

    // Runtime instructions
    void setWind(float* wind);
    void setAir(float pressure, float temp, float density);
    virtual void init() {}
    virtual void integrate(float dt)=0;
    virtual void stabilize()=0;

protected:
    float _pos[3];
    float _dir[3];
    float _throttle;
    float _mixture;
    bool _starter; // true=engaged, false=disengaged
    bool _fuel; // true=available, false=out

    float _wind[3];
    float _pressure;
    float _temp;
    float _rho;
};

}; // namespace yasim
#endif // _THRUSTER_HPP

