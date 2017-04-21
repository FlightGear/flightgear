#ifndef _INTEGRATOR_HPP
#define _INTEGRATOR_HPP

#include "BodyEnvironment.hpp"
#include "RigidBody.hpp"

namespace yasim {

//
// These objects are responsible for extracting force data from a
// BodyEnvironment object, using a RigidBody object to calculate
// accelerations, and then tying that all together into a new
// "solution" of position/orientation/etc... for the body.  The method
// used is a fourth-order Runge-Kutta integration.
//
class Integrator
{
public:
    // Sets the RigidBody that will be integrated.
    void setBody(RigidBody* body) { _body = body; }  

    // Sets the BodyEnvironment object used to calculate the second
    // derivatives.
    void setEnvironment(BodyEnvironment* env) { _env = env; }

    // Sets the time interval between steps.  Units can be anything,
    // but they must match the other values (if you specify speed in
    // m/s, then angular acceleration had better be in 1/s^2 and the
    // time interval should be in seconds, etc...)
    void setInterval(float dt) { _dt = dt; }
    float getInterval() const { return _dt; }

    // The current state, i.e. initial conditions for the next
    // integration iteration.  Note that the acceleration parameters
    // in the State object are ignored.
    State* getState() {  return &_s; }
    void setState(State* s) {  _s = *s; }

    // Do a 4th order Runge-Kutta integration over one time interval.
    // This is the top level of the simulation.
    void calcNewInterval();

private:
    void orthonormalize(float* m);
    void rotMatrix(float* r, float dt, float* out);
    
    // Transforms a "local" vector to a "global" vector (not coordinate!)
    // using the specified orientation.    
    void l2gVector(const float* orient, const float* v, float* out) { 
      Math::tmul33(orient, v, out); }
    
    void extrapolatePosition(double* pos, float* v, float dt, float* o1, float* o2);

    BodyEnvironment* _env;
    RigidBody* _body;
    float _dt;

    State _s;
};

}; // namespace yasim
#endif // _INTEGRATOR_HPP
