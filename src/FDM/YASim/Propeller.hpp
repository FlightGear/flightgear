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
    Propeller(float radius, float v, float omega, float rho, float power);

    void setStops (float fine_stop, float coarse_stop);

    void setTakeoff(float omega0, float power0);

    void modPitch(float mod);

    void setPropPitch(float proppitch);

    void setPropFeather(int state);

    void setManualPitch();

    void calc(float density, float v, float omega,
	      float* thrustOut, float* torqueOut);

private:
    float _r;           // characteristic radius
    float _j0;          // zero-thrust advance ratio
    float _baseJ0;      //  ... uncorrected for prop advance
    float _f0;          // thrust coefficient
    float _etaC;        // Peak efficiency
    float _lambdaPeak;  // constant, ~0.759835;
    float _beta;        // constant, ~1.48058;
    float _tc0;         // thrust "coefficient" at takeoff
    float _fine_stop;   // ratio for minimum pitch (high RPM)
    float _coarse_stop; // ratio for maximum pitch (low RPM)
    bool  _matchTakeoff; // Does _tc0 mean anything?
    bool  _manual;      // manual pitch mode
    float _proppitch;   // prop pitch control setting (0 ~ 1.0)
    float _propfeather; // prop feather control setting (0 = norm, 1 = feather)
};

}; // namespace yasim
#endif // _PROPELLER_HPP
