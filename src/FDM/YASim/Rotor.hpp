#ifndef _ROTOR_HPP
#define _ROTOR_HPP

#include "Vector.hpp"
#include "Rotorpart.hpp"
#include "Rotorblade.hpp"
namespace yasim {

class Surface;
class Rotorpart;

class Rotor {
public:
    Rotor();
    ~Rotor();


    // Rotor geometry:
    void setNormal(float* normal);    //the normal vector (direction of rotormast, pointing up)
    void setForward(float* forward);    //the normal vector pointing forward (for ele and ail)
    //void setMaxPitchForce(float force);
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
    void setRelLenHinge(float value);
    void setBase(float* base);        // in local coordinates
    void setCyclicail(float lval,float rval);
    void setCyclicele(float lval,float rval);
    void setCollective(float lval);
    void setAlphaoutput(int i, const char *text);
    void setCcw(int ccw);
    void setSimBlades(int value);
    void setEngineOn(int value);

    int getValueforFGSet(int j,char *b,float *f);
    void setName(const char *text);
    void inititeration(float dt);
    void compile();

    void getTip(float* tip);



    // Ground effect information, stil missing
    float getGroundEffect(float* posOut);
    
    // Query the list of Rotorpart objects
    int numRotorparts();
    Rotorpart* getRotorpart(int n);
    // Query the list of Rotorblade objects
    int numRotorblades();
    Rotorblade* getRotorblade(int n);
    void setAlpha0(float f);
    void setAlphamin(float f);
    void setAlphamax(float f);
    void setTeeterdamp(float f);
    void setMaxteeterdamp(float f);
    void setRelLenTeeterHinge(float value);
    void setAlpha0factor(float f);

private:
    void strncpy(char *dest,const char *src,int maxlen);
    void interp(float* v1, float* v2, float frac, float* out);
    Rotorpart* newRotorpart(float* pos, float *posforceattac, float *normal,
         float* speed,float *dirzentforce, float zentforce,float maxpitchforce,float maxpitch, float minpitch, float mincyclic,float maxcyclic,
         float delta3,float mass,float translift,float rellenhinge,float len);

    Rotorblade* newRotorblade(
         float* pos,  float *normal,float *front, float *right,
         float lforceattac,float relenhinge,
         float *dirzentforce, float zentforce,float maxpitchforce,float maxpitch, 
         float delta3,float mass,float translift,float phi,float omega,float len,float speed);
    


    Vector _rotorparts;
    Vector _rotorblades;

    float _base[3];

    float _normal[3];//the normal vector (direction of rotormast, pointing up)
    float _forward[3];
    float _diameter;
    int _number_of_blades;
    float _weight_per_blade;
    float _rel_blade_center;
    float _min_pitch;
    float _max_pitch;
    float _force_at_max_pitch;
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
    int _ccw;
    int _engineon;
    float _omega,_omegan,_omegarel;
    float _alphamin,_alphamax,_alpha0,_alpha0factor;
    float _teeterdamp,_maxteeterdamp;
    float _rellenteeterhinge;






};

}; // namespace yasim
#endif // _ROTOR_HPP
