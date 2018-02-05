#ifndef _RIGIDBODY_HPP
#define _RIGIDBODY_HPP
#include <simgear/props/props.hxx>
#include "Vector.hpp"
#include "Math.hpp"

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
    SGPropertyNode_ptr _bodyN;
public:
    RigidBody();
    ~RigidBody();

    // Adds a point mass to the system.  Returns a handle so the gyro
    // can be later modified via setMass().
    int addMass(float mass, const float* pos, bool isStatic = false);

    // Modifies a previously-added point mass (fuel tank running dry,
    // gear going up, swing wing swinging, pilot bailing out, etc...)
    void setMass(int handle, float mass);
    void setMass(int handle, float mass, const float* pos, bool isStatic = false);

    int numMasses() const { return _nMasses; }
    float getMass(int handle) const { return _masses[handle].m; }
    void getMassPosition(int handle, float* out) const;
    float getTotalMass() const { return _totalMass; }

    // The velocity, in local coordinates, of the specified point on a
    // body rotating about its c.g. with velocity rot.
    void pointVelocity(const float* pos, const float* rot, float* out);

    // Sets the "gyroscope" for the body.  This is the total
    // "intrinsic" angular momentum of the body; that is, rotations of
    // sub-objects, NOT rotation of the whole body within the global
    // frame.  Because angular momentum is additive in this way, we
    // don't need to specify specific gyro objects; just add all their
    // momenta together and set it here.
    void setGyro(const float* angularMomentum) { Math::set3(angularMomentum, _gyro); }


    // When masses are moved or changed, this object needs to
    // regenerate its internal tables.  This step is expensive, so
    // it's exposed to the client who can amortize the call across
    // multiple changes. see also _recalcStatic() 
    /// calculate the total mass, centre of gravity and inertia tensor
    void recalc();

    /// Resets the current force/torque parameters to zero.
    void reset();

    /// Applies a force at the center of gravity.
    void addForce(const float* force) { Math::add3(_force, force, _force); }
    
    /// Applies a force at the specified position.
    void addForce(const float* pos, const float* force); 
    
    /// Adds a torque with the specified axis and magnitude
    void addTorque(const float* torque) { Math::add3(_torque, torque, _torque); }
    void setTorque(const float* torque) { Math::set3(torque, _torque); }

    // Sets the rotation rate of the body (about its c.g.) within the
    // surrounding environment.  This is needed to compute torque on
    // the body due to the centripetal forces involved in the
    // rotation.  NOTE: the rotation vector, like all other
    // coordinates used here, is specified IN THE LOCAL COORDINATE
    // SYSTEM.
    void setBodySpin(const float* rotation) { Math::set3(rotation, _spin); }

    // Returns the center of gravity of the masses, in the body
    // coordinate system. valid only after recalc()
    void getCG(float* cgOut) const { Math::set3(_cg, cgOut); }

    // Returns the acceleration of the body's c.g. relative to the
    // rest of the world, specified in local coordinates.
    void getAccel(float* accelOut) const { Math::mul3(1/_totalMass, _force, accelOut); }

    // Returns the acceleration of a specific location in local
    // coordinates.  If the body is rotating, this will be different
    // from the c.g. acceleration due to the centripetal accelerations
    // of points not on the rotation axis.
    void getAccel(const float* pos, float* accelOut) const;

    // Returns the instantaneous rate of change of the angular
    // velocity, as a vector in local coordinates.
    void getAngularAccel(float* accelOut) const;
    
    // Returns the intertia tensor in a float[9] allocated by caller.
    void getInertiaMatrix(float* inertiaOut) const;

private:
    /** 
    Most of the mass points do not change after compilation of the aircraft so
    they can be replaced by one aggregated mass at the c.g. of the static masses.
    The isStatic flag is used to mark those masses.
    */
    struct Mass { float m; float p[3]; bool isStatic; };
    void _recalcStatic(); /// aggregate static masses
    Mass  _staticMass;		/// aggregated static masses, calculated once
    Mass* _masses;        /// mass elements
    int   _nMasses;       /// number of masses
    int   _massesAlloced; /// counter for memory allocation

    float _totalMass;
    float _cg[3];
    float _gyro[3];

    // Inertia tensor, and its inverse.  Computed from the above.
    float _tI_static[9];
    float _tI[9];
    float _invI[9];

    // Externally determined quantities
    float _force[3];
    float _torque[3];
    float _spin[3];
};

}; // namespace yasim
#endif // _RIGIDBODY_HPP
