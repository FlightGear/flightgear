#ifndef _ROTORPART_HPP
#define _ROTORPART_HPP

namespace yasim {

class Rotorpart
{
public:
    Rotorpart();

    // Position of this surface in local coords
    void setPosition(float* p);
    void getPosition(float* out);


    void setPositionForceAttac(float* p);
    void getPositionForceAttac(float* out);

    void setNormal(float* p);
    void getNormal(float* out);

    void setMaxPitchForce(float force);

    void setCollective(float pos);

    void setCyclic(float pos);

    void setSpeed(float* p);
    void setDirectionofZentipetalforce(float* p);
    void setZentipetalForce(float f);
    void setMaxpitch(float f);
    void setMinpitch(float f);
    void setMaxcyclic(float f);
    void setMincyclic(float f);
    void setDelta3(float f);
    void setDynamic(float f);
    void setTranslift(float f);
    void setC2(float f);
    void setZentForce(float f);
    void setRelLenHinge(float f);
    void setRelamp(float f);

    float getAlpha(int i);
    float getrealAlpha(void);
    char* getAlphaoutput(int i);
    void setAlphaoutput(char *text,int i);

    void inititeration(float dt,float *rot);
    
    float getWeight(void);
    void setWeight(float value);






    void calcForce(float* v, float rho, float* forceOut, float* torqueOut);
    void setlastnextrp(Rotorpart*lastrp,Rotorpart*nextrp,Rotorpart *oppositerp);
    void setTorque(float torque_max_force,float torque_no_force);
    void setOmega(float value);
    void setOmegaN(float value);
    float getIncidence();
    float getPhi();
    void setAlphamin(float f);
    void setAlphamax(float f);
    void setAlpha0(float f);
    void setAlpha0factor(float f);
    void setLen(float value);


private:
    void strncpy(char *dest,const char *src,int maxlen);
    Rotorpart *_lastrp,*_nextrp,*_oppositerp;

    float _dt;
    float _pos[3];    // position in local coords
    float _posforceattac[3];    // position in local coords
    float _normal[3]; //direcetion of the rotation axis
    float _torque_max_force;
    float _torque_no_force;
    float _speed[3];
    float _directionofzentipetalforce[3];
    float _zentipetalforce;
    float _maxpitch;
    float _minpitch;
    float _maxpitchforce;
    float _maxcyclic;
    float _mincyclic;
    float _cyclic;
    float _collective;
    float _delta3;
    float _dynamic;
    float _translift;
    float _c2;
    float _mass;
    float _alpha;
    float _alphaalt;
    float _alphamin,_alphamax,_alpha0,_alpha0factor;
    float _rellenhinge;
    float _relamp;
    float _omega,_omegan;
    float _phi;
    float _len;
    float _incidence;



          
    char _alphaoutputbuf[2][256];
    int _alpha2type;

};

}; // namespace yasim
#endif // _ROTORPART_HPP
