#ifndef _GEAR_HPP
#define _GEAR_HPP
#include "Math.hpp"

namespace simgear {
class BVHMaterial;
}

namespace yasim {

class RigidBody;
struct State;

// A landing gear has the following parameters:
//
// position: a point in the aircraft's local coordinate system where the
//     fully-extended wheel will be found.
// compression: a vector from position where a fully-compressed wheel
//     will be, also in aircraft-local coordinates.
// spring coefficient: force coefficient along the compression axis, in
//     Newtons per meter.
// damping coefficient: force per compression speed, in Ns/m
// static coefficient of friction: force along the ground plane exerted
//     by a motionless wheel.  A unitless scalar.
// dynamic coefficient of friction: force along the ground plane exerted
//     by a sliding/skidding wheel.
// braking fraction: fraction of the dynamic friction that will be
//     actually exerted by a rolling wheel
// rotation: the angle from "forward" by which the wheel is rotated
//     around its compression axis.  In radians.
//
class Gear {
public:
    Gear();

    // Externally set values
    void setPosition(float* position) { Math::set3(position, _pos); }
    void getPosition(float* out) { Math::set3(_pos, out); }
    void setCompression(float* compression) { Math::set3(compression, _cmpr); }
    void getCompression(float* out) { Math::set3(_cmpr, out); }
    void setSpring(float spring) { _spring = spring; }
    float getSpring() { return _spring; }
    void setDamping(float damping) { _damp = damping; }
    float getDamping() {return _damp; }
    void setStaticFriction(float sfric) { _sfric = sfric; }
    float getStaticFriction() { return _sfric; }
    void setDynamicFriction(float dfric) { _dfric = dfric; }
    float getDynamicFriction() { return _dfric; }
    void setBrake(float brake) { _brake = Math::clamp(brake, 0, 1); }
    float getBrake() { return _brake; }
    void setRotation(float rotation) { _rot = rotation; }
    float getRotation() { return _rot; }
    void setExtension(float extension) { _extension = Math::clamp(extension, 0, 1); }  
    float getExtension() { return _extension; }
    void setCastering(bool c) { _castering = c; }
    bool getCastering() { return _castering; }
    void setOnWater(bool c) { _onWater = c; }
    void setOnSolid(bool c) { _onSolid = c; }
    void setSpringFactorNotPlaning(float f) { _spring_factor_not_planing = f; }
    void setSpeedPlaning(float s) { _speed_planing = s; }
    void setReduceFrictionByExtension(float s) { _reduceFrictionByExtension = s; }
    void setInitialLoad(float l) { _initialLoad = l; }
    float getInitialLoad() {return _initialLoad; }
    void setIgnoreWhileSolving(bool c) { _ignoreWhileSolving = c; }

    void setGlobalGround(double* global_ground, float* global_vel,
        double globalX, double globalY,
        const simgear::BVHMaterial *material);
    void getGlobalGround(double* global_ground);
    float getCasterAngle() { return _casterAngle; }
    float getRollSpeed() { return _rollSpeed; }
    float getBumpAltitude();
    bool getGroundIsSolid() { return _ground_isSolid; }
    float getGroundFrictionFactor() { return (float)_ground_frictionFactor; }
    void integrate(float dt);

    // Takes a velocity of the aircraft relative to ground, a rotation
    // vector, and a ground plane (all specified in local coordinates)
    // and make a force and point of application (i.e. ground contact)
    // available via getForce().
    void calcForce(RigidBody* body, State* s, float* v, float* rot);

    // Computed values: total force, weight-on-wheels (force normal to
    // ground) and compression fraction.
    void getForce(float* force, float* contact);
    float getWoW() { return _wow; }
    float getCompressFraction() { return _frac; }
    float getCompressDist() { return _compressDist; }
    bool getSubmergable() {return (!_ground_isSolid)&&(!_isContactPoint); }
    bool getIgnoreWhileSolving() {return _ignoreWhileSolving; }
    void setContactPoint(bool c) { _isContactPoint=c; }

private:
    float calcFriction(float wgt, float v);
    float calcFrictionFluid(float wgt, float v);

    bool _castering;
    float _pos[3];
    float _cmpr[3];
    float _spring;
    float _damp;
    float _sfric;
    float _dfric;
    float _brake;
    float _rot;
    float _extension;
    float _force[3];
    float _contact[3];
    float _wow;
    float _frac;
    float _initialLoad;
    float _compressDist;
    double _global_ground[4];
    float _global_vel[3];
    float _casterAngle;
    float _rollSpeed;
    bool _isContactPoint;
    bool _onWater;
    bool _onSolid;
    float _spring_factor_not_planing;
    float _speed_planing;
    float _reduceFrictionByExtension;
    bool _ignoreWhileSolving;

    double _ground_frictionFactor;
    double _ground_rollingFriction;
    double _ground_loadCapacity;
    double _ground_loadResistance;
    double _ground_bumpiness;
    bool _ground_isSolid;
    double _global_x;
    double _global_y;
};

}; // namespace yasim
#endif // _GEAR_HPP
