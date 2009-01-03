#ifndef _BODYENVIRONMENT_HPP
#define _BODYENVIRONMENT_HPP

#include "RigidBody.hpp"
#include "Math.hpp"

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
        int i;
        for(i=0; i<3; i++) {
            pos[i] = v[i] = rot[i] = acc[i] = racc[i] = 0;
            int j;
            for(j=0; j<3; j++)
                orient[3*i+j] = i==j ? 1.0f : 0.0f;
        }
    }

    void posLocalToGlobal(float* lpos, double *gpos) {
        float tmp[3];
        Math::tmul33(orient, lpos, tmp);
        gpos[0] = tmp[0] + pos[0];
        gpos[1] = tmp[1] + pos[1];
        gpos[2] = tmp[2] + pos[2];
    }
    void posGlobalToLocal(double* gpos, float *lpos) {
        lpos[0] = (float)(gpos[0] - pos[0]);
        lpos[1] = (float)(gpos[1] - pos[1]);
        lpos[2] = (float)(gpos[2] - pos[2]);
        Math::vmul33(orient, lpos, lpos);
    }
    void velLocalToGlobal(float* lvel, float *gvel) {
        Math::tmul33(orient, lvel, gvel);
    }
    void velGlobalToLocal(float* gvel, float *lvel) {
        Math::vmul33(orient, gvel, lvel);
    }

    void planeGlobalToLocal(double* gplane, float *lplane) {
      // First the normal vector transformed to local coordinates.
      lplane[0] = (float)-gplane[0];
      lplane[1] = (float)-gplane[1];
      lplane[2] = (float)-gplane[2];
      Math::vmul33(orient, lplane, lplane);

      // Then the distance from the plane to the Aircraft's origin.
      lplane[3] = (float)(pos[0]*gplane[0] + pos[1]*gplane[1]
                          + pos[2]*gplane[2] - gplane[3]);
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

    virtual ~BodyEnvironment() {} // #!$!?! gcc warning...
};

}; // namespace yasim
#endif // _BODYENVIRONMENT_HPP
