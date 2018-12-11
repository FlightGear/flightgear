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
    double pos[3] {0, 0, 0};    // position
    float  orient[9]  {1,0,0,  0,1,0,  0,0,1}; // global->local xform matrix
    float  v[3] {0, 0, 0};      // velocity
    float  rot[3] {0, 0, 0};    // rotational velocity
    float  acc[3] {0, 0, 0};    // acceleration
    float  racc[3] {0, 0, 0};   // rotational acceleration
    double dt {0};              // time offset

    // Simple initialization
    State() {
    }

    void posLocalToGlobal(const float* lpos, double *gpos) const {
        float tmp[3];
        Math::tmul33(orient, lpos, tmp);
        gpos[0] = tmp[0] + pos[0];
        gpos[1] = tmp[1] + pos[1];
        gpos[2] = tmp[2] + pos[2];
    }
    
    void posGlobalToLocal(const double* gpos, float *lpos) const {
        lpos[0] = (float)(gpos[0] - pos[0]);
        lpos[1] = (float)(gpos[1] - pos[1]);
        lpos[2] = (float)(gpos[2] - pos[2]);
        Math::vmul33(orient, lpos, lpos);
    }
    
    void localToGlobal(const float* local, float *global) const {
        Math::tmul33(orient, local, global);
    }
    
    void globalToLocal(const float* global, float *local) const {
        Math::vmul33(orient, global, local);
    }

    void planeGlobalToLocal(const double* gplane, float *lplane) const {
      // First the normal vector transformed to local coordinates.
      lplane[0] = (float)-gplane[0];
      lplane[1] = (float)-gplane[1];
      lplane[2] = (float)-gplane[2];
      Math::vmul33(orient, lplane, lplane);

      // Then the distance from the plane to the Aircraft's origin.
      lplane[3] = (float)(pos[0]*gplane[0] + pos[1]*gplane[1]
                          + pos[2]*gplane[2] - gplane[3]);
    }
    
    // used by Airplane::runCruise, runApproach, solveHelicopter and in yasim-test
    void setupOrientationFromAoa(float aoa) 
    {
      float cosAoA = Math::cos(aoa);
      float sinAoA = Math::sin(aoa);
      orient[0] =  cosAoA; orient[1] = 0; orient[2] = sinAoA;
      orient[3] =       0; orient[4] = 1; orient[5] =      0;
      orient[6] = -sinAoA; orient[7] = 0; orient[8] = cosAoA;
    }
    
    void setupSpeedAndPosition(float speed, float gla) 
    {
      
      // FIXME check axis, guess sin should go to 2 instead of 1?
      v[0] = speed*Math::cos(gla); 
      v[1] = -speed*Math::sin(gla); 
      v[2] = 0;
      for(int i=0; i<3; i++) {
        pos[i] = rot[i] = acc[i] = racc[i] = 0;
      }
      
      pos[2] = 1;
    }

    void setupState(float aoa, float speed, float gla) {
        setupOrientationFromAoa(aoa);
        setupSpeedAndPosition(speed, gla);
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
