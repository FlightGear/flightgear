#include <simgear/debug/logstream.hxx>

#include "Math.hpp"
#include "Surface.hpp"
#include "Rotorpart.hpp"
#include "Rotor.hpp"

#include STL_IOSTREAM
#include STL_IOMANIP

SG_USING_STD(setprecision);

#include <stdio.h>

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
    _name[0] = 0;
    _normal[0] = _normal[1] = 0;
    _normal[2] = 1;
    _normal_with_yaw_roll[0]= _normal_with_yaw_roll[1]=0;
    _normal_with_yaw_roll[2]=1;
    _number_of_blades=4;
    _omega=_omegan=_omegarel=_omegarelneu=0;
    _ddt_omega=0;
    _pitch_a=0;
    _pitch_b=0;
    _power_at_pitch_0=0;
    _power_at_pitch_b=0;
    _no_torque=0;
    _rel_blade_center=.7;
    _rel_len_hinge=0.01;
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
    _lift_factor=1;
    _liftcoef=0.1;
    _dragcoef0=0.1;
    _dragcoef1=0.1;
    _twist=0; 
    _number_of_segments=1;
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
        _global_ground[i] =  0;
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
}

Rotor::~Rotor()
{
    int i;
    for(i=0; i<_rotorparts.size(); i++) {
        Rotorpart* r = (Rotorpart*)_rotorparts.get(i);
        delete r;
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
    for(i=0; i<_rotorparts.size(); i++) {
        Rotorpart* r = (Rotorpart*)_rotorparts.get(i);
        r->setOmega(_omega);
        r->setDdtOmega(_ddt_omega);
        r->inititeration(dt,rot);
    }

    //calculate the normal of the rotor disc, for calcualtion of the downwash
    float side[3],help[3];
    Math::cross3(_normal,_forward,side);
    Math::mul3(Math::cos(_yaw)*Math::cos(_roll),_normal,_normal_with_yaw_roll);

    Math::mul3(Math::sin(_yaw),_forward,help);
    Math::add3(_normal_with_yaw_roll,help,_normal_with_yaw_roll);

    Math::mul3(Math::sin(_roll),side,help);
    Math::add3(_normal_with_yaw_roll,help,_normal_with_yaw_roll);
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
    float c1=  Math::sin(incidence-_airfoil_incidence_no_lift)*_liftcoef;
    float c2=  Math::sin(2*(incidence-_airfoil_incidence_no_lift))
        *_liftcoef*_lift_factor_stall;
    return (1-stall)*c1 + stall *c2;
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
    if (4!=numRotorparts()) return 0; //compile first!
    if (j==0)
    {
        sprintf(text,"/rotors/%s/cone", _name);
        *f=( ((Rotorpart*)getRotorpart(0))->getrealAlpha()
            +((Rotorpart*)getRotorpart(1))->getrealAlpha()
            +((Rotorpart*)getRotorpart(2))->getrealAlpha()
            +((Rotorpart*)getRotorpart(3))->getrealAlpha()
            )/4*180/pi;
    }
    else
        if (j==1)
        {
            sprintf(text,"/rotors/%s/roll", _name);
            _roll = ( ((Rotorpart*)getRotorpart(0))->getrealAlpha()
                -((Rotorpart*)getRotorpart(2))->getrealAlpha()
                )/2*(_ccw?-1:1);
            *f=_roll *180/pi;
        }
        else
            if (j==2)
            {
                sprintf(text,"/rotors/%s/yaw", _name);
                _yaw=( ((Rotorpart*)getRotorpart(1))->getrealAlpha()
                    -((Rotorpart*)getRotorpart(3))->getrealAlpha()
                    )/2;
                *f=_yaw*180/pi;
            }
            else
                if (j==3)
                {
                    sprintf(text,"/rotors/%s/rpm", _name);
                    *f=_omega/2/pi*60;
                }
                else
                    if (j==4)
                    {
                        sprintf(text,"/rotors/%s/debug/debugfge",_name);
                        *f=_f_ge;
                    }
                    else if (j==5)
                    {
                        sprintf(text,"/rotors/%s/debug/debugfvs",_name);
                        *f=_f_vs;
                    }
                    else if (j==6)
                    {
                        sprintf(text,"/rotors/%s/debug/debugftl",_name);
                        *f=_f_tl;
                    }
                    else if (j==7)
                    {
                        sprintf(text,"/rotors/%s/debug/vortexstate",_name);
                        *f=_vortex_state;
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
                        sprintf(text,"/rotors/%s/blade%i_%s",
                            _name,b+1,
                            w==0?"pos":(w==1?"flap":"incidence"));
                        *f=((Rotorpart*)getRotorpart(0))->getPhi()*180/pi
                            +360*b/_number_of_blades*(_ccw?1:-1);
                        if (*f>360) *f-=360;
                        if (*f<0) *f+=360;
                        int k,l;
                        float rk,rl,p;
                        p=(*f/90);
                        k=int(p);
                        l=int(p+1);
                        rk=l-p;
                        rl=1-rk;
                        if(w==2) {k+=2;l+=2;}
                        else
                            if(w==1) {k+=1;l+=1;}
                            k%=4;
                            l%=4;
                            if (w==1) *f=rk*((Rotorpart*) getRotorpart(k))->getrealAlpha()*180/pi
                                +rl*((Rotorpart*) getRotorpart(l))->getrealAlpha()*180/pi;
                            else if(w==2) *f=rk*((Rotorpart*)getRotorpart(k))->getIncidence()*180/pi
                                +rl*((Rotorpart*)getRotorpart(l))->getIncidence()*180/pi;
                    }
                    return j+1;
}

void Rotorgear::setEngineOn(int value)
{
    _engineon=value;
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

void Rotor::setRelLenHinge(float value)
{
    _rel_len_hinge=value;
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

void Rotor::setParameter(char *parametername, float value)
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
        cout << "internal error in parameter set up for rotor: '"
            << parametername <<"'" << endl;
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

void Rotor::setCollective(float lval)
{
    lval = Math::clamp(lval, -1, 1);
    int i;
    for(i=0; i<_rotorparts.size(); i++) {
        ((Rotorpart*)_rotorparts.get(i))->setCollective(lval);
    }
    _collective=_min_pitch+(lval+1)/2*(_max_pitch-_min_pitch);
}

void Rotor::setCyclicele(float lval,float rval)
{
    rval = Math::clamp(rval, -1, 1);
    lval = Math::clamp(lval, -1, 1);
    float col=_mincyclicele+(lval+1)/2*(_maxcyclicele-_mincyclicele);
    if (_rotorparts.size()!=4) return;
    ((Rotorpart*)_rotorparts.get(1))->setCyclic(lval);
    ((Rotorpart*)_rotorparts.get(3))->setCyclic(-lval);
}

void Rotor::setCyclicail(float lval,float rval)
{
    lval = Math::clamp(lval, -1, 1);
    rval = Math::clamp(rval, -1, 1);
    float col=_mincyclicail+(lval+1)/2*(_maxcyclicail-_mincyclicail);
    if (_rotorparts.size()!=4) return;
    if (_ccw) lval *=-1;
    ((Rotorpart*)_rotorparts.get(0))->setCyclic(-lval);
    ((Rotorpart*)_rotorparts.get(2))->setCyclic( lval);
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

    // find h, the distance to the ground 
    // The ground plane transformed to the local frame.
    float ground[4];
    s->planeGlobalToLocal(_global_ground, ground);

    float h = ground[3] - Math::dot3(_base, ground);
    // Now h is the distance from the rotor center to ground

    // Calculate ground effect
    _f_ge=1+_diameter/h*_ground_effect_constant;

    // Now calculate translational lift
    float v_vert = Math::dot3(v,_normal);
    float help[3];
    Math::cross3(v,_normal,help);
    float v_horiz = Math::mag3(help);
    _f_tl = ((1-Math::pow(2.7183,-v_horiz/_translift_ve))
        *(_translift_maxfactor-1)+1)/_translift_maxfactor;

    _lift_factor = _f_ge*_f_tl*_f_vs;
}

float Rotor::getGroundEffect(float* posOut)
{
    return _diameter*0; //ground effect for rotor is calcualted not here
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
    Math::mul3(-v,_normal_with_yaw_roll,downwash);
    //the downwash is calculated in the opposite direction of the normal
}

void Rotor::compile()
{
    // Have we already been compiled?
    if(_rotorparts.size() != 0) return;

    //rotor is divided into 4 pointlike parts

    SG_LOG(SG_FLIGHT, SG_DEBUG, "debug: e "
        << _mincyclicele << "..." <<_maxcyclicele << ' '
        << _mincyclicail << "..." << _maxcyclicail << ' '
        << _min_pitch << "..." << _max_pitch);

    _dynamic=_dynamic*(1/                          //inverse of the time
        ( (60/_rotor_rpm)/4         //for rotating 90 deg
        +(60/_rotor_rpm)/(2*_number_of_blades) //+ meantime a rotorblade 
                                               //will pass a given point 
        ));
    float directions[5][3];
    //pointing forward, right, ... the 5th is ony for calculation
    directions[0][0]=_forward[0];
    directions[0][1]=_forward[1];
    directions[0][2]=_forward[2];
    int i;
    SG_LOG(SG_FLIGHT, SG_DEBUG, "Rotor rotating ccw? " << _ccw);
    for (i=1;i<5;i++)
    {
        if (!_ccw)
            Math::cross3(directions[i-1],_normal,directions[i]);
        else
            Math::cross3(_normal,directions[i-1],directions[i]);
        Math::unit3(directions[i],directions[i]);
    }
    Math::set3(directions[4],directions[0]);
    float rotorpartmass = _weight_per_blade*_number_of_blades/4*.453;
    //was pounds -> now kg

    _torque_of_inertia = 1/12. * ( 4 * rotorpartmass) * _diameter 
        * _diameter * _rel_blade_center * _rel_blade_center /(0.5*0.5);
    float speed=_rotor_rpm/60*_diameter*_rel_blade_center*pi;
    float lentocenter=_diameter*_rel_blade_center*0.5;
    float lentoforceattac=_diameter*_rel_len_hinge*0.5;
    float zentforce=rotorpartmass*speed*speed/lentocenter;
    float pitchaforce=_force_at_pitch_a/4*.453*9.81;
    //was pounds of force, now N, devided by 4 (so its now per rotorpart)

    float torque0=0,torquemax=0,torqueb=0;
    float omega=_rotor_rpm/60*2*pi;
    _omegan=omega;
    float omega0=omega*Math::sqrt(1/(1-_rel_len_hinge));
    _delta*=pitchaforce/(_pitch_a*omega*lentocenter*2*rotorpartmass);

    float phi=Math::atan2(2*omega*_delta,omega0*omega0-omega*omega);
    float relamp=omega*omega/(2*_delta*Math::sqrt(sqr(omega0*omega0-omega*omega)
        +4*_delta*_delta*omega*omega));
    if (!_no_torque)
    {
        torque0=_power_at_pitch_0/4*1000/omega;  
        // f*r=p/w ; p=f*s/t;  r=s/t/w ; r*w*t = s
        torqueb=_power_at_pitch_b/4*1000/omega;
        torquemax=_power_at_pitch_b/4*1000/omega/_pitch_b*_max_pitch;

        if(_ccw)
        {
            torque0*=-1;
            torquemax*=-1;
            torqueb*=-1;
        }
    }

    SG_LOG(SG_FLIGHT, SG_DEBUG,
        "spd: " << setprecision(8) << speed
        << " lentoc: " << lentocenter
        << " dia: " << _diameter
        << " rbl: " << _rel_blade_center
        << " hing: " << _rel_len_hinge
        << " lfa: " << lentoforceattac);

    SG_LOG(SG_FLIGHT, SG_DEBUG,
        "tq: " << setprecision(8) << torque0 << ".." << torquemax
        << " d3: " << _delta3);
    SG_LOG(SG_FLIGHT, SG_DEBUG,
        "o/o0: " << setprecision(8) << omega/omega0
        << " phi: " << phi*180/pi
        << " relamp: " << relamp
        << " delta: " <<_delta);

    Rotorpart* rps[4];
    for (i=0;i<4;i++)
    {
        float lpos[3],lforceattac[3],lspeed[3],dirzentforce[3];

        Math::mul3(lentocenter,directions[i],lpos);
        Math::add3(lpos,_base,lpos);
        Math::mul3(lentoforceattac,directions[i+1],lforceattac);
        //i+1: +90deg (gyro)!!!

        Math::add3(lforceattac,_base,lforceattac);
        Math::mul3(speed,directions[i+1],lspeed);
        Math::mul3(1,directions[i+1],dirzentforce);

        float maxcyclic=(i&1)?_maxcyclicele:_maxcyclicail;
        float mincyclic=(i&1)?_mincyclicele:_mincyclicail;

        Rotorpart* rp=rps[i]=newRotorpart(lpos, lforceattac, _normal,
            lspeed,dirzentforce,zentforce,pitchaforce, _max_pitch,_min_pitch,
            mincyclic,maxcyclic,_delta3,rotorpartmass,_translift,
            _rel_len_hinge,lentocenter);
        rp->setAlphaoutput(_alphaoutput[i&1?i:(_ccw?i^2:i)],0);
        rp->setAlphaoutput(_alphaoutput[4+(i&1?i:(_ccw?i^2:i))],1+(i>1));
        _rotorparts.add(rp);
        rp->setTorque(torquemax,torque0);
        rp->setRelamp(relamp);
        rp->setDirectionofRotorPart(directions[i]);
        rp->setTorqueOfInertia(_torque_of_inertia/4.);
    }
    for (i=0;i<4;i++)
    {
        rps[i]->setlastnextrp(rps[(i+3)%4],rps[(i+1)%4],rps[(i+2)%4]);
    }
    for (i=0;i<4;i++)
    {
        rps[i]->setCompiled();
    }
    float lift[4],torque[4], v_wind[3];
    v_wind[0]=v_wind[1]=v_wind[2]=0;
    rps[0]->setOmega(_omegan);

    if (_airfoil_lift_coefficient==0)
    {
        //calculate the lift and drag coefficients now
        _liftcoef=0;
        _dragcoef0=1;
        _dragcoef1=0;
        rps[0]->calculateAlpha(v_wind,rho_null,0,0,0,&(torque[0]),&(lift[0])); 
        //0 degree, c0

        _liftcoef=0;
        _dragcoef0=0;
        _dragcoef1=1;
        rps[0]->calculateAlpha(v_wind,rho_null,0,0,0,&(torque[1]),&(lift[1]));
        //0 degree, c1

        _liftcoef=0;
        _dragcoef0=1;
        _dragcoef1=0;
        rps[0]->calculateAlpha(v_wind,rho_null,_pitch_b,0,0,&(torque[2]),&(lift[2])); 
        //picth b, c0

        _liftcoef=0;
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

        _liftcoef=1;
        rps[0]->calculateAlpha(v_wind,rho_null,_pitch_a,0,0,
            &(torque[0]),&(lift[0])); //max_pitch a
        _liftcoef = pitchaforce/lift[0];
    }
    else
    {
        _liftcoef=_airfoil_lift_coefficient/4*_number_of_blades;
        _dragcoef0=_airfoil_drag_coefficient0/4*_number_of_blades;
        _dragcoef1=_airfoil_drag_coefficient1/4*_number_of_blades;
    }

    //Check
    rps[0]->calculateAlpha(v_wind,rho_null,_pitch_a,0,0,
        &(torque[0]),&(lift[0])); //pitch a
    rps[0]->calculateAlpha(v_wind,rho_null,_pitch_b,0,0,
        &(torque[1]),&(lift[1])); //pitch b
    rps[0]->calculateAlpha(v_wind,rho_null,0,0,0,
        &(torque[3]),&(lift[3])); //pitch 0
    SG_LOG(SG_FLIGHT, SG_DEBUG,
        "Rotor: coefficients for airfoil:" << endl << setprecision(6)
        << " drag0: " << _dragcoef0*4/_number_of_blades
        << " drag1: " << _dragcoef1*4/_number_of_blades
        << " lift: " << _liftcoef*4/_number_of_blades
        << endl
        << "at 10 deg:" << endl
        << "drag: " << (Math::sin(10./180*pi)*_dragcoef1+_dragcoef0)
            *4/_number_of_blades
        << "lift: " << Math::sin(10./180*pi)*_liftcoef*4/_number_of_blades
        << endl
        << "Some results (Pitch [degree], Power [kW], Lift [N]" << endl
        << 0.0f << "deg " << Math::abs(torque[3]*4*_omegan/1000) << "kW "
            << lift[3] << endl
        << _pitch_a*180/pi << "deg " << Math::abs(torque[0]*4*_omegan/1000) 
            << "kW " << lift[0] << endl
        << _pitch_b*180/pi << "deg " << Math::abs(torque[1]*4*_omegan/1000) 
            << "kW " << lift[1] << endl << endl);

    rps[0]->setOmega(0);
}

Rotorpart* Rotor::newRotorpart(float* pos, float *posforceattac, float *normal,
    float* speed,float *dirzentforce, float zentforce,float maxpitchforce,
    float maxpitch, float minpitch, float mincyclic,float maxcyclic,
    float delta3,float mass,float translift,float rellenhinge,float len)
{
    Rotorpart *r = new Rotorpart();
    r->setPosition(pos);
    r->setNormal(normal);
    r->setZentipetalForce(zentforce);
    r->setPositionForceAttac(posforceattac);
    r->setSpeed(speed);
    r->setDirectionofZentipetalforce(dirzentforce);
    r->setMaxpitch(maxpitch);
    r->setMinpitch(minpitch);
    r->setMaxcyclic(maxcyclic);
    r->setMincyclic(mincyclic);
    r->setDelta3(delta3);
    r->setDynamic(_dynamic);
    r->setTranslift(_translift);
    r->setC2(_c2);
    r->setWeight(mass);
    r->setRelLenHinge(rellenhinge);
    r->setOmegaN(_omegan);
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
#undef p

    SG_LOG(SG_FLIGHT, SG_DEBUG, setprecision(8)
        << "newrp: pos("
        << pos[0] << ' ' << pos[1] << ' ' << pos[2]
        << ") pfa ("
        << posforceattac[0] << ' ' << posforceattac[1] << ' ' 
        << posforceattac[2] << ')');
    SG_LOG(SG_FLIGHT, SG_DEBUG, setprecision(8)
        << "       nor("
        << normal[0] << ' ' << normal[1] << ' ' << normal[2]
        << ") spd ("
        << speed[0] << ' ' << speed[1] << ' ' << speed[2] << ')');
    SG_LOG(SG_FLIGHT, SG_DEBUG, setprecision(8)
        << "       dzf("
        << dirzentforce[0] << ' ' << dirzentforce[1] << dirzentforce[2]
        << ") zf  (" << zentforce << ") mpf (" << maxpitchforce << ')');
        SG_LOG(SG_FLIGHT, SG_DEBUG, setprecision(8)
        << "       pit(" << minpitch << ".." << maxpitch
        << ") mcy (" << mincyclic << ".." << maxcyclic
        << ") d3 (" << delta3 << ')');
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
        if (_engineon)
        {
            max_torque_of_engine=_max_power_engine;
            float df=1-omegarel;
            df/=_engine_prop_factor;
            df = Math::clamp(df, 0, 1);
            max_torque_of_engine = df * _max_power_engine;
        }
        total_torque*=-1;
        _ddt_omegarel=0;
        float rel_torque_engine=1;
        if (total_torque<=0)
            rel_torque_engine=0;
        else
            if (max_torque_of_engine>0)
                rel_torque_engine=1/max_torque_of_engine*total_torque;
            else
                rel_torque_engine=0;

        //add the rotor brake
        float dt=0.1f;
        if (r0->_rotorparts.size()) dt=((Rotorpart*)r0->_rotorparts.get(0))->getDt();

        float rotor_brake_torque;
        rotor_brake_torque=_rotorbrake*_max_power_rotor_brake;
        //clamp it to the value you need to stop the rotor
        rotor_brake_torque=Math::clamp(rotor_brake_torque,0,
            total_torque_of_inertia/dt*omegarel);
        max_torque_of_engine-=rotor_brake_torque;

        //change the rotation of the rotors 
        if ((max_torque_of_engine<total_torque) //decreasing rotation
            ||((max_torque_of_engine>total_torque)&&(omegarel<1))
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
                
                if (lim1<_engine_accell_limit) lim1=_engine_accell_limit; 
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
                    float torque_scalar=0;
                    Rotorpart* rp = (Rotorpart*)r->_rotorparts.get(i);
                    float torque[3];
                    rp->getAccelTorque(_ddt_omegarel,torque);
                    Math::add3(torque,torqueOut,torqueOut);
                }
            }
        }
    }
}

void Rotorgear::addRotor(Rotor* rotor)
{
    _rotors.add(rotor);
    _in_use = 1;
}

float Rotorgear::compile(RigidBody* body)
{
    float wgt = 0;
    for(int j=0; j<_rotors.size(); j++) {
        Rotor* r = (Rotor*)_rotors.get(j);
        r->compile();
        int i;
        for(i=0; i<r->numRotorparts(); i++) {
            Rotorpart* rp = (Rotorpart*)r->getRotorpart(i);
            float mass = rp->getWeight();
            mass = mass * Math::sqrt(mass);
            float pos[3];
            rp->getPosition(pos);
            body->addMass(mass, pos);
            wgt += mass;
        }
    }
    return wgt;
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
        p(engine_accell_limit,0.01)
        cout << "internal error in parameter set up for rotorgear: '"
            << parametername <<"'" << endl;
#undef p
}

Rotorgear::Rotorgear()
{
    _in_use=0;
    _engineon=0;
    _rotorbrake=0;
    _max_power_rotor_brake=1;
    _max_power_engine=1000*450;
    _engine_prop_factor=0.05f;
    _yasimdragfactor=1;
    _yasimliftfactor=1;
    _ddt_omegarel=0;
    _engine_accell_limit=0.05f;
}

Rotorgear::~Rotorgear()
{
    for(int i=0; i<_rotors.size(); i++)
        delete (Rotor*)_rotors.get(i);
}

}; // namespace yasim
