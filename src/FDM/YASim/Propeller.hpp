#ifndef _PROPELLER_HPP
#define _PROPELLER_HPP

namespace yasim {

// A generic propeller model.  See the TeX documentation for
// implementation details, this is too hairy to explain in code
// comments.
class Propeller
{
public:
    // Initializes a propeller with the specified "cruise" numbers
    // for airspeed, RPM, power and air density, and two "takeoff"
    // numbers for RPM and power (with air speed and density being
    // zero and sea level).  RPM values are in radians per second, of
    // course.
    Propeller(float radius, float v, float omega, float rho, float power,
              float omega0, float power0);

    void calc(float density, float v, float omega,
	      float* thrustOut, float* torqueOut);

private:
    float _r;           // characteristic radius
    float _J0;          // zero-thrust advance ratio
    float _lambdaS;     // "propeller stall" normalized advance ratio
    float _F0;          // thrust coefficient
    float _etaC;        // Peak efficiency
    float _lambdaPeak;  // constant, ~0.759835;
    float _beta;        // constant, ~1.48058;
    float _takeoffCoef; // correction to get the zero-speed torque right
};

}; // namespace yasim
#endif // _PROPELLER_HPP
