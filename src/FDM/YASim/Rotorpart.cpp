#include <ostream>

#include <simgear/debug/logstream.hxx>

#include "Math.hpp"
#include "Rotorpart.hpp"
#include "Rotor.hpp"
#include <stdio.h>
#include <string.h>
namespace yasim {
using std::endl;

const float pi=3.14159;
float _help = 0;
Rotorpart::Rotorpart()
{
    _compiled=0;
    _cyclic=0;
    _collective=0;
    _rellenhinge=0;
    _shared_flap_hinge=false;
    _dt=0;
#define set3(x,a,b,c) x[0]=a;x[1]=b;x[2]=c;
    set3 (_speed,1,0,0);
    set3 (_directionofcentripetalforce,1,0,0);
    set3 (_directionofrotorpart,0,1,0);
    set3 (_direction_of_movement,1,0,0);
    set3 (_last_torque,0,0,0);
#undef set3
    _centripetalforce=1;
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
    _last90rp=0;
    _next90rp=0;
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
    _torque=0;
    _rotor_correction_factor=0.6;
    _direction=0;
    _balance=1;
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
        +_next90rp->getrealAlpha()*Math::sin(a);
    else
        _alphaalt=_alpha*Math::cos(a)
        +_last90rp->getrealAlpha()*Math::sin(-a);
    //calculate the rotation of the fuselage, determine
    //the part in the same direction as alpha
    //and add it ro _alphaalt
    //alpha is rotation about "normal cross dirofzentf"

    float dir[3];
    Math::cross3(_directionofcentripetalforce,_normal,dir);
    a=Math::dot3(rot,dir);
    _alphaalt -= a;
    _alphaalt= Math::clamp(_alphaalt,_alphamin,_alphamax);

    //unbalance
    float b;
    b=_rotor->getBalance();
    float s =Math::sin(_phi+_direction);
    //float c =Math::cos(_phi+_direction);
    if (s>0)
        _balance=(b>0)?(1.-s*(1.-b)):(1.-s)*(1.+b);
    else
        _balance=(b>0)?1.:1.+b;
}

void Rotorpart::setRotor(Rotor *rotor)
{
    _rotor=rotor;
}

void Rotorpart::setParameter(const char *parametername, float value)
{
#define p(a) if (strcmp(parametername,#a)==0) _##a = value; else

    p(twist)
        p(number_of_segments)
        p(rel_len_where_incidence_is_measured)
        p(rel_len_blade_start)
        p(rotor_correction_factor)
        SG_LOG(SG_INPUT, SG_ALERT,
            "internal error in parameter set up for rotorpart: '"
            << parametername <<"'" << endl);
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
    for(int i=0; i<3; i++) _pos[i] = p[i];
}

float Rotorpart::getIncidence()
{
    return(_incidence);
}

void Rotorpart::getPosition(float* out)
{
    for(int i=0; i<3; i++) out[i] = _pos[i];
}

void Rotorpart::setPositionForceAttac(float* p)
{
    for(int i=0; i<3; i++) _posforceattac[i] = p[i];
}

void Rotorpart::getPositionForceAttac(float* out)
{
    for(int i=0; i<3; i++) out[i] = _posforceattac[i];
}

void Rotorpart::setSpeed(float* p)
{
    for(int i=0; i<3; i++) _speed[i] = p[i];
    Math::unit3(_speed,_direction_of_movement); 
}

void Rotorpart::setDirectionofZentipetalforce(float* p)
{
    for(int i=0; i<3; i++) _directionofcentripetalforce[i] = p[i];
}

void Rotorpart::setDirectionofRotorPart(float* p)
{
    for(int i=0; i<3; i++) _directionofrotorpart[i] = p[i];
}

void Rotorpart::setDirection(float direction)
{
    _direction=direction;
}

void Rotorpart::setOmega(float value)
{
    _omega=value;
}

void Rotorpart::setPhi(float value)
{
    _phi=value;
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
    _centripetalforce=f;
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

void Rotorpart::setSharedFlapHinge(bool s)
{
    _shared_flap_hinge=s;
}

void Rotorpart::setC2(float f)
{
    _c2=f;
}

void Rotorpart::setAlpha0(float f)
{
    if (f>-0.01) f=-0.01; //half a degree bending 
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
        return _alpha*180/pi;//in Grad = 1
    else
    {
        if (_alpha2type==1) //yaw or roll
            return (getAlpha(0)-_oppositerp->getAlpha(0))/2;
        else //collective
            return (getAlpha(0)+_oppositerp->getAlpha(0)+
            _next90rp->getAlpha(0)+_last90rp->getAlpha(0))/4;
    }
}
float Rotorpart::getrealAlpha(void)
{
    return _alpha;
}

void Rotorpart::setAlphaoutput(char *text,int i)
{
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
    for(int i=0; i<3; i++) _normal[i] = p[i];
}

void Rotorpart::getNormal(float* out)
{
    for(int i=0; i<3; i++) out[i] = _normal[i];
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
    Rotorpart *oppositerp,Rotorpart*last90rp,Rotorpart*next90rp)
{
    _lastrp=lastrp;
    _nextrp=nextrp;
    _oppositerp=oppositerp;
    _last90rp=last90rp;
    _next90rp=next90rp;
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
    float v_local[3],v_local_scalar,lift_moment,v_flap[3],v_help[3];
    float relgrav = Math::dot3(_normal,_rotor->getGravDirection());
    lift_moment=-_mass*_len*9.81*relgrav;
    *torque=0;//
    if((_nextrp==NULL)||(_lastrp==NULL)||(_rotor==NULL)) 
        return 0.0;//not initialized. Can happen during startup of flightgear
    if (returnlift!=NULL) *returnlift=0;
    float flap_omega=(_next90rp->getrealAlpha()-_last90rp->getrealAlpha())
        *_omega / pi;
    float local_width=_diameter*(1-_rel_len_blade_start)/2.
        /(float (_number_of_segments));
    for (int n=0;n<_number_of_segments;n++)
    {
        float rel = (n+.5)/(float (_number_of_segments));
        float r= _diameter *0.5 *(rel*(1-_rel_len_blade_start)
            +_rel_len_blade_start);
        float local_incidence=incidence+_twist *rel -
            _twist *_rel_len_where_incidence_is_measured;
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
            //Math::unit3(v_local,v_help);
            Math::mul3(1/v_local_scalar,v_local,v_help);
        float incidence_of_airspeed = Math::asin(Math::clamp(
            Math::dot3(v_help,_normal),-1,1)) + local_incidence;
        //ias = incidence_of_airspeed;

        //reduce the ias (Prantl factor)
        float prantl_factor=2/pi*Math::acos(Math::exp(
            -_rotor->getNumberOfBlades()/2.*(1-rel)
             *Math::sqrt(1+1/Math::sqr(Math::tan(
               pi/2-Math::abs(incidence_of_airspeed-local_incidence))))));
        incidence_of_airspeed = (incidence_of_airspeed+
            _rotor->getAirfoilIncidenceNoLift())*prantl_factor
            *_rotor_correction_factor-_rotor->getAirfoilIncidenceNoLift();
        //ias = incidence_of_airspeed;
        float lift_wo_cyc = _rotor->getLiftCoef(incidence_of_airspeed
            -cyc*_rotor_correction_factor*prantl_factor,v_local_scalar)
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
    //use 1st order approximation for alpha
    //float alpha=Math::atan2(lift_moment,_centripetalforce * _len); 
    float alpha;
    if (_shared_flap_hinge)
    {
        float div=0;
        if (Math::abs(_alphaalt) >1e-6)
            div=(_centripetalforce * _len - _mass * _len * 9.81 * relgrav /_alpha0*(_alphaalt+_oppositerp->getAlphaAlt())/(2.0*_alphaalt));
        if (Math::abs(div)>1e-6)
        {
            alpha=lift_moment/div;
        }
        else if(Math::abs(_alphaalt+_oppositerp->getAlphaAlt())>1e-6)
        {
            float div=(_centripetalforce * _len - _mass * _len * 9.81 *0.5 * relgrav)*(_alphaalt+_oppositerp->getAlphaAlt());
            if (Math::abs(div)>1e-6)
            {
                alpha=_oppositerp->getAlphaAlt()+lift_moment/div*_alphaalt;
            }
            else
                alpha=_alphaalt;
        }
        else
            alpha=_alphaalt;
        if (_omega/_omegan<0.2)
        {
            float frac = 0.001+_omega/_omegan*4.995;
            alpha=Math::clamp(alpha,_alphamin,_alphamax);
            alpha=_alphaalt*(1-frac)+frac*alpha;
        }
    }
    else
    {
        float div=(_centripetalforce * _len - _mass * _len * 9.81 /_alpha0);
        if (Math::abs(div)>1e-6)
            alpha=lift_moment/div;
        else
            alpha=_alphaalt;
    }
 
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
    _centripetalforce=_mass*_len*_omega*_omega;
    float vrel[3],vreldir[3];
    Math::sub3(_speed,v,vrel);
    float scalar_torque=0;
    Math::unit3(vrel,vreldir);//direction of blade-movement rel. to air
    //Angle of blade which would produce no vertical force (where the 
    //effective incidence is zero)

    float cyc=_cyclic;
    float col=_collective;
    if (_shared_flap_hinge)
        _incidence=(col+cyc)-_delta3*0.5*(_alphaalt-_oppositerp->getAlphaAlt());
    else
        _incidence=(col+cyc)-_delta3*_alphaalt;
    //the incidence of the rotorblade due to control input reduced by the
    //delta3 effect, see README.YASIM
    //float beta=_relamp*cyc+col; 
    //the incidence of the rotorblade which is used for the calculation

    float alpha,factor; //alpha is the flapping angle
    //the new flapping angle will be the old flapping angle
    //+ factor *(alpha - "old flapping angle")
    alpha=calculateAlpha(v,rho,_incidence,cyc,0,&scalar_torque);
    alpha=Math::clamp(alpha,_alphamin,_alphamax);
    //the incidence is a function of alpha (if _delta* != 0)
    //Therefore missing: wrap this function in an integrator
    //(runge kutta e. g.)

    factor=_dt*_dynamic;
    if (factor>1) factor=1;

    float dirblade[3];
    Math::cross3(_normal,_directionofcentripetalforce,dirblade);
    //float vblade=Math::abs(Math::dot3(dirblade,v));

    alpha=_alphaalt+(alpha-_alphaalt)*factor;
    _alpha=alpha;
    float meancosalpha=(1*Math::cos(_last90rp->getrealAlpha())
        +1*Math::cos(_next90rp->getrealAlpha())
        +1*Math::cos(_oppositerp->getrealAlpha())
        +1*Math::cos(alpha))/4;
    float schwenkfactor=1-(Math::cos(_lastrp->getrealAlpha())-meancosalpha)*_rotor->getNumberOfParts()/4;

    //missing: consideration of rellenhinge

    //add the unbalance
    _centripetalforce*=_balance;
    scalar_torque*=_balance;

    float xforce = Math::cos(alpha)*_centripetalforce;
    float zforce = schwenkfactor*Math::sin(alpha)*_centripetalforce;
    *torque_scalar=scalar_torque;
    scalar_torque+= 0*_ddt_omega*_torque_of_inertia;
    float thetorque = scalar_torque;
    float f=_rotor->getCcw()?1:-1;
    for(int i=0; i<3; i++) {
        _last_torque[i]=torque[i] = f*_normal[i]*thetorque;
        out[i] = _normal[i]*zforce*_rotor->getLiftFactor()
            +_directionofcentripetalforce[i]*xforce;
    }
}

void Rotorpart::getAccelTorque(float relaccel,float *t)
{
    float f=_rotor->getCcw()?1:-1;
    for(int i=0; i<3; i++) {
        t[i]=f*-1* _normal[i]*relaccel*_omegan* _torque_of_inertia;// *_omeagan ?
        _rotor->addTorque(-relaccel*_omegan* _torque_of_inertia);
    }
}

std::ostream &  operator<<(std::ostream & out, const Rotorpart& rp)
{
#define i(x) << #x << ":" << rp.x << endl
#define iv(x) << #x << ":" << rp.x[0] << ";" << rp.x[1] << ";" <<rp.x[2] << ";" << endl
    out << "Writing Info on Rotorpart " << endl
        i( _dt)
        iv( _last_torque)
        i( _compiled)
        iv( _pos)    // position in local coords
        iv( _posforceattac)    // position in local coords
        iv( _normal) //direcetion of the rotation axis
        i( _torque_max_force)
        i( _torque_no_force)
        iv( _speed)
        iv( _direction_of_movement)
        iv( _directionofcentripetalforce)
        iv( _directionofrotorpart)
        i( _centripetalforce)
        i( _cyclic)
        i( _collective)
        i( _delta3)
        i( _dynamic)
        i( _translift)
        i( _c2)
        i( _mass)
        i( _alpha)
        i( _alphaalt)
        i( _alphamin) i(_alphamax) i(_alpha0) i(_alpha0factor)
        i( _rellenhinge)
        i( _relamp)
        i( _omega) i(_omegan) i(_ddt_omega)
        i( _phi)
        i( _len)
        i( _incidence)
        i( _twist) //outer incidence = inner inner incidence + _twist
        i( _number_of_segments)
        i( _rel_len_where_incidence_is_measured)
        i( _rel_len_blade_start)
        i( _diameter)
        i( _torque_of_inertia)
        i( _torque) 
        i (_alphaoutputbuf[0])
        i (_alphaoutputbuf[1])
        i( _alpha2type)
        i(_rotor_correction_factor)
    << endl;
#undef i
#undef iv
    return out;  
}
}; // namespace yasim
