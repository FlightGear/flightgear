#ifndef _ENGINE_HPP
#define _ENGINE_HPP

namespace yasim {

class PistonEngine;
class TurbineEngine;

//
// Interface for the "Engine" part of a PropEngine object.  This is a
// virtual class, intended to be implemented by stuff like
// PistonEngine and TurbineEngine, and maybe exotics like
// SolarElectricEngine, etc...
//

class Engine {
public:
    virtual PistonEngine* isPistonEngine() { return 0; }
    virtual TurbineEngine* isTurbineEngine() { return 0; }

    void setThrottle(float throttle) { _throttle = throttle; }
    void setStarter(bool starter) { _starter = starter; }
    void setMagnetos(int magnetos) { _magnetos = magnetos; }
    void setMixture(float mixture) { _mixture = mixture; }
    void setBoost(float boost) { _boost = boost; }
    void setFuelState(bool hasFuel) { _fuel = hasFuel; }
    void setRunning(bool r) { _running = r; }

    bool isRunning() { return _running; }
    virtual bool isCranking() { return false; }

    virtual void calc(float pressure, float temp, float speed) = 0;
    virtual void stabilize() {}
    virtual void integrate(float dt) {}
    virtual float getTorque() = 0;
    virtual float getFuelFlow() = 0;

    virtual ~Engine() {}
protected:
    float _throttle;
    bool _starter; // true=engaged, false=disengaged
    int _magnetos; // 0=off, 1=right, 2=left, 3=both
    float _mixture;
    float _boost;
    bool _fuel;
    bool _running;
};

}; // namespace yasim
#endif // _ENGINE_HPP
