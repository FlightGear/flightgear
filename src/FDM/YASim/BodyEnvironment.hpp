#ifndef _BODYENVIRONMENT_HPP
#define _BODYENVIRONMENT_HPP

#include "RigidBody.hpp"

namespace yasim {

// The values for position and orientation within the global reference
// frame, along with their first and second time derivatives.  The
// orientation is stored as a matrix for simplicity.  Note that
// because it is orthonormal, its inverse is simply its transpose.
// You can get local->global transformations by calling Math::tmul33()
// and using the same matrix.
struct State {
    double pos[3];    // position
    float  orient[9]; // global->local xform matrix
    float  v[3];      // velocity
    float  rot[3];    // rotational velocity
    float  acc[3];    // acceleration
    float  racc[3];   // rotational acceleration

    // Simple initialization
    State() {
        for(int i=0; i<3; i++) {
            pos[i] = v[i] = rot[i] = acc[i] = racc[i] = 0;
            for(int j=0; j<3; j++)
                orient[3*i+j] = i==j ? 1 : 0;
        }
    }
};

//
// Objects implementing this interface are responsible for calculating
// external forces on a RigidBody object.  These will then be used by
// an Integrator to decide on a new solution to the state equations,
// which will be reported to the BodyEnvironment for further action.
//
class BodyEnvironment
{
public:
    // This method inspects the "environment" in which a RigidBody
    // exists, calculates forces and torques on the body, and sets
    // them via addForce()/addTorque().  Because this method is called
    // multiple times ("trials") as part of a Runge-Kutta integration,
    // this is NOT the place to make decisions about anything but the
    // forces on the object.  Note that the acc and racc fields of the
    // passed-in State object are undefined! (They are calculed BY
    // this method).
    virtual void calcForces(State* state) = 0;

    // Called when the RK4 integrator has determined a "real" new
    // point on the curve of life.  Any side-effect producing checks
    // of body state vs. the environment can happen here (crashes,
    // etc...).
    virtual void newState(State* state) = 0;
};

}; // namespace yasim
#endif // _BODYENVIRONMENT_HPP
