#ifndef _ROTORBLADE_HPP
#define _ROTORBLADE_HPP

namespace yasim {

class Rotorblade
{
public:
    Rotorblade();

    // Position of this surface in local coords
    void setPosition(float* p);
    void getPosition(float* out);
    float getPhi();
    void setPhi(float value);

    void setPositionForceAttac(float* p);
    void getPositionForceAttac(float* out);

    void setNormal(float* p);
    void setFront(float* p);
    void setRight(float* p);
    void getNormal(float* out);

    void setMaxPitchForce(float force);

    void setCollective(float pos);

    void setCyclicele(float pos);
    void setCyclicail(float pos);

    void setOmega(float value);
    void setOmegaN(float value);
    void setLen(float value);
    void setLenHinge(float value);
    void setLforceattac(float value);

    void setSpeed(float p);
    void setDirectionofZentipetalforce(float* p);
    void setZentipetalForce(float f);
    void setMaxpitch(float f);
    void setDelta3(float f);
    void setDelta(float f);
    void setDeltaPhi(float f);
    void setDynamic(float f);
    void setTranslift(float f);
    void setC2(float f);
    void setStepspersecond(float steps);
    void setZentForce(float f);

    float getAlpha(int i);
    float getrealAlpha(void);
    char* getAlphaoutput(int i);
    void setAlphaoutput(char *text,int i);

    void inititeration(float dt,float *rot);
    
    float getWeight(void);
    void setWeight(float value);

    float getFlapatPos(int k);







    // local -> Rotorblade coords
    //void setOrientation(float* o);


    void calcForce(float* v, float rho, float* forceOut, float* torqueOut);
    void setlastnextrp(Rotorblade*lastrp,Rotorblade*nextrp,Rotorblade *oppositerp);
    void setTorque(float torque_max_force,float torque_no_force);
    void calcFrontRight();
    float getIncidence();
    void setAlpha0(float f);
    void setAlphamin(float f);
    void setAlphamax(float f);
    void setAlpha0factor(float f);
    void setTeeterdamp(float f);
    void setMaxteeterdamp(float f);
    void setRelLenTeeterHinge(float value);

private:
    void strncpy(char *dest,const char *src,int maxlen);
    Rotorblade *_lastrp,*_nextrp,*_oppositerp;

    float _dt;
    float _phi,_omega,_omegan;
    float _delta;
    float _deltaphi;
    int _first;
    
    float _len;
    float _lforceattac;
    float _pos[3];    // position in local coords
    float _posforceattac[3];    // position in local coords
    float _normal[3]; //direcetion of the rotation axis
    float _front[3],_right[3];
    float _lright[3],_lfront[3];
    float _torque_max_force;
    float _torque_no_force;
    float _speed;
    float _directionofzentipetalforce[3];
    float _zentipetalforce;
    float _maxpitch;
    float _maxpitchforce;
    float _cyclicele;
    float _cyclicail;
    float _collective;
    float _delta3;
    float _dynamic;
    float _flapatpos[4];//flapangle at 0, 90, 180 and 270 degree, for graphics
    float _translift;
    float _c2;
    float _mass;
    float _alpha;
    float _alphaalt;
    float _alphaomega;
    float _rellenhinge;
    float _incidence;
    float _alphamin,_alphamax,_alpha0,_alpha0factor;
    float _stepspersecond;
    float _teeter,_ddtteeter;
    float _teeterdamp,_maxteeterdamp;
    float _rellenteeterhinge;



          
    char _alphaoutputbuf[2][256];
    int _alpha2type;

    //float _orient[9]; // local->surface orthonormal matrix

    bool  _calcforcesdone;
    float _oldt[3],_oldf[3];

    float _showa,_showb;

};

}; // namespace yasim
#endif // _ROTORBLADE_HPP
