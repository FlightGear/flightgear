#include "Atmosphere.hpp"
#include "Math.hpp"
#include "Propeller.hpp"
namespace yasim {

Propeller::Propeller(float radius, float v, float omega,
		     float rho, float power, float omega0,
                     float power0)
{
    // Initialize numeric constants:
    _lambdaPeak = Math::pow(9.0, -1.0/8.0);
    _beta = 1.0/(Math::pow(9.0, -1.0/8.0) - Math::pow(9.0, -9.0/8.0));

    _r = radius;
    _etaC = 0.85; // make this settable?

    _J0 = v/(omega*_lambdaPeak);

    float V2 = v*v + (_r*omega)*(_r*omega);
    _F0 = 2*_etaC*power/(rho*v*V2);

    float stallAngle = 0.25;
    _lambdaS = _r*(_J0/_r - stallAngle) / _J0;

    // Now compute a correction for zero forward speed to make the
    // takeoff performance correct.
    float torque0 = power0/omega0; 
    float thrust, torque;
    _takeoffCoef = 1;
    calc(Atmosphere::getStdDensity(0), 0, omega0, &thrust, &torque);
    _takeoffCoef = torque/torque0;
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
	torque = (density*V2*_F0*_J0)/(8*_etaC*_beta*(1-_lambdaPeak));
    }

    // There's an undefined point at 1.  Just offset by a tiny bit to
    // fix (note: the discontinuity is at EXACTLY one, this is about
    // the only time in history you'll see me use == on a floating
    // point number!)
    if(lambda == 1) lambda = 0.9999;

    // Compute thrust, remembering to clamp lambda to the stall point
    float lambda2 = lambda < _lambdaS ? _lambdaS : lambda;
    float thrust = (0.5*density*V2*_F0/(1-_lambdaPeak))*(1-lambda2);

    // Calculate lambda^8
    float l8 = lambda*lambda; l8 = l8*l8; l8 = l8*l8;

    // thrust/torque ratio
    float gamma = (_etaC*_beta/_J0)*(1-l8);

    // Correct slow speeds to get the takeoff parameters correct
    if(lambda < _lambdaPeak) {
        // This will interpolate takeoffCoef along a descending from 1
        // at lambda==0 to 0 at the peak, fairing smoothly into the
        // flat peak.
        float frac = (lambda - _lambdaPeak)/_lambdaPeak;
        gamma *= 1 + (_takeoffCoef - 1)*frac*frac;
    }

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
