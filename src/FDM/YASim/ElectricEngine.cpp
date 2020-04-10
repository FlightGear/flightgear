//
// Based on original patch from Ian Dall <ian@beware.dropbear.id.au>
// Date:   Wed Jan 11 22:35:24 2012 +1030
// #595 Support for electric motors in YASim (patch provided)
// Improved by ThunderFly s.r.o. <info@thunderfly.cz>
////////////////////////////////////////////////////////

#include "Atmosphere.hpp"
#include "Math.hpp"
#include "ElectricEngine.hpp"
namespace yasim {

// idealized DC electric motor model
ElectricEngine::ElectricEngine(float voltage, float Kv, float Rm) :
    _Rm(Rm)
{
    _running = false;
    _omega0 = voltage * Kv; // calculate RPM of unloaded motor
    _K = 1/Kv; // calculate reciprocal value of motor velocity constant
    _torque0 = _K * _K * _omega0 / _Rm; // rough estimate of full load torque valid for DC electric motor invalid for BLDC
}

void ElectricEngine::calc(float pressure, float temp, float omega)
{
    _running = (_throttle > _minThrottle);
    // There are a huge variety of electronic speed controls and there
    // is nothing to say that there isn't feedback to provide
    // essentially an arbitrary relationship between throttle and
    // omega. However, below is essentially equivalent to an idealised
    // DC motor where the throttle controls the terminal voltage. PWM
    // controllers in conjunction with the inductance of the windings
    // should approximate this.
    if (_running) {
        _torque = ((_throttle - _minThrottle) / (1 - _minThrottle) - omega / _omega0) * _torque0;
    }
    else {
        _torque = 0;
    }
}

}; // namespace yasim
