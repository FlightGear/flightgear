#include <simgear/debug/logstream.hxx>

#include "Math.hpp"
#include "Surface.hpp"
#include "Rotorpart.hpp"
#include "Rotorblade.hpp"
#include "Rotor.hpp"

#include STL_IOSTREAM
#include STL_IOMANIP

SG_USING_STD(setprecision);

//#include <string.h>
#include <stdio.h>

namespace yasim {

const float pi=3.14159;

static inline float sqr(float a) { return a * a; }

Rotor::Rotor()
{
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
    _force_at_max_pitch=0;
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
    _number_of_blades=4;
    _omega=_omegan=_omegarel=0;
    _pitch_a=0;
    _pitch_b=0;
    _power_at_pitch_0=0;
    _power_at_pitch_b=0;
    _rel_blade_center=.7;
    _rel_len_hinge=0.01;
    _rellenteeterhinge=0.01;
    _rotor_rpm=442;
    _sim_blades=0;
    _teeterdamp=0.00001;
    _translift=0.05;
    _weight_per_blade=42;


    
}

Rotor::~Rotor()
{
    int i;
    for(i=0; i<_rotorparts.size(); i++) {
        Rotorpart* r = (Rotorpart*)_rotorparts.get(i);
        delete r;
    }
    for(i=0; i<_rotorblades.size(); i++) {
        Rotorblade* r = (Rotorblade*)_rotorblades.get(i);
        delete r;
    }
    
}

void Rotor::inititeration(float dt)
{
   if ((_engineon)&&(_omegarel>=1)) return;
   if ((!_engineon)&&(_omegarel<=0)) return;
   _omegarel+=dt*1/30.*(_engineon?1:-1);
   _omegarel=Math::clamp(_omegarel,0,1);
   _omega=_omegan*_omegarel;
   int i;
    for(i=0; i<_rotorparts.size(); i++) {
        Rotorpart* r = (Rotorpart*)_rotorparts.get(i);
        r->setOmega(_omega);
    }
    for(i=0; i<_rotorblades.size(); i++) {
        Rotorblade* r = (Rotorblade*)_rotorblades.get(i);
        r->setOmega(_omega);
    }
}

int Rotor::getValueforFGSet(int j,char *text,float *f)
{
  if (_name[0]==0) return 0;
  
  
  if (_sim_blades)
  {
     if (!numRotorblades()) return 0;
     if (j==0)
     {
        sprintf(text,"/rotors/%s/cone", _name);

        *f=( ((Rotorblade*)getRotorblade(0))->getFlapatPos(0)
            +((Rotorblade*)getRotorblade(0))->getFlapatPos(1)
            +((Rotorblade*)getRotorblade(0))->getFlapatPos(2)
            +((Rotorblade*)getRotorblade(0))->getFlapatPos(3)
           )/4*180/pi;

     }
     else
     if (j==1)
     {
        sprintf(text,"/rotors/%s/roll", _name);

        *f=( ((Rotorblade*)getRotorblade(0))->getFlapatPos(1)
            -((Rotorblade*)getRotorblade(0))->getFlapatPos(3)
           )/2*180/pi;
     }
     else
     if (j==2)
     {
        sprintf(text,"/rotors/%s/yaw", _name);

        *f=( ((Rotorblade*)getRotorblade(0))->getFlapatPos(2)
            -((Rotorblade*)getRotorblade(0))->getFlapatPos(0)
           )/2*180/pi;
     }
     else
     if (j==3)
     {
        sprintf(text,"/rotors/%s/rpm", _name);

        *f=_omega/2/pi*60;
     }
     else
     {

         int b=(j-4)/3;
     
         if (b>=numRotorblades()) return 0;
         int w=j%3;
         sprintf(text,"/rotors/%s/blade%i_%s",
            _name,b+1,
            w==0?"pos":(w==1?"flap":"incidence"));
         if (w==0) *f=((Rotorblade*)getRotorblade(b))->getPhi()*180/pi;
         else if (w==1) *f=((Rotorblade*) getRotorblade(b))->getrealAlpha()*180/pi;
         else *f=((Rotorblade*)getRotorblade(b))->getIncidence()*180/pi;
     }
     return j+1;
  }
  else
  {
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
        *f=( ((Rotorpart*)getRotorpart(0))->getrealAlpha()
            -((Rotorpart*)getRotorpart(2))->getrealAlpha()
           )/2*180/pi*(_ccw?-1:1);
     }
     else
     if (j==2)
     {
        sprintf(text,"/rotors/%s/yaw", _name);
        *f=( ((Rotorpart*)getRotorpart(1))->getrealAlpha()
            -((Rotorpart*)getRotorpart(3))->getrealAlpha()
           )/2*180/pi;
     }
     else
     if (j==3)
     {
        sprintf(text,"/rotors/%s/rpm", _name);

        *f=_omega/2/pi*60;
     }
     else
     {
       int b=(j-4)/3;
       if (b>=_number_of_blades) return 0;
       int w=j%3;
       sprintf(text,"/rotors/%s/blade%i_%s",
            _name,b+1,
          w==0?"pos":(w==1?"flap":"incidence"));
       *f=((Rotorpart*)getRotorpart(0))->getPhi()*180/pi+360*b/_number_of_blades*(_ccw?1:-1);
       if (*f>360) *f-=360;
       if (*f<0) *f+=360;
       int k,l;
       float rk,rl,p;
       p=(*f/90);
       k=int(p);
       l=int(p+1);
       rk=l-p;
       rl=1-rk;
       /*
       rl=sqr(Math::sin(rl*pi/2));
       rk=sqr(Math::sin(rk*pi/2));
       */
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
  
}
void Rotor::setEngineOn(int value)
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
int Rotor::numRotorblades()
{
    return _rotorblades.size();
}

Rotorblade* Rotor::getRotorblade(int n)
{
    return ((Rotorblade*)_rotorblades.get(n));
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
    for(i=0; i<3; i++) { _normal[i] = normal[i]*invsum; }
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
  //printf("SetAlphaoutput %i [%s]\n",i,text);
  strncpy(_alphaoutput[i],text,255);
}
void Rotor::setName(const char *text)
{
  strncpy(_name,text,128);//128: some space needed for settings
}

void Rotor::setCcw(int ccw)
{
     _ccw=ccw;
}
void Rotor::setNotorque(int value)
{
   _no_torque=value;
}
void Rotor::setSimBlades(int value)
{
   _sim_blades=value;
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




void Rotor::setCollective(float lval)
{
    lval = Math::clamp(lval, -1, 1);
    int i;
    //printf("col: %5.3f\n",lval);
    for(i=0; i<_rotorparts.size(); i++) {
        ((Rotorpart*)_rotorparts.get(i))->setCollective(lval);
        
    }
    float col=_min_pitch+(lval+1)/2*(_max_pitch-_min_pitch);
    for(i=0; i<_rotorblades.size(); i++) {
        ((Rotorblade*)_rotorblades.get(i))->setCollective(col);
        
    }
}
void Rotor::setCyclicele(float lval,float rval)
{
    rval = Math::clamp(rval, -1, 1);
    lval = Math::clamp(lval, -1, 1);
    float col=_mincyclicele+(lval+1)/2*(_maxcyclicele-_mincyclicele);
    int i;
    for(i=0; i<_rotorblades.size(); i++) {
        //((Rotorblade*)_rotorblades.get(i))->setCyclicele(col*Math::sin(((Rotorblade*)_rotorblades.get(i))->getPhi()));
        ((Rotorblade*)_rotorblades.get(i))->setCyclicele(col);
    }
    if (_rotorparts.size()!=4) return;
    //printf("                     ele: %5.3f  %5.3f\n",lval,rval);
    ((Rotorpart*)_rotorparts.get(1))->setCyclic(lval);
    ((Rotorpart*)_rotorparts.get(3))->setCyclic(-lval);
}
void Rotor::setCyclicail(float lval,float rval)
{
    lval = Math::clamp(lval, -1, 1);
    rval = Math::clamp(rval, -1, 1);
    float col=_mincyclicail+(lval+1)/2*(_maxcyclicail-_mincyclicail);
    int i;
    for(i=0; i<_rotorblades.size(); i++) {
        ((Rotorblade*)_rotorblades.get(i))->setCyclicail(col);
    }
    if (_rotorparts.size()!=4) return;
    //printf("ail: %5.3f %5.3f\n",lval,rval);
    if (_ccw) lval *=-1;
    ((Rotorpart*)_rotorparts.get(0))->setCyclic(-lval);
    ((Rotorpart*)_rotorparts.get(2))->setCyclic( lval);
}


float Rotor::getGroundEffect(float* posOut)
{
    /*
    int i;
    for(i=0; i<3; i++) posOut[i] = _base[i];
    float span = _length * Math::cos(_sweep) * Math::cos(_dihedral);
    span = 2*(span + Math::abs(_base[2]));
    */
    return _diameter;
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
  
  if(!_sim_blades)
  {
    _dynamic=_dynamic*(1/                          //inverse of the time
             ( (60/_rotor_rpm)/4         //for rotating 90 deg
              +(60/_rotor_rpm)/(2*_number_of_blades) //+ meantime a rotorblade will pass a given point 
             ));
    float directions[5][3];//pointing forward, right, ... the 5th is ony for calculation
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
    float rotorpartmass = _weight_per_blade*_number_of_blades/4*.453;//was pounds -> now kg
    float speed=_rotor_rpm/60*_diameter*_rel_blade_center*pi;
    float lentocenter=_diameter*_rel_blade_center*0.5;
    float lentoforceattac=_diameter*_rel_len_hinge*0.5;
    float zentforce=rotorpartmass*speed*speed/lentocenter;
    _force_at_max_pitch=_force_at_pitch_a/_pitch_a*_max_pitch;
    float maxpitchforce=_force_at_max_pitch/4*.453*9.81;//was pounds of force, now N
    float torque0=0,torquemax=0;
    float omega=_rotor_rpm/60*2*pi;
    _omegan=omega;
    float omega0=omega*Math::sqrt(1/(1-_rel_len_hinge));
    //float omega0=omega*Math::sqrt((1-_rel_len_hinge));
    //_delta=omega*_delta;
    _delta*=maxpitchforce/(_max_pitch*omega*lentocenter*2*rotorpartmass);

    float phi=Math::atan2(2*omega*_delta,omega0*omega0-omega*omega);
    //float relamp=omega*omega/(2*_delta*Math::sqrt(omega0*omega0-_delta*_delta));
    float relamp=omega*omega/(2*_delta*Math::sqrt(sqr(omega0*omega0-omega*omega)+4*_delta*_delta*omega*omega));
    if (!_no_torque)
    {
       torque0=_power_at_pitch_0/4*1000/omega;
       torquemax=_power_at_pitch_b/4*1000/omega/_pitch_b*_max_pitch;

       if(_ccw)
       {
         torque0*=-1;
         torquemax*=-1;
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
           "zf: " << setprecision(8) << zentforce
           << " mpf: " << maxpitchforce);
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
         Math::mul3(lentoforceattac,directions[i+1],lforceattac);//i+1: +90deg (gyro)!!!
         Math::add3(lforceattac,_base,lforceattac);
         Math::mul3(speed,directions[i+1],lspeed);
         Math::mul3(1,directions[i+1],dirzentforce);

         float maxcyclic=(i&1)?_maxcyclicele:_maxcyclicail;
         float mincyclic=(i&1)?_mincyclicele:_mincyclicail;
            

         Rotorpart* rp=rps[i]=newRotorpart(lpos, lforceattac, _normal,
                       lspeed,dirzentforce,zentforce,maxpitchforce, _max_pitch,_min_pitch,mincyclic,maxcyclic,
                       _delta3,rotorpartmass,_translift,_rel_len_hinge,lentocenter);
         rp->setAlphaoutput(_alphaoutput[i&1?i:(_ccw?i^2:i)],0);
         rp->setAlphaoutput(_alphaoutput[4+(i&1?i:(_ccw?i^2:i))],1+(i>1));
         _rotorparts.add(rp);
         rp->setTorque(torquemax,torque0);
         rp->setRelamp(relamp);

         
    }
    for (i=0;i<4;i++)
    {
      
      rps[i]->setlastnextrp(rps[(i+3)%4],rps[(i+1)%4],rps[(i+2)%4]);
    }
  }
  else
  {
    float directions[5][3];//pointing forward, right, ... the 5th is ony for calculation
    directions[0][0]=_forward[0];
    directions[0][1]=_forward[1];
    directions[0][2]=_forward[2];
    int i;
    SG_LOG(SG_FLIGHT, SG_DEBUG, "Rotor rotating ccw? " <<_ccw);
    for (i=1;i<5;i++)

    {
         //if (!_ccw)
           Math::cross3(directions[i-1],_normal,directions[i]);
         //else
         //  Math::cross3(_normal,directions[i-1],directions[i]);
         Math::unit3(directions[i],directions[i]);
    }
    Math::set3(directions[4],directions[0]);
    float speed=_rotor_rpm/60*_diameter*_rel_blade_center*pi;
    float lentocenter=_diameter*_rel_blade_center*0.5;
    float lentoforceattac=_diameter*_rel_len_hinge*0.5;
    float zentforce=_weight_per_blade*.453*speed*speed/lentocenter;
    _force_at_max_pitch=_force_at_pitch_a/_pitch_a*_max_pitch;
    float maxpitchforce=_force_at_max_pitch/_number_of_blades*.453*9.81;//was pounds of force, now N
    float torque0=0,torquemax=0;
    float omega=_rotor_rpm/60*2*pi;
    _omegan=omega;
    float omega0=omega*Math::sqrt(1/(1-_rel_len_hinge));
    //float omega0=omega*Math::sqrt(1-_rel_len_hinge);
    //_delta=omega*_delta;
    _delta*=maxpitchforce/(_max_pitch*omega*lentocenter*2*_weight_per_blade*.453);
    float phi=Math::atan2(2*omega*_delta,omega0*omega0-omega*omega);
    // float phi2=Math::abs(omega0-omega)<.000000001?pi/2:Math::atan(2*omega*_delta/(omega0*omega0-omega*omega));
    float relamp=omega*omega/(2*_delta*Math::sqrt(sqr(omega0*omega0-omega*omega)+4*_delta*_delta*omega*omega));
    if (!_no_torque)
    {
       torque0=_power_at_pitch_0/_number_of_blades*1000/omega;
       torquemax=_power_at_pitch_b/_number_of_blades*1000/omega/_pitch_b*_max_pitch;

       if(_ccw)
       {
         torque0*=-1;
         torquemax*=-1;
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
           "zf: " << setprecision(8) << zentforce
           << " mpf: " << maxpitchforce);
    SG_LOG(SG_FLIGHT, SG_DEBUG,
           "tq: " << setprecision(8) << torque0 << ".." << torquemax
           << " d3: " << _delta3);
    SG_LOG(SG_FLIGHT, SG_DEBUG,
           "o/o0: " << setprecision(8) << omega/omega0
           << " phi: " << phi*180/pi
           << " relamp: " << relamp
           << " delta: " <<_delta);

    // float lspeed[3];
    float dirzentforce[3];

    float f=(!_ccw)?1:-1;
    //Math::mul3(f*speed,directions[1],lspeed);
    Math::mul3(f,directions[1],dirzentforce);
    for (i=0;i<_number_of_blades;i++)
    {

            


         Rotorblade* rb=newRotorblade(_base,_normal,directions[0],directions[1],
                       lentoforceattac,_rel_len_hinge,
                       dirzentforce,zentforce,maxpitchforce, _max_pitch,
                       _delta3,_weight_per_blade*.453,_translift,2*pi/_number_of_blades*i,
                       omega,lentocenter,/*f* */speed);
         //rp->setAlphaoutput(_alphaoutput[i&1?i:(_ccw?i^2:i)],0);
         //rp->setAlphaoutput(_alphaoutput[4+(i&1?i:(_ccw?i^2:i))],1+(i>1));
         _rotorblades.add(rb);
         rb->setTorque(torquemax,torque0);
         rb->setDeltaPhi(pi/2.-phi);
         rb->setDelta(_delta);

         rb->calcFrontRight();
         
    }
    /*
    for (i=0;i<4;i++)
    {
      
      rps[i]->setlastnextrp(rps[(i-1)%4],rps[(i+1)%4],rps[(i+2)%4]);
    }
    */

  }
}


Rotorblade* Rotor::newRotorblade(float* pos,  float *normal, float *front, float *right,
         float lforceattac,float rellenhinge,
         float *dirzentforce, float zentforce,float maxpitchforce,float maxpitch, 
         float delta3,float mass,float translift,float phi,float omega,float len,float speed)
{
    Rotorblade *r = new Rotorblade();
    r->setPosition(pos);
    r->setNormal(normal);
    r->setFront(front);
    r->setRight(right);
    r->setMaxPitchForce(maxpitchforce);
    r->setZentipetalForce(zentforce);
    r->setAlpha0(_alpha0);
    r->setAlphamin(_alphamin);
    r->setAlphamax(_alphamax);
    r->setAlpha0factor(_alpha0factor);



    r->setSpeed(speed);
    r->setDirectionofZentipetalforce(dirzentforce);
    r->setMaxpitch(maxpitch);
    r->setDelta3(delta3);
    r->setDynamic(_dynamic);
    r->setTranslift(_translift);
    r->setC2(_c2);
    r->setStepspersecond(_stepspersecond);
    r->setWeight(mass);
    r->setOmegaN(omega);
    r->setPhi(phi);
    r->setLforceattac(lforceattac);
    r->setLen(len);
    r->setLenHinge(rellenhinge);
    r->setRelLenTeeterHinge(_rellenteeterhinge);
    r->setTeeterdamp(_teeterdamp);
    r->setMaxteeterdamp(_maxteeterdamp);

    /*
    #define a(x) x[0],x[1],x[2]
    printf("newrp: pos(%5.3f %5.3f %5.3f) pfa (%5.3f %5.3f %5.3f)\n"
           "       nor(%5.3f %5.3f %5.3f) spd (%5.3f %5.3f %5.3f)\n"
           "       dzf(%5.3f %5.3f %5.3f) zf  (%5.3f) mpf (%5.3f)\n"
           "       pit (%5.3f..%5.3f) mcy (%5.3f..%5.3f) d3 (%5.3f)\n"
           ,a(pos),a(posforceattac),a(normal),
          a(speed),a(dirzentforce),zentforce,maxpitchforce,minpitch,maxpitch,mincyclic,maxcyclic,
          delta3);
    #undef a
    */
    return r;
}

Rotorpart* Rotor::newRotorpart(float* pos, float *posforceattac, float *normal,
         float* speed,float *dirzentforce, float zentforce,float maxpitchforce,
         float maxpitch, float minpitch, float mincyclic,float maxcyclic,
         float delta3,float mass,float translift,float rellenhinge,float len)
{
    Rotorpart *r = new Rotorpart();
    r->setPosition(pos);
    r->setNormal(normal);
    r->setMaxPitchForce(maxpitchforce);
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

}; // namespace yasim
 
