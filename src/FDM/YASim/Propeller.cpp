#include <stdio.h>

#include "Atmosphere.hpp"
#include "Math.hpp"
#include "Propeller.hpp"
namespace yasim {

Propeller::Propeller(float radius, float v, float omega,
		     float rho, float power)
{
    // Initialize numeric constants:
    _lambdaPeak = Math::pow(5.0, -1.0/4.0);
    _beta = 1.0f/(Math::pow(5.0f, -1.0f/4.0f) - Math::pow(5.0f, -5.0f/4.0f));

    _r = radius;
    _etaC = 0.85f; // make this settable?

    _j0 = v/(omega*_lambdaPeak);
    _baseJ0 = _j0;

    float V2 = v*v + (_r*omega)*(_r*omega);
    _f0 = 2*_etaC*power/(rho*v*V2);

    _matchTakeoff = false;
    _manual = false;
    _proppitch = 0;
    _propfeather = 0;
}

void Propeller::setTakeoff(float omega0, float power0)
{
    // Takeoff thrust coefficient at lambda==0
    _matchTakeoff = true;
    float V2 = _r*omega0 * _r*omega0;
    float gamma = _etaC * _beta / _j0;
    float torque = power0 / omega0;
    float density = Atmosphere::getStdDensity(0);
    _tc0 = (torque * gamma) / (0.5f * density * V2 * _f0);
}

void Propeller::setStops(float fine_stop, float coarse_stop)
{
    _fine_stop = fine_stop;
    _coarse_stop = coarse_stop;
}
    
void Propeller::modPitch(float mod)
{
    _j0 *= mod;
    if(_j0 < _fine_stop*_baseJ0) _j0 = _fine_stop*_baseJ0;
    if(_j0 > _coarse_stop*_baseJ0)     _j0 = _coarse_stop*_baseJ0;
}

void Propeller::setManualPitch()
{
    _manual = true;
}

void Propeller::setPropPitch(float proppitch)
{
    // makes only positive range of axis effective.
    _proppitch = Math::clamp(proppitch, 0, 1);
}

void Propeller::setPropFeather(int state)
{
    // 0 = normal, 1 = feathered
    _propfeather = (state != 0);
}

void Propeller::calc(float density, float v, float omega,
		     float* thrustOut, float* torqueOut)
{
    // For manual pitch, exponentially modulate the J0 value between
    // 0.25 and 4.  A prop pitch of 0.5 results in no change from the
    // base value.
    // TODO: integrate with _fine_stop and _coarse_stop variables
    if (_manual) 
        _j0 = _baseJ0 * Math::pow(2, 2 - 4*_proppitch);
    
    float tipspd = _r*omega;
    float V2 = v*v + tipspd*tipspd;

    // Sanify
    if(v < 0) v = 0;
    if(omega < 0.001) omega = 0.001;

    float J = v/omega;    // Advance ratio
    float lambda = J/_j0; // Unitless scalar advance ratio

    // There's an undefined point at lambda == 1.
    if(lambda == 1.0f) lambda = 0.9999f;

    float l4 = lambda*lambda; l4 = l4*l4;   // lambda^4
    float gamma = (_etaC*_beta/_j0)*(1-l4); // thrust/torque ratio

    // Compute a thrust coefficient, with clamping at very low
    // lambdas (fast propeller / slow aircraft).
    float tc = (1 - lambda) / (1 - _lambdaPeak);
    if(_matchTakeoff && tc > _tc0) tc = _tc0;

    float thrust = 0.5f * density * V2 * _f0 * tc;
    float torque = thrust/gamma;
    if(lambda > 1) {
        // This is the negative thrust / windmilling regime.  Throw
        // out the efficiency graph approach and instead simply
        // extrapolate the existing linear thrust coefficient and a
        // torque coefficient that crosses the axis at a preset
        // windmilling speed.  The tau0 value is an analytically
        // calculated (i.e. don't mess with it) value for a torque
        // coefficient at lamda==1.
        float tau0 = (0.25f * _j0) / (_etaC * _beta * (1 - _lambdaPeak));
        float lambdaWM = 1.2f; // lambda of zero torque (windmilling)
        torque = tau0 - tau0 * (lambda - 1) / (lambdaWM - 1);
        torque *= 0.5f * density * V2 * _f0;
    }
    
    if (_propfeather) {
        thrust = 0;
    }

    *thrustOut = thrust;
    *torqueOut = torque;
}

}; // namespace yasim
