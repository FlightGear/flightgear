#ifndef _GEAR_HPP
#define _GEAR_HPP

class SGMaterial;

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
    void setPosition(float* position);
    void setCompression(float* compression);
    void setSpring(float spring);
    void setDamping(float damping);
    void setStaticFriction(float sfric);
    void setDynamicFriction(float dfric);
    void setBrake(float brake);
    void setRotation(float rotation);
    void setExtension(float extension);
    void setCastering(bool castering);
    void setOnWater(bool c);
    void setOnSolid(bool c);
    void setSpringFactorNotPlaning(float f);
    void setSpeedPlaning(float s);
    void setReduceFrictionByExtension(float s);
    void setInitialLoad(float l);
    void setIgnoreWhileSolving(bool c);
    void setGlobalGround(double* global_ground, float* global_vel,
        double globalX, double globalY,
        const SGMaterial *material);
    void getPosition(float* out);
    void getCompression(float* out);
    void getGlobalGround(double* global_ground);
    float getSpring();
    float getDamping();
    float getStaticFriction();
    float getDynamicFriction();
    float getBrake();
    float getRotation();
    float getExtension();
    float getInitialLoad() {return _initialLoad; }
    bool getCastering();
    float getCasterAngle() { return _casterAngle; }
    float getRollSpeed() { return _rollSpeed; }
    float getBumpAltitude();
    bool getGroundIsSolid();
    float getGroundFrictionFactor() { return (float)_ground_frictionFactor; }

    // Takes a velocity of the aircraft relative to ground, a rotation
    // vector, and a ground plane (all specified in local coordinates)
    // and make a force and point of application (i.e. ground contact)
    // available via getForce().
    void calcForce(RigidBody* body, State* s, float* v, float* rot);

    // Computed values: total force, weight-on-wheels (force normal to
    // ground) and compression fraction.
    void getForce(float* force, float* contact);
    float getWoW();
    float getCompressFraction();
    float getCompressDist() { return _compressDist; }
    bool getSubmergable() {return (!_ground_isSolid)&&(!_isContactPoint); }
    bool getIgnoreWhileSolving() {return _ignoreWhileSolving; }
    void setContactPoint(bool c);

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
