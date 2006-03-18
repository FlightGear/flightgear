#ifndef _HOOK_HPP
#define _HOOK_HPP

namespace yasim {

class Ground;
class RigidBody;
struct State;

// A landing hook has the following parameters:
//
// position: a point in the aircraft's local coordinate system where the
//     fully-extended wheel will be found.
//
class Hook {
public:
    Hook();

    // Externally set values
    void setPosition(float* position);
    void setLength(float length);
    void setDownAngle(float ang);
    void setUpAngle(float ang);
    void setExtension(float extension);
    void setGlobalGround(double *global_ground);

    void getPosition(float* out);
    float getLength(void);
    float getDownAngle(void);
    float getUpAngle(void);
    float getExtension(void);

    void getTipPosition(float* out);
    void getTipGlobalPosition(State* s, double* out);

    // Takes a velocity of the aircraft relative to ground, a rotation
    // vector, and a ground plane (all specified in local coordinates)
    // and make a force and point of application (i.e. ground contact)
    // available via getForce().
    void calcForce(Ground *g_cb, RigidBody* body, State* s, float* lv, float* lrot);

    // Computed values: total force, weight-on-wheels (force normal to
    // ground) and compression fraction.
    void getForce(float* force, float* off);
    float getCompressFraction(void);

    float getAngle(void);
    float getHookPos(int i);

private:
    float _pos[3];
    float _length;
    float _down_ang;
    float _up_ang;
    float _ang;
    float _extension;
    float _force[3];
    float _frac;
    bool _has_wire;

    double _old_mount[3];
    double _old_tip[3];

    double _global_ground[4];
};

}; // namespace yasim
#endif // _HOOK_HPP
