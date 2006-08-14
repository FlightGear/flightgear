#include <simgear/debug/logstream.hxx>

#include "Math.hpp"
#include "Rotorpart.hpp"
#include "Rotor.hpp"
#include <stdio.h>
#include <string.h>
namespace yasim {
const float pi=3.14159;
float _help = 0;
Rotorpart::Rotorpart()
{
    _compiled=0;
    _cyclic=0;
    _collective=0;
    _rellenhinge=0;
    _dt=0;
#define set3(x,a,b,c) x[0]=a;x[1]=b;x[2]=c;
    set3 (_speed,1,0,0);
    set3 (_directionofzentipetalforce,1,0,0);
    set3 (_directionofrotorpart,0,1,0);
    set3 (_direction_of_movement,1,0,0);
    set3 (_last_torque,0,0,0);
#undef set3
    _zentipetalforce=1;
    _maxpitch=.02;
    _minpitch=0;
    _maxcyclic=0.02;
    _mincyclic=-0.02;
    _delta3=0.5;
    _cyclic=0;
    _collective=-1;
    _relamp=1;
    _mass=10;
    _incidence = 0;
    _alpha=0;
    _alphamin=-.1;
    _alphamax= .1;
    _alpha0=-.05;
    _alpha0factor=1;
    _alphaoutputbuf[0][0]=0;
    _alphaoutputbuf[1][0]=0;
    _alpha2type=0;
    _alphaalt=0;
    _lastrp=0;
    _nextrp=0;
    _oppositerp=0;
    _translift=0;
    _dynamic=100;
    _c2=0;
    _torque_max_force=0;
    _torque_no_force=0;
    _omega=0;
    _omegan=1;
    _ddt_omega=0;
    _phi=0;
    _len=1;
    _rotor=NULL;
    _twist=0; 
    _number_of_segments=1;
    _rel_len_where_incidence_is_measured=0.7;
    _diameter=10;
    _torque_of_inertia=0;
    _rel_len_blade_start=0;
}

void Rotorpart::inititeration(float dt,float *rot)
{
    _dt=dt;
    _phi+=_omega*dt;
    while (_phi>(2*pi)) _phi-=2*pi;
    while (_phi<(0   )) _phi+=2*pi;
    float a=Math::dot3(rot,_normal);
    if(a>0)
        _alphaalt=_alpha*Math::cos(a)
        +_nextrp->getrealAlpha()*Math::sin(a);
    else
        _alphaalt=_alpha*Math::cos(a)
        +_lastrp->getrealAlpha()*Math::sin(-a);
    //calculate the rotation of the fuselage, determine
    //the part in the same direction as alpha
    //and add it ro _alphaalt
    //alpha is rotation about "normal cross dirofzentf"

    float dir[3];
    Math::cross3(_directionofzentipetalforce,_normal,dir);
    a=Math::dot3(rot,dir);
    _alphaalt -= a;
    _alphaalt= Math::clamp(_alphaalt,_alphamin,_alphamax);
}

void Rotorpart::setRotor(Rotor *rotor)
{
    _rotor=rotor;
}

void Rotorpart::setParameter(char *parametername, float value)
{
#define p(a) if (strcmp(parametername,#a)==0) _##a = value; else

    p(twist)
        p(number_of_segments)
        p(rel_len_where_incidence_is_measured)
        p(rel_len_blade_start)
        cout << "internal error in parameter set up for rotorpart: '"
            << parametername <<"'" << endl;
#undef p
}

void Rotorpart::setTorque(float torque_max_force,float torque_no_force)
{
    _torque_max_force=torque_max_force;
    _torque_no_force=torque_no_force;
}

void Rotorpart::setTorqueOfInertia(float toi)
{
    _torque_of_inertia=toi;
}

void Rotorpart::setWeight(float value)
{
    _mass=value;
}

float Rotorpart::getWeight(void)
{
    return(_mass/.453); //_mass is in kg, returns pounds 
}

void Rotorpart::setPosition(float* p)
{
    int i;
    for(i=0; i<3; i++) _pos[i] = p[i];
}

float Rotorpart::getIncidence()
{
    return(_incidence);
}

void Rotorpart::getPosition(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _pos[i];
}

void Rotorpart::setPositionForceAttac(float* p)
{
    int i;
    for(i=0; i<3; i++) _posforceattac[i] = p[i];
}

void Rotorpart::getPositionForceAttac(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _posforceattac[i];
}

void Rotorpart::setSpeed(float* p)
{
    int i;
    for(i=0; i<3; i++) _speed[i] = p[i];
    Math::unit3(_speed,_direction_of_movement); 
}

void Rotorpart::setDirectionofZentipetalforce(float* p)
{
    int i;
    for(i=0; i<3; i++) _directionofzentipetalforce[i] = p[i];
}

void Rotorpart::setDirectionofRotorPart(float* p)
{
    int i;
    for(i=0; i<3; i++) _directionofrotorpart[i] = p[i];
}

void Rotorpart::setOmega(float value)
{
    _omega=value;
}

void Rotorpart::setOmegaN(float value)
{
    _omegan=value;
}

void Rotorpart::setDdtOmega(float value)
{
    _ddt_omega=value;
}

void Rotorpart::setZentipetalForce(float f)
{
    _zentipetalforce=f;
} 

void Rotorpart::setMinpitch(float f)
{
    _minpitch=f;
} 

void Rotorpart::setMaxpitch(float f)
{
    _maxpitch=f;
} 

void Rotorpart::setMaxcyclic(float f)
{
    _maxcyclic=f;
} 

void Rotorpart::setMincyclic(float f)
{
    _mincyclic=f;
} 

void Rotorpart::setDelta3(float f)
{
    _delta3=f;
} 

void Rotorpart::setRelamp(float f)
{
    _relamp=f;
} 

void Rotorpart::setTranslift(float f)
{
    _translift=f;
}

void Rotorpart::setDynamic(float f)
{
    _dynamic=f;
}

void Rotorpart::setRelLenHinge(float f)
{
    _rellenhinge=f;
}

void Rotorpart::setC2(float f)
{
    _c2=f;
}

void Rotorpart::setAlpha0(float f)
{
    _alpha0=f;
}

void Rotorpart::setAlphamin(float f)
{
    _alphamin=f;
}

void Rotorpart::setAlphamax(float f)
{
    _alphamax=f;
}

void Rotorpart::setAlpha0factor(float f)
{
    _alpha0factor=f;
}

void Rotorpart::setDiameter(float f)
{
    _diameter=f;
}

float Rotorpart::getPhi()
{
    return(_phi);
}

float Rotorpart::getAlpha(int i)
{
    i=i&1;

    if (i==0)
        return _alpha*180/3.14;//in Grad = 1
    else
    {
        if (_alpha2type==1) //yaw or roll
            return (getAlpha(0)-_oppositerp->getAlpha(0))/2;
        else //collective
            return (getAlpha(0)+_oppositerp->getAlpha(0)+
            _nextrp->getAlpha(0)+_lastrp->getAlpha(0))/4;
    }
}
float Rotorpart::getrealAlpha(void)
{
    return _alpha;
}

void Rotorpart::setAlphaoutput(char *text,int i)
{
    SG_LOG(SG_FLIGHT, SG_DEBUG, "setAlphaoutput rotorpart ["
        << text << "] typ" << i);

    strncpy(_alphaoutputbuf[i>0],text,255);

    if (i>0) _alpha2type=i;
}

char* Rotorpart::getAlphaoutput(int i)
{
    return _alphaoutputbuf[i&1];
}

void Rotorpart::setLen(float value)
{
    _len=value;
}

void Rotorpart::setNormal(float* p)
{
    int i;
    for(i=0; i<3; i++) _normal[i] = p[i];
}

void Rotorpart::getNormal(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _normal[i];
}

void Rotorpart::setCollective(float pos)
{
    _collective = pos;
}

void Rotorpart::setCyclic(float pos)
{
    _cyclic = pos;
}

void Rotorpart::setlastnextrp(Rotorpart*lastrp,Rotorpart*nextrp,
    Rotorpart *oppositerp)
{
    _lastrp=lastrp;
    _nextrp=nextrp;
    _oppositerp=oppositerp;
}

void Rotorpart::strncpy(char *dest,const char *src,int maxlen)
{
    int n=0;
    while(src[n]&&n<(maxlen-1))
    {
        dest[n]=src[n];
        n++;
    }
    dest[n]=0;
}

// Calculate the flapping angle, where zentripetal force and
//lift compensate each other
float Rotorpart::calculateAlpha(float* v_rel_air, float rho, 
    float incidence, float cyc, float alphaalt, float *torque,
    float *returnlift)
{
    float moment[3],v_local[3],v_local_scalar,lift_moment,v_flap[3],v_help[3];
    float ias;//nur f. dgb
    int i,n;
    for (i=0;i<3;i++)
        moment[i]=0;
    lift_moment=0;
    *torque=0;//
    if((_nextrp==NULL)||(_lastrp==NULL)||(_rotor==NULL)) 
        return 0.0;//not initialized. Can happen during startupt of flightgear
    if (returnlift!=NULL) *returnlift=0;
    float flap_omega=(_nextrp->getrealAlpha()-_lastrp->getrealAlpha())
        *_omega / pi;
    float local_width=_diameter*(1-_rel_len_blade_start)/2.
        /(float (_number_of_segments));
    for (n=0;n<_number_of_segments;n++)
    {
        float rel = (n+.5)/(float (_number_of_segments));
        float r= _diameter *0.5 *(rel*(1-_rel_len_blade_start)
            +_rel_len_blade_start);
        float local_incidence=incidence+_twist *rel - _twist
            *_rel_len_where_incidence_is_measured;
        float local_chord = _rotor->getChord()*rel+_rotor->getChord()
            *_rotor->getTaper()*(1-rel);
        float A = local_chord * local_width;
        //calculate the local air speed and the incidence to this speed;
        Math::mul3(_omega * r , _direction_of_movement , v_local);

        // add speed component due to flapping
        Math::mul3(flap_omega * r,_normal,v_flap);
        Math::add3(v_flap,v_local,v_local);
        Math::sub3(v_rel_air,v_local,v_local); 
        //v_local is now the total airspeed at the blade 
        //apparent missing: calculating the local_wind = v_rel_air at 
        //every point of the rotor. It differs due to aircraft-rotation
        //it is considered in v_flap

        //substract now the component of the air speed parallel to 
        //the blade;
       Math::mul3(Math::dot3(v_local,_directionofrotorpart),
           _directionofrotorpart,v_help);
        Math::sub3(v_local,v_help,v_local);

        //split into direction and magnitude
        v_local_scalar=Math::mag3(v_local);
        if (v_local_scalar!=0)
            Math::unit3(v_local,v_local);
        float incidence_of_airspeed = Math::asin(Math::clamp(
            Math::dot3(v_local,_normal),-1,1)) + local_incidence;
        ias = incidence_of_airspeed;
        float lift_wo_cyc = 
            _rotor->getLiftCoef(incidence_of_airspeed-cyc,v_local_scalar)
            * v_local_scalar * v_local_scalar * A *rho *0.5;
        float lift_with_cyc = 
            _rotor->getLiftCoef(incidence_of_airspeed,v_local_scalar)
            * v_local_scalar * v_local_scalar *A *rho*0.5;
        float lift=lift_wo_cyc+_relamp*(lift_with_cyc-lift_wo_cyc);
        //take into account that the rotor is a resonant system where
        //the cyclic input hase increased result
        float drag = -_rotor->getDragCoef(incidence_of_airspeed,v_local_scalar) 
            * v_local_scalar * v_local_scalar * A *rho*0.5;
        float angle = incidence_of_airspeed - incidence; 
        //angle between blade movement caused by rotor-rotation and the
        //total movement of the blade

        lift_moment += r*(lift * Math::cos(angle) 
            - drag * Math::sin(angle));
        *torque     += r*(drag * Math::cos(angle) 
            + lift * Math::sin(angle));

        if (returnlift!=NULL) *returnlift+=lift;
    }
    float alpha=Math::atan2(lift_moment,_zentipetalforce * _len); 

    return (alpha);
}

// Calculate the aerodynamic force given a wind vector v (in the
// aircraft's "local" coordinates) and an air density rho.  Returns a
// torque about the Y axis, too.
void Rotorpart::calcForce(float* v, float rho,  float* out, float* torque,
    float* torque_scalar)
{
    if (_compiled!=1)
    {
        for (int i=0;i<3;i++)
            torque[i]=out[i]=0;
        *torque_scalar=0;
        return;
    }
    _zentipetalforce=_mass*_len*_omega*_omega;
    float vrel[3],vreldir[3];
    Math::sub3(_speed,v,vrel);
    float scalar_torque=0,alpha_alteberechnung=0;
    Math::unit3(vrel,vreldir);//direction of blade-movement rel. to air
    float delta=Math::asin(Math::dot3(_normal,vreldir));
    //Angle of blade which would produce no vertical force (where the 
    //effective incidence is zero)

    float cyc=_mincyclic+(_cyclic+1)/2*(_maxcyclic-_mincyclic);
    float col=_minpitch+(_collective+1)/2*(_maxpitch-_minpitch);
    _incidence=(col+cyc)-_delta3*_alphaalt;
    //the incidence of the rotorblade due to control input reduced by the
    //delta3 effect, see README.YASIM
    float beta=_relamp*cyc+col; 
    //the incidence of the rotorblade which is used for the calculation

    float alpha,factor; //alpha is the flapping angle
    //the new flapping angle will be the old flapping angle
    //+ factor *(alpha - "old flapping angle")
    if((_omega*10)>_omegan) 
    //the rotor is rotaing quite fast.
    //(at least 10% of the nominal rotational speed)
    {
        alpha=calculateAlpha(v,rho,_incidence,cyc,0,&scalar_torque);
        //the incidence is a function of alpha (if _delta* != 0)
        //Therefore missing: wrap this function in an integrator
        //(runge kutta e. g.)

        factor=_dt*_dynamic;
        if (factor>1) factor=1;
    }
    else //the rotor is not rotating or rotating very slowly 
    {
        alpha=calculateAlpha(v,rho,_incidence,cyc,alpha_alteberechnung,
            &scalar_torque);
        //calculate drag etc., e. g. for deccelrating the rotor if engine
        //is off and omega <10%

        float rel =_omega*10 / _omegan;
        alpha=rel * alpha + (1-rel)* _alpha0;
        factor=_dt*_dynamic/10;
        if (factor>1) factor=1;
    }

    float vz=Math::dot3(_normal,v); //the s
    float dirblade[3];
    Math::cross3(_normal,_directionofzentipetalforce,dirblade);
    float vblade=Math::abs(Math::dot3(dirblade,v));
    float tliftfactor=Math::sqrt(1+vblade*_translift);

    alpha=_alphaalt+(alpha-_alphaalt)*factor;
    _alpha=alpha;
    float meancosalpha=(1*Math::cos(_lastrp->getrealAlpha())
        +1*Math::cos(_nextrp->getrealAlpha())
        +1*Math::cos(_oppositerp->getrealAlpha())
        +1*Math::cos(alpha))/4;
    float schwenkfactor=1-(Math::cos(_lastrp->getrealAlpha())-meancosalpha);

    //missing: consideration of rellenhinge
    float xforce = Math::cos(alpha)*_zentipetalforce;
    float zforce = schwenkfactor*Math::sin(alpha)*_zentipetalforce;
    *torque_scalar=scalar_torque;
    scalar_torque+= 0*_ddt_omega*_torque_of_inertia;
    float thetorque = scalar_torque;
    int i;
    float f=_rotor->getCcw()?1:-1;
    for(i=0; i<3; i++) {
        _last_torque[i]=torque[i] = f*_normal[i]*thetorque;
        out[i] = _normal[i]*zforce*_rotor->getLiftFactor()
            +_directionofzentipetalforce[i]*xforce;
    }
}

void Rotorpart::getAccelTorque(float relaccel,float *t)
{
    int i;
    float f=_rotor->getCcw()?1:-1;
    for(i=0; i<3; i++) {
        t[i]=f*-1* _normal[i]*relaccel*_omegan* _torque_of_inertia;
        _rotor->addTorque(-relaccel*_omegan* _torque_of_inertia);
    }
}
}; // namespace yasim
