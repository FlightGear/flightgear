#ifndef _THRUSTER_HPP
#define _THRUSTER_HPP

namespace yasim {

class Thruster {
public:
    Thruster();
    virtual ~Thruster();

    // Static data
    void getPosition(float* out);
    void setPosition(float* pos);
    void getDirection(float* out);
    void setDirection(float* dir);

    virtual Thruster* clone()=0;

    // Controls
    void setThrottle(float throttle);
    void setMixture(float mixture);
    void setPropAdvance(float advance);

    // Dynamic output
    virtual void getThrust(float* out)=0;
    virtual void getTorque(float* out)=0;
    virtual void getGyro(float* out)=0;
    virtual float getFuelFlow()=0;

    // Runtime instructions
    void setWind(float* wind);
    void setDensity(float rho);
    virtual void integrate(float dt)=0;

protected:
    void cloneInto(Thruster* out);

    float _pos[3];
    float _dir[3];
    float _throttle;
    float _mixture;
    float _propAdvance;

    float _wind[3];
    float _rho;
};

}; // namespace yasim
#endif // _THRUSTER_HPP

