#ifndef _LAUNCHBAR_HPP
#define _LAUNCHBAR_HPP

namespace yasim {

class Ground;
class RigidBody;
struct State;

// A launchbar has the following parameters:
//
// position: a point in the aircraft's local coordinate system where the
//     fully-extended wheel will be found.
//
class Launchbar {
public:
    enum LBState { Arrested, Launch, Unmounted, Completed };

    Launchbar();

    // Externally set values
    void setLaunchbarMount(float* position);
    void setHoldbackMount(float* position);
    void setLength(float length);
    void setHoldbackLength(float length);
    void setDownAngle(float ang);
    void setUpAngle(float ang);
    void setExtension(float extension);
    void setLaunchCmd(bool cmd);
    void setGlobalGround(double *global_ground);
    void setAcceleration(float acceleration);

    void getLaunchbarMount(float* out);
    void getHoldbackMount(float* out);
    const char* getState(void);
    float getLength(void);
    float getHoldbackLength(void);
    float getDownAngle(void);
    float getUpAngle(void);
    float getExtension(void);
    bool getStrop(void);

    void getTipPosition(float* out);
    void getHoldbackTipPosition(float* out);
    float getTipPos(int i);
    float getHoldbackTipPos(int i);
    void getTipGlobalPosition(State* s, double* out);

    float getPercentPosOnCat(float* lpos, float off, float lends[2][3]);
    void getPosOnCat(float perc, float* lpos, float* lvel,
                     float lends[2][3], float lendvels[2][3]);

    // Takes a velocity of the aircraft relative to ground, a rotation
    // vector, and a ground plane (all specified in local coordinates)
    // and make a force and point of application (i.e. ground contact)
    // available via getForce().
    void calcForce(Ground *g_cb, RigidBody* body, State* s,
                   float* lv, float* lrot);

    // Computed values: total force, weight-on-wheels (force normal to
    // ground) and compression fraction.
    void getForce(float* force1, float* off1, float* force2, float* off2);
    float getCompressFraction(void);
    float getHoldbackCompressFraction(void);
    float getAngle(void);
    float getHoldbackAngle(void);
    float getLaunchbarPos(int i);
    float getHoldbackPos(int j);

private:
    float _launchbar_mount[3];
    float _holdback_mount[3];
    float _length;
    float _holdback_length;
    float _down_ang;
    float _up_ang;
    float _ang;
    float _h_ang;
    float _extension;
    float _launchbar_force[3];
    float _holdback_force[3];
    float _frac;
    float _h_frac;
    float _pos_on_cat;
    bool _launch_cmd;
    bool _strop;
    double _global_ground[4];
    LBState _state;
    float _acceleration;
};

}; // namespace yasim
#endif // _LAUNCHBAR_HPP
