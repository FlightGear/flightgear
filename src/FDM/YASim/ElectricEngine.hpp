// Based on original patch from Ian Dall <ian@beware.dropbear.id.au>
// Date:   Wed Jan 11 22:35:24 2012 +1030
// #595 Support for electric motors in YASim (patch provided)
// Improved by ThunderFly s.r.o. <info@thunderfly.cz>
////////////////////////////////////////////////////////

#ifndef _ELECTRICENGINE_HPP
#define _ELECTRICENGINE_HPP

#include "Engine.hpp"

namespace yasim {

class ElectricEngine : public Engine {
public:
    virtual ElectricEngine* isElectricEngine() { return this; }

    // Initializes an engine from known "takeoff" parameters.
    // voltage - power supply voltage applied to the motor in volts
    // Kv - electric motor velocity constant in RPM/1V
    // Rm - engine winding resistance in Ohms
    ElectricEngine(float voltage, float Kv, float Rm);

    virtual void calc(float pressure, float temp, float speed);
    virtual void stabilize() { return; };
    virtual void integrate(float dt) { return; };
    virtual float getTorque() { return _torque; };
    virtual float getFuelFlow() { return 0; };
    virtual float getOmega0() { return _omega0; };

private:
    // Static configuration:
    float _omega0 {0};		// Reference engine speed
    float _minThrottle {0.05f};		// minimum throttle [0:1]
    float _K {0};			// _K = Eb/omega = tau/Ia
    float _Rm {1};    // engine windig resistence

    // Runtime state/output:
    float _torque {0};  // actual torque of the motor
    float _torque0 {1};   // nominal operating torque
};

}; // namespace yasim
#endif // _ELECTRICENGINE_HPP
