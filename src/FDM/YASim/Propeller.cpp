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
    _beta = 1.0/(Math::pow(5.0, -1.0/4.0) - Math::pow(5.0, -5.0/4.0));

    _r = radius;
    _etaC = 0.85; // make this settable?

    _J0 = v/(omega*_lambdaPeak);
    _baseJ0 = _J0;

    float V2 = v*v + (_r*omega)*(_r*omega);
    _F0 = 2*_etaC*power/(rho*v*V2);

    _matchTakeoff = false;
}

void Propeller::setTakeoff(float omega0, float power0)
{
    // Takeoff thrust coefficient at lambda==0
    _matchTakeoff = true;
    float V2 = _r*omega0 * _r*omega0;
    float gamma = _etaC * _beta / _J0;
    float torque = power0 / omega0;
    float density = Atmosphere::getStdDensity(0);
    _tc0 = (torque * gamma) / (0.5 * density * V2 * _F0);
}
    
void Propeller::modPitch(float mod)
{
    _J0 *= mod;
    if(_J0 < 0.25*_baseJ0) _J0 = 0.25*_baseJ0;
    if(_J0 > 4*_baseJ0)    _J0 = 4*_baseJ0;
}


void Propeller::calc(float density, float v, float omega,
		     float* thrustOut, float* torqueOut)
{
    float tipspd = _r*omega;
    float V2 = v*v + tipspd*tipspd;

    // Clamp v (forward velocity) to zero, now that we've used it to
    // calculate V (propeller "speed")
    if(v < 0) v = 0;

    float J = v/omega;
    float lambda = J/_J0;

    float torque = 0;
    if(lambda > 1) {
	lambda = 1.0/lambda;
	torque = (density*V2*_F0*_J0)/(4*_etaC*_beta*(1-_lambdaPeak));
    }

    // There's an undefined point at 1.  Just offset by a tiny bit to
    // fix (note: the discontinuity is at EXACTLY one, this is about
    // the only time in history you'll see me use == on a floating
    // point number!)
    if(lambda == 1) lambda = 0.9999;

    // Calculate lambda^4
    float l4 = lambda*lambda; l4 = l4*l4;

    // thrust/torque ratio
    float gamma = (_etaC*_beta/_J0)*(1-l4);

    // Compute a thrust, clamp to takeoff thrust to prevend huge
    // numbers at slow speeds.
    float tc = (1 - lambda) / (1 - _lambdaPeak);
    if(_matchTakeoff && tc > _tc0) tc = _tc0;

    float thrust = 0.5 * density * V2 * _F0 * tc;

    if(torque > 0) {
	torque -= thrust/gamma;
	thrust = -thrust;
    } else {
	torque = thrust/gamma;
    }

    *thrustOut = thrust;
    *torqueOut = torque;
}

}; // namespace yasim
