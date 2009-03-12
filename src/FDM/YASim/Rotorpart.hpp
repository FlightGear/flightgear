#ifndef _ROTORPART_HPP
#define _ROTORPART_HPP
#include <iosfwd>

namespace yasim {
    class Rotor;
    class Rotorpart
    {
        friend std::ostream &  operator<<(std::ostream & out, const Rotorpart& rp);
    private:
        float _dt;
        float _last_torque[3];
        int _compiled;
    public:
        Rotorpart();

        // Position of this surface in local coords
        void setPosition(float* p);
        void getPosition(float* out);
        void setCompiled() {_compiled=1;}
        float getDt() {return _dt;}
        void setPositionForceAttac(float* p);
        void getPositionForceAttac(float* out);
        void setNormal(float* p);
        void getNormal(float* out);
        void setCollective(float pos);
        void setCyclic(float pos);
        void getLastTorque(float *t)
            {for (int i=0;i<3;i++) t[i]=_last_torque[i];}
        void getAccelTorque(float relaccel,float *t);
        void setSpeed(float* p);
        void setDirectionofZentipetalforce(float* p);
        void setDirectionofRotorPart(float* p);
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
        void setDiameter(float f);
        float getAlpha(int i);
        float getrealAlpha(void);
        char* getAlphaoutput(int i);
        void setAlphaoutput(char *text,int i);
        void inititeration(float dt,float *rot);
        float getWeight(void);
        void setWeight(float value);
        void calcForce(float* v, float rho, float* forceOut, float* torqueOut,
            float* torque_scalar);
        float calculateAlpha(float* v, float rho, float incidence, float cyc,
            float alphaalt, float *torque,float *returnlift=0);
        void setlastnextrp(Rotorpart*lastrp,Rotorpart*nextrp,
            Rotorpart *oppositerp,Rotorpart*last90rp,Rotorpart*next90rp);
        void setTorque(float torque_max_force,float torque_no_force);
        void setOmega(float value);
        void setOmegaN(float value);
        void setPhi(float value);
        void setDdtOmega(float value);
        float getIncidence();
        float getPhi();
        void setAlphamin(float f);
        void setAlphamax(float f);
        void setAlpha0(float f);
        void setAlpha0factor(float f);
        void setLen(float value);
        void setParameter(const char *parametername, float value);
        void setRotor(Rotor *rotor);
        void setTorqueOfInertia(float toi);
        void writeInfo(std::ostringstream &buffer);
        void setSharedFlapHinge(bool s);
        void setDirection(float direction);
        float getAlphaAlt() {return _alphaalt;}

    private:
        void strncpy(char *dest,const char *src,int maxlen);
        Rotorpart *_lastrp,*_nextrp,*_oppositerp,*_last90rp,*_next90rp;
        Rotor *_rotor;

        float _pos[3];    // position in local coords
        float _posforceattac[3];    // position in local coords
        float _normal[3]; //direcetion of the rotation axis
        float _torque_max_force;
        float _torque_no_force;
        float _speed[3];
        float _direction_of_movement[3];
        float _directionofcentripetalforce[3];
        float _directionofrotorpart[3];
        float _centripetalforce;
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
        float _omega,_omegan,_ddt_omega;
        float _phi;
        float _len;
        float _incidence;
        float _twist; //outer incidence = inner inner incidence + _twist
        int _number_of_segments;
        float _rel_len_where_incidence_is_measured;
        float _rel_len_blade_start;
        float _diameter;
        float _torque_of_inertia;
        float _torque; 
        // total torque of rotor (scalar) for calculating new rotor rpm
        char _alphaoutputbuf[2][256];
        int _alpha2type;
        float _rotor_correction_factor;
        bool _shared_flap_hinge;
        float _direction;
        float _balance;
    };
    std::ostream &  operator<<(std::ostream & out, const Rotorpart& rp);
}; // namespace yasim
#endif // _ROTORPART_HPP
