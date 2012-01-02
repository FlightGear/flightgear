
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/debug/logstream.hxx>

#include "Math.hpp"
#include <Main/fg_props.hxx>
#include "Surface.hpp"
#include "Rotorpart.hpp"
#include "Glue.hpp"
#include "Ground.hpp"
#include "Rotor.hpp"

#include <iostream>
#include <iomanip>

#ifdef TEST_DEBUG
#include <stdio.h>
#endif
#include <string.h>
#include <iostream>
#include <sstream>

using std::setprecision;
using std::endl;

namespace yasim {

const float pi=3.14159;

static inline float sqr(float a) { return a * a; }

Rotor::Rotor()
{
    int i;
    _alpha0=-.05;
    _alpha0factor=1;
    _alphamin=-.1;
    _alphamax= .1;
    _alphaoutput[0][0]=0;
    _alphaoutput[1][0]=0;
    _alphaoutput[2][0]=0;
    _alphaoutput[3][0]=0;
    _alphaoutput[4][0]=0;
    _alphaoutput[5][0]=0;
    _alphaoutput[6][0]=0;
    _alphaoutput[7][0]=0;
    _base[0] = _base[1] = _base[2] = 0;
    _ccw=0;
    _delta=1;
    _delta3=0;
    _diameter =10;
    _dynamic=1;
    _engineon=0;
    _force_at_pitch_a=0;
    _forward[0]=1;
    _forward[1]=_forward[2]=0;
    _max_pitch=13./180*pi;
    _maxcyclicail=10./180*pi;
    _maxcyclicele=10./180*pi;
    _maxteeterdamp=0;
    _mincyclicail=-10./180*pi;
    _mincyclicele=-10./180*pi;
    _min_pitch=-.5/180*pi;
    _cyclicele=0;
    _cyclicail=0;
    _name[0] = 0;
    _normal[0] = _normal[1] = 0;
    _normal[2] = 1;
    _normal_with_yaw_roll[0]= _normal_with_yaw_roll[1]=0;
    _normal_with_yaw_roll[2]=1;
    _number_of_blades=4;
    _omega=_omegan=_omegarel=_omegarelneu=0;
    _phi_null=0;
    _ddt_omega=0;
    _pitch_a=0;
    _pitch_b=0;
    _power_at_pitch_0=0;
    _power_at_pitch_b=0;
    _no_torque=0;
    _rel_blade_center=.7;
    _rel_len_hinge=0.01;
    _shared_flap_hinge=false;
    _rellenteeterhinge=0.01;
    _rotor_rpm=442;
    _sim_blades=0;
    _teeterdamp=0.00001;
    _translift=0.05;
    _weight_per_blade=42;
    _translift_ve=20;
    _translift_maxfactor=1.3;
    _ground_effect_constant=0.1;
    _vortex_state_lift_factor=0.4;
    _vortex_state_c1=0.1;
    _vortex_state_c2=0;
    _vortex_state_c3=0;
    _vortex_state_e1=1;
    _vortex_state_e2=1;
    _vortex_state_e3=1;
    _vortex_state=0;
    _lift_factor=1;
    _liftcoef=0.1;
    _dragcoef0=0.1;
    _dragcoef1=0.1;
    _twist=0; 
    _number_of_segments=1;
    _number_of_parts=4;
    _rel_len_where_incidence_is_measured=0.7;
    _torque_of_inertia=1;
    _torque=0;
    _chord=0.3;
    _taper=1;
    _airfoil_incidence_no_lift=0;
    _rel_len_blade_start=0;
    _airfoil_lift_coefficient=0;
    _airfoil_drag_coefficient0=0;
    _airfoil_drag_coefficient1=0;
    for(i=0; i<2; i++)
        _global_ground[i] = _tilt_center[i] = 0;
    _global_ground[2] = 1;
    _global_ground[3] = -1e3;
    _incidence_stall_zero_speed=18*pi/180.;
    _incidence_stall_half_sonic_speed=14*pi/180.;
    _lift_factor_stall=0.28;
    _stall_change_over=2*pi/180.;
    _drag_factor_stall=8;
    _stall_sum=1;
    _stall_v2sum=1;
    _collective=0;
    _yaw=_roll=0;
    for (int k=0;k<4;k++)
        for (i=0;i<3;i++)
            _groundeffectpos[k][i]=0;
    _ground_effect_altitude=1;
    _cyclic_factor=1;
    _lift_factor=_f_ge=_f_vs=_f_tl=1;
    _rotor_correction_factor=.65;
    _balance1=1;
    _balance2=1;
    _properties_tied=0;
    _num_ground_contact_pos=0;
    _directions_and_postions_dirty=true;
    _tilt_yaw=0;
    _tilt_roll=0;
    _tilt_pitch=0;
    _old_tilt_roll=0;
    _old_tilt_pitch=0;
    _old_tilt_yaw=0;
    _min_tilt_yaw=0;
    _min_tilt_pitch=0;
    _min_tilt_roll=0;
    _max_tilt_yaw=0;
    _max_tilt_pitch=0;
    _max_tilt_roll=0;
    _downwash_factor=1;
}

Rotor::~Rotor()
{
    int i;
    for(i=0; i<_rotorparts.size(); i++) {
        Rotorpart* r = (Rotorpart*)_rotorparts.get(i);
        delete r;
    }
    //untie the properties
    if(_properties_tied)
    {
        SGPropertyNode * node = fgGetNode("/rotors", true)->getNode(_name,true);
        node->untie("balance-ext");
        node->untie("balance-int");
        _properties_tied=0;
    }
}

void Rotor::inititeration(float dt,float omegarel,float ddt_omegarel,float *rot)
{
    _stall_sum=0;
    _stall_v2sum=0;
    _omegarel=omegarel;
    _omega=_omegan*_omegarel; 
    _ddt_omega=_omegan*ddt_omegarel;
    int i;
    float drot[3];
    updateDirectionsAndPositions(drot);
    Math::add3(rot,drot,drot);
    for(i=0; i<_rotorparts.size(); i++) {
        float s = Math::sin(float(2*pi*i/_number_of_parts+(_phi-pi/2.)*(_ccw?1:-1)));
        float c = Math::cos(float(2*pi*i/_number_of_parts+(_phi-pi/2.)*(_ccw?1:-1)));
        Rotorpart* r = (Rotorpart*)_rotorparts.get(i);
        r->setOmega(_omega);
        r->setDdtOmega(_ddt_omega);
        r->inititeration(dt,drot);
        r->setCyclic(_cyclicail*c+_cyclicele*s);
    }

    //calculate the normal of the rotor disc, for calcualtion of the downwash
    float side[3],help[3];
    Math::cross3(_normal,_forward,side);
    Math::mul3(Math::cos(_yaw)*Math::cos(_roll),_normal,_normal_with_yaw_roll);

    Math::mul3(Math::sin(_yaw),_forward,help);
    Math::add3(_normal_with_yaw_roll,help,_normal_with_yaw_roll);

    Math::mul3(Math::sin(_roll),side,help);
    Math::add3(_normal_with_yaw_roll,help,_normal_with_yaw_roll);

    //update balance
    if ((_balance1*_balance2 < 0.97) && (_balance1>-1))
    {
        _balance1-=(0.97-_balance1*_balance2)*(0.97-_balance1*_balance2)*0.005;
        if (_balance1<-1) _balance1=-1;
    }
}

float Rotor::calcStall(float incidence,float speed)
{
    float stall_incidence=_incidence_stall_zero_speed
        +(_incidence_stall_half_sonic_speed
        -_incidence_stall_zero_speed)*speed/(343./2);
    //missing: Temeperature dependency of sonic speed
    incidence = Math::abs(incidence);
    if (incidence > (90./180.*pi))
        incidence = pi-incidence;
    float stall = (incidence-stall_incidence)/_stall_change_over;
    stall = Math::clamp(stall,0,1);

    _stall_sum+=stall*speed*speed;
    _stall_v2sum+=speed*speed;

    return stall;
}

float Rotor::getLiftCoef(float incidence,float speed)
{
    float stall=calcStall(incidence,speed);
    /* the next shold look like this, but this is the inner loop of
           the rotor simulation. For small angles (and we hav only small
           angles) the first order approximation works well
    float c1=  Math::sin(incidence-_airfoil_incidence_no_lift)*_liftcoef;
    for c2 we would need higher order, because in stall the angle can be large
    */
    float i2;
    if (incidence > (pi/2))
        i2 = incidence-pi;
    else if (incidence <-(pi/2))
        i2 = (incidence+pi);
    else 
        i2 = incidence;
    float c1=  (i2-_airfoil_incidence_no_lift)*_liftcoef;
    if (stall > 0)
    {
    float c2=  Math::sin(2*(incidence-_airfoil_incidence_no_lift))
        *_liftcoef*_lift_factor_stall;
    return (1-stall)*c1 + stall *c2;
    }
    else
        return c1;
}

float Rotor::getDragCoef(float incidence,float speed)
{
    float stall=calcStall(incidence,speed);
    float c1= (Math::abs(Math::sin(incidence-_airfoil_incidence_no_lift))
        *_dragcoef1+_dragcoef0);
    float c2= c1*_drag_factor_stall;
    return (1-stall)*c1 + stall *c2;
}

int Rotor::getValueforFGSet(int j,char *text,float *f)
{
    if (_name[0]==0) return 0;
    if (4>numRotorparts()) return 0; //compile first!
    if (j==0)
    {
        sprintf(text,"/rotors/%s/cone-deg", _name);
        *f=(_balance1>-1)?( ((Rotorpart*)getRotorpart(0))->getrealAlpha()
            +((Rotorpart*)getRotorpart(1*(_number_of_parts>>2)))->getrealAlpha()
            +((Rotorpart*)getRotorpart(2*(_number_of_parts>>2)))->getrealAlpha()
            +((Rotorpart*)getRotorpart(3*(_number_of_parts>>2)))->getrealAlpha()
            )/4*180/pi:0;
    }
    else
        if (j==1)
        {
            sprintf(text,"/rotors/%s/roll-deg", _name);
            _roll = ( ((Rotorpart*)getRotorpart(0))->getrealAlpha()
                -((Rotorpart*)getRotorpart(2*(_number_of_parts>>2)))->getrealAlpha()
                )/2*(_ccw?-1:1);
            *f=(_balance1>-1)?_roll *180/pi:0;
        }
        else
            if (j==2)
            {
                sprintf(text,"/rotors/%s/yaw-deg", _name);
                _yaw=( ((Rotorpart*)getRotorpart(1*(_number_of_parts>>2)))->getrealAlpha()
                    -((Rotorpart*)getRotorpart(3*(_number_of_parts>>2)))->getrealAlpha()
                    )/2;
                *f=(_balance1>-1)?_yaw*180/pi:0;
            }
            else
                if (j==3)
                {
                    sprintf(text,"/rotors/%s/rpm", _name);
                    *f=(_balance1>-1)?_omega/2/pi*60:0;
                }
                else
                    if (j==4)
                    {
                        sprintf(text,"/rotors/%s/tilt/pitch-deg",_name);
                        *f=_tilt_pitch*180/pi;
                    }
                    else if (j==5)
                    {
                        sprintf(text,"/rotors/%s/tilt/roll-deg",_name);
                        *f=_tilt_roll*180/pi;
                    }
                    else if (j==6)
                    {
                        sprintf(text,"/rotors/%s/tilt/yaw-deg",_name);
                        *f=_tilt_yaw*180/pi;
                    }
                    else if (j==7)
                    {
                        sprintf(text,"/rotors/%s/balance", _name);
                        *f=_balance1;
                    }
                    else if (j==8)
                    {
                        sprintf(text,"/rotors/%s/stall",_name);
                        *f=getOverallStall();
                    }
                    else if (j==9)
                    {
                        sprintf(text,"/rotors/%s/torque",_name);
                        *f=-_torque;;
                    }
                    else
                    {
                        int b=(j-10)/3; 
                        if (b>=_number_of_blades) 
                        {
                            return 0;
                        }
                        int w=j%3;
                        sprintf(text,"/rotors/%s/blade[%i]/%s",
                            _name,b,
                            w==0?"position-deg":(w==1?"flap-deg":"incidence-deg"));
                        *f=((Rotorpart*)getRotorpart(0))->getPhi()*180/pi
                            +360*b/_number_of_blades*(_ccw?1:-1);
                        if (*f>360) *f-=360;
                        if (*f<0) *f+=360;
                        if (_balance1<=-1) *f=0;
                        int k,l;
                        float rk,rl,p;
                        p=(*f/90);
                        k=int(p);
                        l=k+1;
                        rk=l-p;
                        rk=Math::clamp(rk,0,1);//Delete this
                        rl=1-rk;
                        if(w==2) {k+=2;l+=2;}
                        else
                            if(w==1) {k+=1;l+=1;}
                            k%=4;
                            l%=4;
                            if (w==1) *f=rk*((Rotorpart*) getRotorpart(k*(_number_of_parts>>2)))->getrealAlpha()*180/pi
                                +rl*((Rotorpart*) getRotorpart(l*(_number_of_parts>>2)))->getrealAlpha()*180/pi;
                            else if(w==2) *f=rk*((Rotorpart*)getRotorpart(k*(_number_of_parts>>2)))->getIncidence()*180/pi
                                +rl*((Rotorpart*)getRotorpart(l*(_number_of_parts>>2)))->getIncidence()*180/pi;
                    }
                    return j+1;
}

void Rotorgear::setEngineOn(int value)
{
    _engineon=value;
}

void Rotorgear::setRotorEngineMaxRelTorque(float lval)
{
    _max_rel_torque=lval;
}

void Rotorgear::setRotorRelTarget(float lval)
{
    _target_rel_rpm=lval;
}

void Rotor::setAlpha0(float f)
{
    _alpha0=f;
}

void Rotor::setAlphamin(float f)
{
    _alphamin=f;
}

void Rotor::setAlphamax(float f)
{
    _alphamax=f;
}

void Rotor::setAlpha0factor(float f)
{
    _alpha0factor=f;
}

int Rotor::numRotorparts()
{
    return _rotorparts.size();
}

Rotorpart* Rotor::getRotorpart(int n)
{
    return ((Rotorpart*)_rotorparts.get(n));
}

int Rotorgear::getEngineon()
{
    return _engineon;
}

float Rotor::getTorqueOfInertia()
{
    return _torque_of_inertia;
}

void Rotor::setTorque(float f)
{
    _torque=f;
}

void Rotor::addTorque(float f)
{
    _torque+=f;
}

void Rotor::strncpy(char *dest,const char *src,int maxlen)
{
    int n=0;
    while(src[n]&&n<(maxlen-1))
    {
        dest[n]=src[n];
        n++;
    }
    dest[n]=0;
}

void Rotor::setNormal(float* normal)
{
    int i;
    float invsum,sqrsum=0;
    for(i=0; i<3; i++) { sqrsum+=normal[i]*normal[i];}
    if (sqrsum!=0)
        invsum=1/Math::sqrt(sqrsum);
    else
        invsum=1;
    for(i=0; i<3; i++) 
    {
        _normal_with_yaw_roll[i]=_normal[i] = normal[i]*invsum;
    }
}

void Rotor::setForward(float* forward)
{
    int i;
    float invsum,sqrsum=0;
    for(i=0; i<3; i++) { sqrsum+=forward[i]*forward[i];}
    if (sqrsum!=0)
        invsum=1/Math::sqrt(sqrsum);
    else
        invsum=1;
    for(i=0; i<3; i++) { _forward[i] = forward[i]*invsum; }
}

void Rotor::setForceAtPitchA(float force)
{
    _force_at_pitch_a=force; 
}

void Rotor::setPowerAtPitch0(float value)
{
    _power_at_pitch_0=value; 
}

void Rotor::setPowerAtPitchB(float value)
{
    _power_at_pitch_b=value; 
}

void Rotor::setPitchA(float value)
{
    _pitch_a=value/180*pi; 
}

void Rotor::setPitchB(float value)
{
    _pitch_b=value/180*pi; 
}

void Rotor::setBase(float* base)
{
    int i;
    for(i=0; i<3; i++) _base[i] = base[i];
}

void Rotor::setMaxCyclicail(float value)
{
    _maxcyclicail=value/180*pi;
}

void Rotor::setMaxCyclicele(float value)
{
    _maxcyclicele=value/180*pi;
}

void Rotor::setMinCyclicail(float value)
{
    _mincyclicail=value/180*pi;
}

void Rotor::setMinCyclicele(float value)
{
    _mincyclicele=value/180*pi;
}

void Rotor::setMinCollective(float value)
{
    _min_pitch=value/180*pi;
}

void Rotor::setMinTiltYaw(float value)
{
    _min_tilt_yaw=value/180*pi;
}

void Rotor::setMinTiltPitch(float value)
{
    _min_tilt_pitch=value/180*pi;
}

void Rotor::setMinTiltRoll(float value)
{
    _min_tilt_roll=value/180*pi;
}

void Rotor::setMaxTiltYaw(float value)
{
    _max_tilt_yaw=value/180*pi;
}

void Rotor::setMaxTiltPitch(float value)
{
    _max_tilt_pitch=value/180*pi;
}

void Rotor::setMaxTiltRoll(float value)
{
    _max_tilt_roll=value/180*pi;
}

void Rotor::setTiltCenterX(float value)
{
    _tilt_center[0] = value;
}

void Rotor::setTiltCenterY(float value)
{
    _tilt_center[1] = value;
}

void Rotor::setTiltCenterZ(float value)
{
    _tilt_center[2] = value;
}

void Rotor::setMaxCollective(float value)
{
    _max_pitch=value/180*pi;
}

void Rotor::setDiameter(float value)
{
    _diameter=value;
}

void Rotor::setWeightPerBlade(float value)
{
    _weight_per_blade=value;
}

void Rotor::setNumberOfBlades(float value)
{
    _number_of_blades=int(value+.5);
}

void Rotor::setRelBladeCenter(float value)
{
    _rel_blade_center=value;
}

void Rotor::setDynamic(float value)
{
    _dynamic=value;
}

void Rotor::setDelta3(float value)
{
    _delta3=value;
}

void Rotor::setDelta(float value)
{
    _delta=value;
}

void Rotor::setTranslift(float value)
{
    _translift=value;
}

void Rotor::setSharedFlapHinge(bool s)
{
    _shared_flap_hinge=s;
}

void Rotor::setBalance(float b)
{
    _balance1=b;
}

void Rotor::setC2(float value)
{
    _c2=value;
}

void Rotor::setStepspersecond(float steps)
{
    _stepspersecond=steps;
}

void Rotor::setRPM(float value)
{
    _rotor_rpm=value;
}

void Rotor::setPhiNull(float value)
{
    _phi_null=value;
}

void Rotor::setRelLenHinge(float value)
{
    _rel_len_hinge=value;
}

void Rotor::setDownwashFactor(float value)
{
    _downwash_factor=value;
}

void Rotor::setAlphaoutput(int i, const char *text)
{
    strncpy(_alphaoutput[i],text,255);
}

void Rotor::setName(const char *text)
{
    strncpy(_name,text,256);//256: some space needed for settings
}

void Rotor::setCcw(int ccw)
{	
    _ccw=ccw;
}

void Rotor::setNotorque(int value)
{
    _no_torque=value;
}

void Rotor::setRelLenTeeterHinge(float f)
{
    _rellenteeterhinge=f;
}

void Rotor::setTeeterdamp(float f)
{
    _teeterdamp=f;
}

void Rotor::setMaxteeterdamp(float f)
{
    _maxteeterdamp=f;
}

void Rotor::setGlobalGround(double *global_ground, float* global_vel)
{
    int i;
    for(i=0; i<4; i++) _global_ground[i] = global_ground[i];
}

void Rotor::setParameter(const char *parametername, float value)
{
#define p(a,b) if (strcmp(parametername,#a)==0) _##a = (value * (b)); else
    p(translift_ve,1)
        p(translift_maxfactor,1)
        p(ground_effect_constant,1)
        p(vortex_state_lift_factor,1)
        p(vortex_state_c1,1)
        p(vortex_state_c2,1)
        p(vortex_state_c3,1)
        p(vortex_state_e1,1)
        p(vortex_state_e2,1)
        p(vortex_state_e3,1)
        p(twist,pi/180.)
        p(number_of_segments,1)
        p(number_of_parts,1)
        p(rel_len_where_incidence_is_measured,1)
        p(chord,1)
        p(taper,1)
        p(airfoil_incidence_no_lift,pi/180.)
        p(rel_len_blade_start,1)
        p(incidence_stall_zero_speed,pi/180.)
        p(incidence_stall_half_sonic_speed,pi/180.)
        p(lift_factor_stall,1)
        p(stall_change_over,pi/180.)
        p(drag_factor_stall,1)
        p(airfoil_lift_coefficient,1)
        p(airfoil_drag_coefficient0,1)
        p(airfoil_drag_coefficient1,1)
        p(cyclic_factor,1)
        p(rotor_correction_factor,1)
        SG_LOG(SG_INPUT, SG_ALERT,
            "internal error in parameter set up for rotor: '" << 
               parametername <<"'" << std::endl);
#undef p
}

float Rotor::getLiftFactor()
{
    return _lift_factor;
}

void Rotorgear::setRotorBrake(float lval)
{
    lval = Math::clamp(lval, 0, 1);
    _rotorbrake=lval;
}

void Rotor::setTiltYaw(float lval)
{
    lval = Math::clamp(lval, -1, 1);
    _tilt_yaw = _min_tilt_yaw+(lval+1)/2*(_max_tilt_yaw-_min_tilt_yaw);
    _directions_and_postions_dirty = true;
}

void Rotor::setTiltPitch(float lval)
{
    lval = Math::clamp(lval, -1, 1);
    _tilt_pitch = _min_tilt_pitch+(lval+1)/2*(_max_tilt_pitch-_min_tilt_pitch);
    _directions_and_postions_dirty = true;
}

void Rotor::setTiltRoll(float lval)
{
    lval = Math::clamp(lval, -1, 1);
    _tilt_roll = _min_tilt_roll+(lval+1)/2*(_max_tilt_roll-_min_tilt_roll);
    _directions_and_postions_dirty = true;
}

void Rotor::setCollective(float lval)
{
    lval = Math::clamp(lval, -1, 1);
    int i;
    _collective=_min_pitch+(lval+1)/2*(_max_pitch-_min_pitch);
    for(i=0; i<_rotorparts.size(); i++) {
        ((Rotorpart*)_rotorparts.get(i))->setCollective(_collective);
    }
}

void Rotor::setCyclicele(float lval,float rval)
{
    lval = Math::clamp(lval, -1, 1);
    _cyclicele=_mincyclicele+(lval+1)/2*(_maxcyclicele-_mincyclicele);
}

void Rotor::setCyclicail(float lval,float rval)
{
    lval = Math::clamp(lval, -1, 1);
    if (_ccw) lval *=-1;
    _cyclicail=-(_mincyclicail+(lval+1)/2*(_maxcyclicail-_mincyclicail));
}

void Rotor::setRotorBalance(float lval)
{
    lval = Math::clamp(lval, -1, 1);
    _balance2 = lval;
}

void Rotor::getPosition(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _base[i];
}

void Rotor::calcLiftFactor(float* v, float rho, State *s)
{
    //calculates _lift_factor, which is a foactor for the lift of the rotor
    //due to
    //- ground effect (_f_ge)
    //- vortex state (_f_vs)
    //- translational lift (_f_tl)
    _f_ge=1;
    _f_tl=1;
    _f_vs=1;

    // Calculate ground effect
    _f_ge=1+_diameter/_ground_effect_altitude*_ground_effect_constant;

    // Now calculate translational lift
    // float v_vert = Math::dot3(v,_normal);
    float help[3];
    Math::cross3(v,_normal,help);
    float v_horiz = Math::mag3(help);
    _f_tl = ((1-Math::pow(2.7183,-v_horiz/_translift_ve))
        *(_translift_maxfactor-1)+1)/_translift_maxfactor;

    _lift_factor = _f_ge*_f_tl*_f_vs;

    //store the gravity direction
    Glue::geodUp(s->pos, _grav_direction);
    s->velGlobalToLocal(_grav_direction, _grav_direction);
}

void Rotor::findGroundEffectAltitude(Ground * ground_cb,State *s)
{
    _ground_effect_altitude=findGroundEffectAltitude(ground_cb,s,
        _groundeffectpos[0],_groundeffectpos[1],
        _groundeffectpos[2],_groundeffectpos[3]);
    testForRotorGroundContact(ground_cb,s);
}

void Rotor::testForRotorGroundContact(Ground * ground_cb,State *s)
{
    int i;
    for (i=0;i<_num_ground_contact_pos;i++)
    {
        double pt[3],h;
        s->posLocalToGlobal(_ground_contact_pos[i], pt);

        // Ask for the ground plane in the global coordinate system
        double global_ground[4];
        float global_vel[3];
        ground_cb->getGroundPlane(pt, global_ground, global_vel);
        // find h, the distance to the ground 
        // The ground plane transformed to the local frame.
        float ground[4];
        s->planeGlobalToLocal(global_ground, ground);

        h = ground[3] - Math::dot3(_ground_contact_pos[i], ground);
        // Now h is the distance from _ground_contact_pos[i] to ground
        if (h<0)
        {
            _balance1 -= (-h)/_diameter/_num_ground_contact_pos;
            _balance1 = (_balance1<-1)?-1:_balance1;
        }
    }
}
float Rotor::findGroundEffectAltitude(Ground * ground_cb,State *s,
        float *pos0,float *pos1,float *pos2,float *pos3,
        int iteration,float a0,float a1,float a2,float a3)
{
    float a[5];
    float *p[5],pos4[3];
    a[0]=a0;
    a[1]=a1;
    a[2]=a2;
    a[3]=a3;
    a[4]=-1;
    p[0]=pos0;
    p[1]=pos1;
    p[2]=pos2;
    p[3]=pos3;
    p[4]=pos4;
    Math::add3(p[0],p[2],p[4]);
    Math::mul3(0.5,p[4],p[4]);//the center
    
    float mina=100*_diameter;
    float suma=0;
    for (int i=0;i<5;i++)
    {
        if (a[i]==-1)//in the first iteration,(iteration==0) no height is
                     //passed to this function, these missing values are 
                     //marked by ==-1
        {
            double pt[3];
            s->posLocalToGlobal(p[i], pt);

            // Ask for the ground plane in the global coordinate system
            double global_ground[4];
            float global_vel[3];
            ground_cb->getGroundPlane(pt, global_ground, global_vel);
            // find h, the distance to the ground 
            // The ground plane transformed to the local frame.
            float ground[4];
            s->planeGlobalToLocal(global_ground, ground);

            a[i] = ground[3] - Math::dot3(p[i], ground);
            // Now a[i] is the distance from p[i] to ground
        }
        suma+=a[i];
        if (a[i]<mina)
            mina=a[i];
    }
    if (mina>=10*_diameter)
        return mina; //the ground effect will be zero
    
    //check if further recursion is neccessary
    //if the height does not differ more than 20%, than 
    //we can return then mean height, if not split
    //zhe square to four parts and calcualte the height
    //for each part
    //suma * 0.2 is the mean 
    //0.15 is the maximum allowed difference from the mean
    //to the height at the center
    if ((iteration>2)
       ||(Math::abs(suma*0.2-a[4])<(0.15*0.2*suma*(1<<iteration))))
        return suma*0.2;
    suma=0;
    float pc[4][3],ac[4]; //pc[i]=center of pos[i] and pos[(i+1)&3] 
    for (int i=0;i<4;i++)
    {
        Math::add3(p[i],p[(i+1)&3],pc[i]);
        Math::mul3(0.5,pc[i],pc[i]);
        double pt[3];
        s->posLocalToGlobal(pc[i], pt);

        // Ask for the ground plane in the global coordinate system
        double global_ground[4];
        float global_vel[3];
        ground_cb->getGroundPlane(pt, global_ground, global_vel);
        // find h, the distance to the ground 
        // The ground plane transformed to the local frame.
        float ground[4];
        s->planeGlobalToLocal(global_ground, ground);

        ac[i] = ground[3] - Math::dot3(p[i], ground);
        // Now ac[i] is the distance from pc[i] to ground
    }
    return 0.25*
        (findGroundEffectAltitude(ground_cb,s,p[0],pc[1],p[4],pc[3],
            iteration+1,a[0],ac[0],a[4],ac[3])
        +findGroundEffectAltitude(ground_cb,s,p[1],pc[0],p[4],pc[1],
            iteration+1,a[1],ac[0],a[4],ac[1])
        +findGroundEffectAltitude(ground_cb,s,p[2],pc[1],p[4],pc[2],
            iteration+1,a[2],ac[1],a[4],ac[2])
        +findGroundEffectAltitude(ground_cb,s,p[3],pc[2],p[4],pc[3],
            iteration+1,a[3],ac[2],a[4],ac[3])
        );
}

void Rotor::getDownWash(float *pos, float *v_heli, float *downwash)
{
    float pos2rotor[3],tmp[3];
    Math::sub3(_base,pos,pos2rotor);
    float dist=Math::dot3(pos2rotor,_normal_with_yaw_roll);
    //calculate incidence at 0.7r;
    float inc = _collective+_twist *0.7 
                - _twist*_rel_len_where_incidence_is_measured;
    if (inc < 0) 
        dist *=-1;
    if (dist<0) // we are not in the downwash region
    {
        downwash[0]=downwash[1]=downwash[2]=0.;
        return;
    }

    //calculate the mean downwash speed directly beneath the rotor disk
    float v1bar = Math::sin(inc) *_omega * 0.35 * _diameter * 0.8; 
    //0.35 * d = 0.7 *r, a good position to calcualte the mean downwashd
    //0.8 the slip of the rotor.

    //calculate the time the wind needed from thr rotor to here
    if (v1bar< 1) v1bar = 1;
    float time=dist/v1bar;

    //calculate the pos2rotor, where the rotor was, "time" ago
    Math::mul3(time,v_heli,tmp);
    Math::sub3(pos2rotor,tmp,pos2rotor);

    //and again calculate dist
    dist=Math::dot3(pos2rotor,_normal_with_yaw_roll);
    //missing the normal is offen not pointing to the normal of the rotor 
    //disk. Rotate the normal by yaw and tilt angle

    if (inc < 0) 
        dist *=-1;
    if (dist<0) // we are not in the downwash region
    {
        downwash[0]=downwash[1]=downwash[2]=0.;
        return;
    }
    //of course this could be done in a runge kutta integrator, but it's such
    //a approximation that I beleave, it would'nt be more realistic

    //calculate the dist to the rotor-axis
    Math::cross3(pos2rotor,_normal_with_yaw_roll,tmp);
    float r= Math::mag3(tmp);
    //calculate incidence at r;
    float rel_r = r *2 /_diameter;
    float inc_r = _collective+_twist * r /_diameter * 2 
        - _twist*_rel_len_where_incidence_is_measured;

    //calculate the downwash speed directly beneath the rotor disk
    float v1=0;
    if (rel_r<1)
        v1 = Math::sin(inc_r) *_omega * r * 0.8; 

    //calcualte the downwash speed in a distance "dist" to the rotor disc,
    //for large dist. The speed is assumed do follow a gausian distribution 
    //with sigma increasing with dist^2:
    //sigma is assumed to be half of the rotor diameter directly beneath the
    //disc and is assumed to the rotor diameter at dist = (diameter * sqrt(2))

    float sigma=_diameter/2 + dist * dist / _diameter /4.;
    float v2 = v1bar*_diameter/ (Math::sqrt(2 * pi) * sigma) 
        * Math::pow(2.7183,-.5*r*r/(sigma*sigma))*_diameter/2/sigma;

    //calculate the weight of the two downwash velocities.
    //Directly beneath the disc it is v1, far away it is v2
    float g = Math::pow(2.7183,-2*dist/_diameter); 
    //at dist = rotor radius it is assumed to be 1/e * v1 + (1-1/e)* v2

    float v = g * v1 + (1-g) * v2;
    Math::mul3(-v*_downwash_factor,_normal_with_yaw_roll,downwash);
    //the downwash is calculated in the opposite direction of the normal
}

void Rotor::euler2orient(float roll, float pitch, float hdg, float* out)
{
    // the Glue::euler2orient, inverts y<z due to different bases
    // therefore the negation of all "y" and "z" coeffizients
    Glue::euler2orient(roll,pitch,hdg,out);
    for (int i=3;i<9;i++) out[i]*=-1.0;
}


void Rotor::updateDirectionsAndPositions(float *rot)
{
    if (!_directions_and_postions_dirty)
    {
        rot[0]=rot[1]=rot[2]=0;
        return;
    }
    rot[0]=_old_tilt_roll-_tilt_roll;
    rot[1]=_old_tilt_pitch-_tilt_pitch;
    rot[2]=_old_tilt_yaw-_tilt_yaw;
    _old_tilt_roll=_tilt_roll;
    _old_tilt_pitch=_tilt_pitch;
    _old_tilt_yaw=_tilt_yaw;
    float orient[9];
    euler2orient(_tilt_roll, _tilt_pitch, _tilt_yaw, orient);
    float forward[3];
    float normal[3];
    float base[3];
    Math::sub3(_base,_tilt_center,base);
    Math::vmul33(orient, base, base);
    Math::add3(base,_tilt_center,base);
    Math::vmul33(orient, _forward, forward);
    Math::vmul33(orient, _normal, normal);
#define _base base
#define _forward forward
#define _normal normal
    float directions[5][3];
    //pointing forward, right, ... the 5th is ony for calculation
    directions[0][0]=_forward[0];
    directions[0][1]=_forward[1];
    directions[0][2]=_forward[2];
    int i;
    for (i=1;i<5;i++)
    {
        if (!_ccw)
            Math::cross3(directions[i-1],_normal,directions[i]);
        else
            Math::cross3(_normal,directions[i-1],directions[i]);
    }
    Math::set3(directions[4],directions[0]);
    // now directions[0] is perpendicular to the _normal.and has a length
    // of 1. if _forward is already normalized and perpendicular to the 
    // normal, directions[0] will be the same
    //_num_ground_contact_pos=(_number_of_parts<16)?_number_of_parts:16;
    for (i=0;i<_num_ground_contact_pos;i++)
    {
        float help[3];
        float s = Math::sin(pi*2*i/_num_ground_contact_pos);
        float c = Math::cos(pi*2*i/_num_ground_contact_pos);
        Math::mul3(c*_diameter*0.5,directions[0],_ground_contact_pos[i]);
        Math::mul3(s*_diameter*0.5,directions[1],help);
        Math::add3(help,_ground_contact_pos[i],_ground_contact_pos[i]);
        Math::add3(_base,_ground_contact_pos[i],_ground_contact_pos[i]);
    }
    for (i=0;i<4;i++)
    {
        Math::mul3(_diameter*0.7,directions[i],_groundeffectpos[i]);
        Math::add3(_base,_groundeffectpos[i],_groundeffectpos[i]);
    }
    for (i=0;i<_number_of_parts;i++)
    {
        Rotorpart* rp = getRotorpart(i);
        float lpos[3],lforceattac[3],lspeed[3],dirzentforce[3];
        float s = Math::sin(2*pi*i/_number_of_parts);
        float c = Math::cos(2*pi*i/_number_of_parts);
        float sp = Math::sin(float(2*pi*i/_number_of_parts-pi/2.+_phi));
        float cp = Math::cos(float(2*pi*i/_number_of_parts-pi/2.+_phi));
        float direction[3],nextdirection[3],help[3],direction90deg[3];
        float rotorpartmass = _weight_per_blade*_number_of_blades/_number_of_parts*.453;
        float speed=_rotor_rpm/60*_diameter*_rel_blade_center*pi;
        float lentocenter=_diameter*_rel_blade_center*0.5;
        float lentoforceattac=_diameter*_rel_len_hinge*0.5;
        float zentforce=rotorpartmass*speed*speed/lentocenter;
    
        Math::mul3(c ,directions[0],help);
        Math::mul3(s ,directions[1],direction);
        Math::add3(help,direction,direction);

        Math::mul3(c ,directions[1],help);
        Math::mul3(s ,directions[2],direction90deg);
        Math::add3(help,direction90deg,direction90deg);
        
        Math::mul3(cp ,directions[1],help);
        Math::mul3(sp ,directions[2],nextdirection);
        Math::add3(help,nextdirection,nextdirection);

        Math::mul3(lentocenter,direction,lpos);
        Math::add3(lpos,_base,lpos);
        Math::mul3(lentoforceattac,nextdirection,lforceattac);
        //nextdirection: +90deg (gyro)!!!

        Math::add3(lforceattac,_base,lforceattac);
        Math::mul3(speed,direction90deg,lspeed);
        Math::mul3(1,nextdirection,dirzentforce);
        rp->setPosition(lpos);
        rp->setNormal(_normal);
        rp->setZentipetalForce(zentforce);
        rp->setPositionForceAttac(lforceattac);
        rp->setSpeed(lspeed);
        rp->setDirectionofZentipetalforce(dirzentforce);
        rp->setDirectionofRotorPart(direction);
    }
#undef _base
#undef _forward
#undef _normal
    _directions_and_postions_dirty=false;
}

void Rotor::compile()
{
    // Have we already been compiled?
    if(_rotorparts.size() != 0) return;

    //rotor is divided into _number_of_parts parts
    //each part is calcualted at _number_of_segments points

    //clamp to 4..256
    //and make it a factor of 4
    _number_of_parts=(int(Math::clamp(_number_of_parts,4,256))>>2)<<2;

    _dynamic=_dynamic*(1/                          //inverse of the time
        ( (60/_rotor_rpm)/4         //for rotating 90 deg
        +(60/_rotor_rpm)/(2*_number_of_blades) //+ meantime a rotorblade 
                                               //will pass a given point 
        ));
    //normalize the directions
    Math::unit3(_forward,_forward);
    Math::unit3(_normal,_normal);
    _num_ground_contact_pos=(_number_of_parts<16)?_number_of_parts:16;
    float rotorpartmass = _weight_per_blade*_number_of_blades/_number_of_parts*.453;
    //was pounds -> now kg

    _torque_of_inertia = 1/12. * ( _number_of_parts * rotorpartmass) * _diameter 
        * _diameter * _rel_blade_center * _rel_blade_center /(0.5*0.5);
    float speed=_rotor_rpm/60*_diameter*_rel_blade_center*pi;
    float lentocenter=_diameter*_rel_blade_center*0.5;
    // float lentoforceattac=_diameter*_rel_len_hinge*0.5;
    float zentforce=rotorpartmass*speed*speed/lentocenter;
    float pitchaforce=_force_at_pitch_a/_number_of_parts*.453*9.81;
    // was pounds of force, now N, devided by _number_of_parts
    //(so its now per rotorpart)

    float torque0=0,torquemax=0,torqueb=0;
    float omega=_rotor_rpm/60*2*pi;
    _omegan=omega;
    float omega0=omega*Math::sqrt(1/(1-_rel_len_hinge));
    float delta_theoretical=pitchaforce/(_pitch_a*omega*lentocenter*2*rotorpartmass);
    _delta*=delta_theoretical;

    float relamp=(omega*omega/(2*_delta*Math::sqrt(sqr(omega0*omega0-omega*omega)
        +4*_delta*_delta*omega*omega)))*_cyclic_factor;
    //float relamp_theoretical=(omega*omega/(2*delta_theoretical*Math::sqrt(sqr(omega0*omega0-omega*omega)
    //    +4*delta_theoretical*delta_theoretical*omega*omega)))*_cyclic_factor;
    _phi=Math::acos(_rel_len_hinge);
    _phi-=Math::atan(_delta3);
    if (!_no_torque)
    {
        torque0=_power_at_pitch_0/_number_of_parts*1000/omega;  
        // f*r=p/w ; p=f*s/t;  r=s/t/w ; r*w*t = s
        torqueb=_power_at_pitch_b/_number_of_parts*1000/omega;
        torquemax=_power_at_pitch_b/_number_of_parts*1000/omega/_pitch_b*_max_pitch;

        if(_ccw)
        {
            torque0*=-1;
            torquemax*=-1;
            torqueb*=-1;
        }
    }

    Rotorpart* rps[256];
    int i;
    for (i=0;i<_number_of_parts;i++)
    {
        Rotorpart* rp=rps[i]=newRotorpart(zentforce,pitchaforce,_delta3,rotorpartmass,
            _translift,_rel_len_hinge,lentocenter);
        int k = i*4/_number_of_parts;
        rp->setAlphaoutput(_alphaoutput[k&1?k:(_ccw?k^2:k)],0);
        rp->setAlphaoutput(_alphaoutput[4+(k&1?k:(_ccw?k^2:k))],1+(k>1));
        _rotorparts.add(rp);
        rp->setTorque(torquemax,torque0);
        rp->setRelamp(relamp);
        rp->setTorqueOfInertia(_torque_of_inertia/_number_of_parts);
        rp->setDirection(2*pi*i/_number_of_parts);
    }
    for (i=0;i<_number_of_parts;i++)
    {
        rps[i]->setlastnextrp(rps[(i-1+_number_of_parts)%_number_of_parts],
            rps[(i+1)%_number_of_parts],
            rps[(i+_number_of_parts/2)%_number_of_parts],
            rps[(i-_number_of_parts/4+_number_of_parts)%_number_of_parts],
            rps[(i+_number_of_parts/4)%_number_of_parts]);
    }
    float drot[3];
    updateDirectionsAndPositions(drot);
    for (i=0;i<_number_of_parts;i++)
    {
        rps[i]->setCompiled();
    }
    float lift[4],torque[4], v_wind[3];
    v_wind[0]=v_wind[1]=v_wind[2]=0;
    rps[0]->setOmega(_omegan);

    if (_airfoil_lift_coefficient==0)
    {
        //calculate the lift and drag coefficients now
        _dragcoef0=1;
        _dragcoef1=1;
        _liftcoef=1;
        rps[0]->calculateAlpha(v_wind,rho_null,_pitch_a,0,0,
            &(torque[0]),&(lift[0])); //max_pitch a
        _liftcoef = pitchaforce/lift[0];
        _dragcoef0=1;
        _dragcoef1=0;
        rps[0]->calculateAlpha(v_wind,rho_null,0,0,0,&(torque[0]),&(lift[0])); 
        //0 degree, c0

        _dragcoef0=0;
        _dragcoef1=1;
        rps[0]->calculateAlpha(v_wind,rho_null,0,0,0,&(torque[1]),&(lift[1]));
        //0 degree, c1

        _dragcoef0=1;
        _dragcoef1=0;
        rps[0]->calculateAlpha(v_wind,rho_null,_pitch_b,0,0,&(torque[2]),&(lift[2])); 
        //picth b, c0

        _dragcoef0=0;
        _dragcoef1=1;
        rps[0]->calculateAlpha(v_wind,rho_null,_pitch_b,0,0,&(torque[3]),&(lift[3])); 
        //picth b, c1

        if (torque[0]==0)
        {
            _dragcoef1=torque0/torque[1];
            _dragcoef0=(torqueb-_dragcoef1*torque[3])/torque[2];
        }
        else
        {
            _dragcoef1=(torque0/torque[0]-torqueb/torque[2])
                /(torque[1]/torque[0]-torque[3]/torque[2]);
            _dragcoef0=(torqueb-_dragcoef1*torque[3])/torque[2];
        }
    }
    else
    {
        _liftcoef=_airfoil_lift_coefficient/_number_of_parts*_number_of_blades;
        _dragcoef0=_airfoil_drag_coefficient0/_number_of_parts*_number_of_blades*_c2;
        _dragcoef1=_airfoil_drag_coefficient1/_number_of_parts*_number_of_blades*_c2;
    }

    //Check
    rps[0]->calculateAlpha(v_wind,rho_null,_pitch_a,0,0,
        &(torque[0]),&(lift[0])); //pitch a
    rps[0]->calculateAlpha(v_wind,rho_null,_pitch_b,0,0,
        &(torque[1]),&(lift[1])); //pitch b
    rps[0]->calculateAlpha(v_wind,rho_null,0,0,0,
        &(torque[3]),&(lift[3])); //pitch 0
    SG_LOG(SG_FLIGHT, SG_INFO,
        "Rotor: coefficients for airfoil:" << endl << setprecision(6)
        << " drag0: " << _dragcoef0*_number_of_parts/_number_of_blades/_c2
        << " drag1: " << _dragcoef1*_number_of_parts/_number_of_blades/_c2
        << " lift: " << _liftcoef*_number_of_parts/_number_of_blades
        << endl
        << "at 10 deg:" << endl
        << "drag: " << (Math::sin(10./180*pi)*_dragcoef1+_dragcoef0)
            *_number_of_parts/_number_of_blades/_c2
        << " lift: " << Math::sin(10./180*pi)*_liftcoef*_number_of_parts/_number_of_blades
        << endl
        << "Some results (Pitch [degree], Power [kW], Lift [N])" << endl
        << 0.0f << "deg " << Math::abs(torque[3]*_number_of_parts*_omegan/1000) << "kW "
            << lift[3]*_number_of_parts << endl
        << _pitch_a*180/pi << "deg " << Math::abs(torque[0]*_number_of_parts*_omegan/1000) 
            << "kW " << lift[0]*_number_of_parts << endl
        << _pitch_b*180/pi << "deg " << Math::abs(torque[1]*_number_of_parts*_omegan/1000) 
            << "kW " << lift[1]*_number_of_parts << endl << endl );

    //first calculation of relamp is wrong
    //it used pitchaforce, but this was unknown and
    //on the default value
    _delta*=lift[0]/pitchaforce;
    relamp=(omega*omega/(2*_delta*Math::sqrt(sqr(omega0*omega0-omega*omega)
        +4*_delta*_delta*omega*omega)))*_cyclic_factor;
    for (i=0;i<_number_of_parts;i++)
    {
        rps[i]->setRelamp(relamp);
    }
    rps[0]->setOmega(0);
    setCollective(0);
    setCyclicail(0,0);
    setCyclicele(0,0);

    writeInfo();

    //tie the properties
    /* After reset these values are totally wrong. I have to find out why
    SGPropertyNode * node = fgGetNode("/rotors", true)->getNode(_name,true);
    node->tie("balance_ext",SGRawValuePointer<float>(&_balance2),false);
    node->tie("balance_int",SGRawValuePointer<float>(&_balance1));
    _properties_tied=1;
    */
}
std::ostream &  operator<<(std::ostream & out, Rotor& r)
{
#define i(x) << #x << ":" << r.x << endl
#define iv(x) << #x << ":" << r.x[0] << ";" << r.x[1] << ";" <<r.x[2] << ";" << endl
    out << "Writing Info on Rotor " 
    i(_name)
    i(_torque)
    i(_omega) i(_omegan) i(_omegarel) i(_ddt_omega) i(_omegarelneu)
    i (_chord)
    i( _taper)
    i( _airfoil_incidence_no_lift)
    i( _collective)
    i( _airfoil_lift_coefficient)
    i( _airfoil_drag_coefficient0)
    i( _airfoil_drag_coefficient1)
    i( _ccw)
    i( _number_of_segments)
    i( _number_of_parts)
    iv( _base)
    iv( _groundeffectpos[0])iv( _groundeffectpos[1])iv( _groundeffectpos[2])iv( _groundeffectpos[3])
    i( _ground_effect_altitude)
    iv( _normal)
    iv( _normal_with_yaw_roll)
    iv( _forward)
    i( _diameter)
    i( _number_of_blades)
    i( _weight_per_blade)
    i( _rel_blade_center)
    i( _min_pitch)
    i( _max_pitch)
    i( _force_at_pitch_a)
    i( _pitch_a)
    i( _power_at_pitch_0)
    i( _power_at_pitch_b)
    i( _no_torque)
    i( _sim_blades)
    i( _pitch_b)
    i( _rotor_rpm)
    i( _rel_len_hinge)
    i( _maxcyclicail)
    i( _maxcyclicele)
    i( _mincyclicail)
    i( _mincyclicele)
    i( _delta3)
    i( _delta)
    i( _dynamic)
    i( _translift)
    i( _c2)
    i( _stepspersecond)
    i( _engineon)
    i( _alphamin) i(_alphamax) i(_alpha0) i(_alpha0factor)
    i( _teeterdamp) i(_maxteeterdamp)
    i( _rellenteeterhinge)
    i( _translift_ve)
    i( _translift_maxfactor)
    i( _ground_effect_constant)
    i( _vortex_state_lift_factor)
    i( _vortex_state_c1)
    i( _vortex_state_c2)
    i( _vortex_state_c3)
    i( _vortex_state_e1)
    i( _vortex_state_e2)
    i( _vortex_state_e3)
    i( _lift_factor) i(_f_ge) i(_f_vs) i(_f_tl)
    i( _vortex_state)
    i( _liftcoef)
    i( _dragcoef0)
    i( _dragcoef1)
    i( _twist) //outer incidence = inner inner incidence + _twist
    i( _rel_len_where_incidence_is_measured)
    i( _torque_of_inertia)
    i( _rel_len_blade_start)
    i( _incidence_stall_zero_speed)
    i( _incidence_stall_half_sonic_speed)
    i( _lift_factor_stall)
    i( _stall_change_over)
    i( _drag_factor_stall)
    i( _stall_sum)
    i( _stall_v2sum)
    i( _yaw)
    i( _roll)
    i( _cyclicail)
    i( _cyclicele)
    i( _cyclic_factor) <<endl;
    int j;
    for(j=0; j<r._rotorparts.size(); j++) {
        out << *((Rotorpart*)r._rotorparts.get(j));
    }
    out <<endl << endl;
#undef i
#undef iv
    return out;
}
void Rotor:: writeInfo()
{
#ifdef TEST_DEBUG
    std::ostringstream buffer;
    buffer << *this;
    FILE*f=fopen("c:\\fgmsvc\\bat\\log.txt","at");
    if (!f) f=fopen("c:\\fgmsvc\\bat\\log.txt","wt");
    if (f)
    {
        fprintf(f,"%s",(const char *)buffer.str().c_str());
        fclose (f);
    }
#endif
}
Rotorpart* Rotor::newRotorpart(float zentforce,float maxpitchforce,
    float delta3,float mass,float translift,float rellenhinge,float len)
{
    Rotorpart *r = new Rotorpart();
    r->setDelta3(delta3);
    r->setDynamic(_dynamic);
    r->setTranslift(_translift);
    r->setC2(_c2);
    r->setWeight(mass);
    r->setRelLenHinge(rellenhinge);
    r->setSharedFlapHinge(_shared_flap_hinge);
    r->setOmegaN(_omegan);
    r->setPhi(_phi_null);
    r->setAlpha0(_alpha0);
    r->setAlphamin(_alphamin);
    r->setAlphamax(_alphamax);
    r->setAlpha0factor(_alpha0factor);
    r->setLen(len);
    r->setDiameter(_diameter);
    r->setRotor(this);
#define p(a) r->setParameter(#a,_##a);
    p(twist)
    p(number_of_segments)
    p(rel_len_where_incidence_is_measured)
    p(rel_len_blade_start)
    p(rotor_correction_factor)
#undef p
    return r;
}

void Rotor::interp(float* v1, float* v2, float frac, float* out)
{
    out[0] = v1[0] + frac*(v2[0]-v1[0]);
    out[1] = v1[1] + frac*(v2[1]-v1[1]);
    out[2] = v1[2] + frac*(v2[2]-v1[2]);
}

void Rotorgear::initRotorIteration(float *lrot,float dt)
{
    int i;
    float omegarel;
    if (!_rotors.size()) return;
    Rotor* r0 = (Rotor*)_rotors.get(0);
    omegarel= r0->getOmegaRelNeu();
    for(i=0; i<_rotors.size(); i++) {
        Rotor* r = (Rotor*)_rotors.get(i);
        r->inititeration(dt,omegarel,0,lrot);
    }
}

void Rotorgear::calcForces(float* torqueOut)
{
    int i,j;
    torqueOut[0]=torqueOut[1]=torqueOut[2]=0;
    // check,<if the engine can handle the torque of the rotors. 
    // If not reduce the torque to the fueselage and change rotational 
    // speed of the rotors instead
    if (_rotors.size())
    {
        float omegarel,omegan;
        Rotor* r0 = (Rotor*)_rotors.get(0);
        omegarel= r0->getOmegaRel();

        float total_torque_of_inertia=0;
        float total_torque=0;
        for(i=0; i<_rotors.size(); i++) {
            Rotor* r = (Rotor*)_rotors.get(i);
            omegan=r->getOmegan();
            total_torque_of_inertia+=r->getTorqueOfInertia()*omegan*omegan;
            //FIXME: this is constant, so this can be done in compile

            total_torque+=r->getTorque()*omegan;
        }
        float max_torque_of_engine=0;
        // SGPropertyNode * node=fgGetNode("/rotors/gear", true);
        if (_engineon)
        {
            max_torque_of_engine=_max_power_engine*_max_rel_torque;
            float df=_target_rel_rpm-omegarel;
            df/=_engine_prop_factor;
            df = Math::clamp(df, 0, 1);
            max_torque_of_engine = df * _max_power_engine*_max_rel_torque;
        }
        total_torque*=-1;
        _ddt_omegarel=0;

#if 0
        float rel_torque_engine=1;
        if (total_torque<=0)
            rel_torque_engine=0;
        else
            if (max_torque_of_engine>0)
                rel_torque_engine=1/max_torque_of_engine*total_torque;
            else
                rel_torque_engine=0;
#endif

        //add the rotor brake and the gear fritcion
        float dt=0.1f;
        if (r0->_rotorparts.size()) dt=((Rotorpart*)r0->_rotorparts.get(0))->getDt();

        float rotor_brake_torque;
        rotor_brake_torque=_rotorbrake*_max_power_rotor_brake+_rotorgear_friction;
        //clamp it to the value you need to stop the rotor
        //to avod accelerate the rotor to neagtive rpm:
        rotor_brake_torque=Math::clamp(rotor_brake_torque,0,
            total_torque_of_inertia/dt*omegarel);
        max_torque_of_engine-=rotor_brake_torque;

        //change the rotation of the rotors 
        if ((max_torque_of_engine<total_torque) //decreasing rotation
            ||((max_torque_of_engine>total_torque)&&(omegarel<_target_rel_rpm))
            //increasing rotation due to engine
            ||(total_torque<0) ) //increasing rotation due to autorotation
        {
            _ddt_omegarel=(max_torque_of_engine-total_torque)/total_torque_of_inertia;
            if(max_torque_of_engine>total_torque) 
            {
                //check if the acceleration is due to the engine. If yes,
                //the engine self limits the accel.
                float lim1=-total_torque/total_torque_of_inertia; 
                //accel. by autorotation
                
                if (lim1<_engine_accel_limit) lim1=_engine_accel_limit; 
                //if the accel by autorotation greater than the max. engine
                //accel, then this is the limit, if not: the engine is the limit
                if (_ddt_omegarel>lim1) _ddt_omegarel=lim1;
            }
            if (_ddt_omegarel>5.5)_ddt_omegarel=5.5; 
            //clamp it to avoid overflow. Should never be reached
            if (_ddt_omegarel<-5.5)_ddt_omegarel=-5.5;

            if (_max_power_engine<0.001) {omegarel=1;_ddt_omegarel=0;}
            //for debug: negative or no maxpower will result 
            //in permanet 100% rotation

            omegarel+=dt*_ddt_omegarel;

            if (omegarel>2.5) omegarel=2.5; 
            //clamp it to avoid overflow. Should never be reached
            if (omegarel<-.5) omegarel=-.5;

            r0->setOmegaRelNeu(omegarel);
            //calculate the torque, which is needed to accelerate the rotors.
            //Add this additional torque to the body
            for(j=0; j<_rotors.size(); j++) {
                Rotor* r = (Rotor*)_rotors.get(j);
                for(i=0; i<r->_rotorparts.size(); i++) {
                    // float torque_scalar=0;
                    Rotorpart* rp = (Rotorpart*)r->_rotorparts.get(i);
                    float torque[3];
                    rp->getAccelTorque(_ddt_omegarel,torque);
                    Math::add3(torque,torqueOut,torqueOut);
                }
            }
        }
        _total_torque_on_engine=total_torque+_ddt_omegarel*total_torque_of_inertia;
    }
}

void Rotorgear::addRotor(Rotor* rotor)
{
    _rotors.add(rotor);
    _in_use = 1;
}

void Rotorgear::compile()
{
    // float wgt = 0;
    for(int j=0; j<_rotors.size(); j++) {
        Rotor* r = (Rotor*)_rotors.get(j);
        r->compile();
    }
}

void Rotorgear::getDownWash(float *pos, float * v_heli, float *downwash)
{
    float tmp[3];
    downwash[0]=downwash[1]=downwash[2]=0;
    for(int i=0; i<_rotors.size(); i++) {
        Rotor* ro = (Rotor*)_rotors.get(i);
        ro->getDownWash(pos,v_heli,tmp);
        Math::add3(downwash,tmp,downwash);    //  + downwash
    }
}

void Rotorgear::setParameter(char *parametername, float value)
{
#define p(a,b) if (strcmp(parametername,#a)==0) _##a = (value * (b)); else
        p(max_power_engine,1000)
        p(engine_prop_factor,1)
        p(yasimdragfactor,1)
        p(yasimliftfactor,1)
        p(max_power_rotor_brake,1000)
        p(rotorgear_friction,1000)
        p(engine_accel_limit,0.01)
        SG_LOG(SG_INPUT, SG_ALERT,
            "internal error in parameter set up for rotorgear: '"
            << parametername <<"'" << endl);
#undef p
}
int Rotorgear::getValueforFGSet(int j,char *text,float *f)
{
    if (j==0)
    {
        sprintf(text,"/rotors/gear/total-torque");
        *f=_total_torque_on_engine;
    } else return 0;
    return j+1;
}
Rotorgear::Rotorgear()
{
    _in_use=0;
    _engineon=0;
    _rotorbrake=0;
    _max_power_rotor_brake=1;
    _rotorgear_friction=1;
    _max_power_engine=1000*450;
    _engine_prop_factor=0.05f;
    _yasimdragfactor=1;
    _yasimliftfactor=1;
    _ddt_omegarel=0;
    _engine_accel_limit=0.05f;
    _total_torque_on_engine=0;
    _target_rel_rpm=1;
    _max_rel_torque=1;
}

Rotorgear::~Rotorgear()
{
    for(int i=0; i<_rotors.size(); i++)
        delete (Rotor*)_rotors.get(i);
}

}; // namespace yasim
