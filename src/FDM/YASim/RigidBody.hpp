#ifndef _RIGIDBODY_HPP
#define _RIGIDBODY_HPP

namespace yasim {

//
// A RigidBody object maintains all "internal" state about an object,
// accumulates force and torque information from external sources, and
// calculates the resulting accelerations.
//
//
// Units note: obviously, the choice of mass, time and distance units
// is up to the user.  If you provide mass in kilograms, forces in
// newtons, and torques in newton-meters, you'll get your
// accelerations back in m/s^2.  The angular units, however, are
// UNIFORMLY radians.  Angular velocities are radians per <time unit>,
// the angular acceleration you get back is radians per <time unit>^2,
// and the angular momenta supplied to setGyro must be in radians,
// too.  Radians, not degrees.  Don't forget.
//
class RigidBody
{
public:
    RigidBody();
    ~RigidBody();

    // Adds a point mass to the system.  Returns a handle so the gyro
    // can be later modified via setMass().
    int addMass(float mass, float* pos);

    // Modifies a previously-added point mass (fuel tank running dry,
    // gear going up, swing wing swinging, pilot bailing out, etc...)
    void setMass(int handle, float mass);
    void setMass(int handle, float mass, float* pos);

    int numMasses();
    float getMass(int handle);
    void getMassPosition(int handle, float* out);
    float getTotalMass();

    // The velocity, in local coordinates, of the specified point on a
    // body rotating about its c.g. with velocity rot.
    void pointVelocity(float* pos, float* rot, float* out);

    // Sets the "gyroscope" for the body.  This is the total
    // "intrinsic" angular momentum of the body; that is, rotations of
    // sub-objects, NOT rotation of the whole body within the global
    // frame.  Because angular momentum is additive in this way, we
    // don't need to specify specific gyro objects; just add all their
    // momenta together and set it here.
    void setGyro(float* angularMomentum);


    // When masses are moved or changed, this object needs to
    // regenerate its internal tables.  This step is expensive, so
    // it's exposed to the client who can amortize the call across
    // multiple changes.
    void recalc();

    // Resets the current force/torque parameters to zero.
    void reset();


    // Applies a force at the specified position.
    void addForce(float* pos, float* force);

    // Applies a force at the center of gravity.
    void addForce(float* force);

    // Adds a torque with the specified axis and magnitude
    void addTorque(float* torque);

    // Sets the rotation rate of the body (about its c.g.) within the
    // surrounding environment.  This is needed to compute torque on
    // the body due to the centripetal forces involved in the
    // rotation.  NOTE: the rotation vector, like all other
    // coordinates used here, is specified IN THE LOCAL COORDINATE
    // SYSTEM.
    void setBodySpin(float* rotation);



    // Returns the center of gravity of the masses, in the body
    // coordinate system.
    void getCG(float* cgOut);

    // Returns the acceleration of the body's c.g. relative to the
    // rest of the world, specified in local coordinates.
    void getAccel(float* accelOut);

    // Returns the acceleration of a specific location in local
    // coordinates.  If the body is rotating, this will be different
    // from the c.g. acceleration due to the centripetal accelerations
    // of points not on the rotation axis.
    void getAccel(float* pos, float* accelOut);

    // Returns the instantaneous rate of change of the angular
    // velocity, as a vector in local coordinates.
    void getAngularAccel(float* accelOut);
    
    // Returns the intertia tensor in a float[9] allocated by caller.
    void getInertiaMatrix(float* inertiaOut);

private:
    struct Mass { float m; float p[3]; };

    // Internal "rotational structure"
    Mass* _masses;
    int   _nMasses;
    int   _massesAlloced;
    float _totalMass;
    float _cg[3];
    float _gyro[3];

    // Inertia tensor, and its inverse.  Computed from the above.
    float _tI[9];
    float _invI[9];

    // Externally determined quantities
    float _force[3];
    float _torque[3];
    float _spin[3];
};

}; // namespace yasim
#endif // _RIGIDBODY_HPP
