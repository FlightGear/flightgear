#ifndef _ROTOR_HPP
#define _ROTOR_HPP

#include "Vector.hpp"
#include "Rotorpart.hpp"
#include "Integrator.hpp"
#include "RigidBody.hpp"
#include "BodyEnvironment.hpp"

namespace yasim {

class Surface;
class Rotorpart;
class Ground;
const float rho_null=1.184f; //25DegC, 101325Pa

class Rotor {
    friend std::ostream &  operator<<(std::ostream & out, /*const*/ Rotor& r);
private:
    float _torque;
    float _omega,_omegan,_omegarel,_ddt_omega,_omegarelneu;
    float _phi_null;
    float _chord;
    float _taper;
    float _airfoil_incidence_no_lift;
    float _collective;
    float _airfoil_lift_coefficient;
    float _airfoil_drag_coefficient0;
    float _airfoil_drag_coefficient1;
    int _ccw;
    int _number_of_blades;
    int _number_of_segments;
    int _number_of_parts;
    float _balance1;
    float _balance2;
    float _tilt_yaw;
    float _tilt_roll;
    float _tilt_pitch;
    float _old_tilt_roll;
    float _old_tilt_pitch;
    float _old_tilt_yaw;
    float _downwash_factor;

public:
    Rotor();
    ~Rotor();

    // Rotor geometry:
    void setNormal(float* normal);
    //the normal vector (direction of rotormast, pointing up)

    void setForward(float* forward);
    //the normal vector pointing forward (for ele and ail)
    void setForceAtPitchA(float force);
    void setPowerAtPitch0(float value);
    void setPowerAtPitchB(float value);
    void setNotorque(int value);
    void setPitchA(float value);
    void setPitchB(float value);
    void setMinCyclicail(float value);
    void setMinCyclicele(float value);
    void setMaxCyclicail(float value);
    void setMaxCyclicele(float value);
    void setMaxCollective(float value);
    void setMinCollective(float value);
    void setMinTiltYaw(float value);
    void setMinTiltPitch(float value);
    void setMinTiltRoll(float value);
    void setMaxTiltYaw(float value);
    void setMaxTiltPitch(float value);
    void setMaxTiltRoll(float value);
    void setTiltCenterX(float value);
    void setTiltCenterY(float value);
    void setTiltCenterZ(float value);
    void setTiltYaw(float lval);
    void setTiltPitch(float lval);
    void setTiltRoll(float lval);
    void setDiameter(float value);
    void setWeightPerBlade(float value);
    void setNumberOfBlades(float value);
    void setRelBladeCenter(float value);
    void setDelta3(float value);
    void setDelta(float value);
    void setDynamic(float value);
    void setTranslift(float value);
    void setC2(float value);
    void setStepspersecond(float steps);
    void setRPM(float value);
    void setPhiNull(float value);
    void setRelLenHinge(float value);
    void setBase(float* base);        // in local coordinates
    void getPosition(float* out);
    void setCyclicail(float lval,float rval);
    void setCyclicele(float lval,float rval);
    void setCollective(float lval);
    void setRotorBalance(float lval);
    void setAlphaoutput(int i, const char *text);
    void setCcw(int ccw);
    int getCcw() {return _ccw;};
    void setParameter(const char *parametername, float value);
    void setGlobalGround(double* global_ground, float* global_vel);
    float getTorqueOfInertia();
    int getValueforFGSet(int j,char *b,float *f);
    void setName(const char *text);
    void inititeration(float dt,float omegarel,float ddt_omegarel,float *rot);
    void compile();
    void updateDirectionsAndPositions(float *rot);
    void getTip(float* tip);
    void calcLiftFactor(float* v, float rho, State *s);
    void getDownWash(float *pos, float * v_heli, float *downwash);
    int getNumberOfBlades(){return _number_of_blades;}
    void setDownwashFactor(float value);

    // Query the list of Rotorpart objects
    int numRotorparts();
    Rotorpart* getRotorpart(int n);
    void setAlpha0(float f);
    void setAlphamin(float f);
    void setAlphamax(float f);
    void setTeeterdamp(float f);
    void setMaxteeterdamp(float f);
    void setRelLenTeeterHinge(float value);
    void setAlpha0factor(float f);
    void setTorque(float f);
    void addTorque(float f);
    float getTorque() {return _torque;}
    float getLiftFactor();
    float getLiftCoef(float incidence,float speed);
    float getDragCoef(float incidence,float speed);
    float getOmegaRel() {return _omegarel;}
    float getOmegaRelNeu() {return _omegarelneu;}
    void setOmegaRelNeu(float orn) {_omegarelneu=orn;}
    float getOmegan() {return _omegan;}
    float getTaper() { return _taper;}
    float getChord() { return _chord;}
    int getNumberOfParts() { return _number_of_parts;}
    float getOverallStall() 
        {if (_stall_v2sum !=0 ) return _stall_sum/_stall_v2sum; else return 0;}
    float getAirfoilIncidenceNoLift() {return _airfoil_incidence_no_lift;}
    Vector _rotorparts;
    void findGroundEffectAltitude(Ground * ground_cb,State *s);
    float *getGravDirection() {return _grav_direction;}
    void writeInfo();
    void setSharedFlapHinge(bool s);
    void setBalance(float b);
    float getBalance(){ return (_balance1>0)?_balance1*_balance2:_balance1;}

private:
    void testForRotorGroundContact (Ground * ground_cb,State *s);
    void strncpy(char *dest,const char *src,int maxlen);
    void interp(float* v1, float* v2, float frac, float* out);
    float calcStall(float incidence,float speed);
    float findGroundEffectAltitude(Ground * ground_cb,State *s,
        float *pos0,float *pos1,float *pos2,float *pos3,
        int iteration=0,float a0=-1,float a1=-1,float a2=-1,float a3=-1);
    static void euler2orient(float roll, float pitch, float hdg,
                             float* out);
    Rotorpart* newRotorpart(/*float* pos, float *posforceattac, float *normal,
        float* speed,float *dirzentforce, */float zentforce,float maxpitchforce,
        float delta3,float mass,float translift,float rellenhinge,float len);
    float _base[3];
    float _groundeffectpos[4][3];
    float _ground_contact_pos[16][3];
    int _num_ground_contact_pos;
    float _ground_effect_altitude;
    //some postions, where to calcualte the ground effect
    float _normal[3];//the normal vector (direction of rotormast, pointing up)
    float _normal_with_yaw_roll[3];//the normal vector (perpendicular to rotordisc)
    float _forward[3];
    float _diameter;
    float _weight_per_blade;
    float _rel_blade_center;
    float _tilt_center[3];
    float _min_tilt_yaw;
    float _min_tilt_pitch;
    float _min_tilt_roll;
    float _max_tilt_yaw;
    float _max_tilt_pitch;
    float _max_tilt_roll;
    float _min_pitch;
    float _max_pitch;
    float _force_at_pitch_a;
    float _pitch_a;
    float _power_at_pitch_0;
    float _power_at_pitch_b;
    int _no_torque;
    int _sim_blades;
    float _pitch_b;
    float _rotor_rpm;
    float _rel_len_hinge;
    float _maxcyclicail;
    float _maxcyclicele;
    float _mincyclicail;
    float _mincyclicele;
    float _delta3;
    float _delta;
    float _dynamic;
    float _translift;
    float _c2;
    float _stepspersecond;
    char _alphaoutput[8][256];
    char _name[256];
    int _engineon;
    float _alphamin,_alphamax,_alpha0,_alpha0factor;
    float _teeterdamp,_maxteeterdamp;
    float _rellenteeterhinge;
    float _translift_ve;
    float _translift_maxfactor;
    float _ground_effect_constant;
    float _vortex_state_lift_factor;
    float _vortex_state_c1;
    float _vortex_state_c2;
    float _vortex_state_c3;
    float _vortex_state_e1;
    float _vortex_state_e2;
    float _vortex_state_e3;
    float _lift_factor,_f_ge,_f_vs,_f_tl;
    float _vortex_state;
    double _global_ground[4];
    float _liftcoef;
    float _dragcoef0;
    float _dragcoef1;
    float _twist; //outer incidence = inner inner incidence + _twist
    float _rel_len_where_incidence_is_measured;
    float _torque_of_inertia;
    float _rel_len_blade_start;
    float _incidence_stall_zero_speed;
    float _incidence_stall_half_sonic_speed;
    float _lift_factor_stall;
    float _stall_change_over;
    float _drag_factor_stall;
    float _stall_sum;
    float _stall_v2sum;
    float _yaw;
    float _roll;
    float _cyclicail;
    float _cyclicele;
    float _cyclic_factor;
    float _rotor_correction_factor;
    float _phi;
    bool _shared_flap_hinge;
    float _grav_direction[3];
    int _properties_tied;
    bool _directions_and_postions_dirty;
};
std::ostream &  operator<<(std::ostream & out, /*const*/ Rotor& r);

class Rotorgear {
private:
    int _in_use;
    int _engineon;
    float _max_power_engine;
    float _engine_prop_factor;
    float _yasimdragfactor;
    float _yasimliftfactor;
    float _rotorbrake;
    float _max_power_rotor_brake;
    float _rotorgear_friction;
    float _ddt_omegarel;
    float _engine_accel_limit;
    float _total_torque_on_engine;
    Vector _rotors;
    float _target_rel_rpm;
    float _max_rel_torque;

public:
    Rotorgear();
    ~Rotorgear();
    int isInUse() {return _in_use;}
    void setInUse() {_in_use = 1;}
    void compile();
    void addRotor(Rotor* rotor);
    int getNumRotors() {return _rotors.size();}
    Rotor* getRotor(int i) {return (Rotor*)_rotors.get(i);}
    void calcForces(float* torqueOut);
    void setParameter(char *parametername, float value);
    void setEngineOn(int value);
    int getEngineon();
    void setRotorBrake(float lval);
    void setRotorEngineMaxRelTorque(float lval);
    void setRotorRelTarget(float lval);
    float getYasimDragFactor() { return _yasimdragfactor;}
    float getYasimLiftFactor() { return _yasimliftfactor;}
    float getMaxPowerEngine() { return _max_power_engine;}
    float getMaxPowerRotorBrake() { return _max_power_rotor_brake;}
    float getRotorBrake() { return _rotorbrake;}
    float getEnginePropFactor() {return _engine_prop_factor;}
    Vector* getRotors() { return &_rotors;}
    void initRotorIteration(float *lrot,float dt);
    void getDownWash(float *pos, float * v_heli, float *downwash);
    int getValueforFGSet(int j,char *b,float *f);
};

}; // namespace yasim
#endif // _ROTOR_HPP
