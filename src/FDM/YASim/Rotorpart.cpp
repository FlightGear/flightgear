#include "Math.hpp"
#include "Rotorpart.hpp"
#include <stdio.h>
//#include <string.h>
//#include <Main/fg_props.hxx>
namespace yasim {
const float pi=3.14159;

Rotorpart::Rotorpart()
{
    _cyclic=0;
    _collective=0;
    _rellenhinge=0;
    _dt=0;
    #define set3(x,a,b,c) x[0]=a;x[1]=b;x[2]=c;
    set3 (_speed,1,0,0);
    set3 (_directionofzentipetalforce,1,0,0);
    #undef set3
    _zentipetalforce=1;
    _maxpitch=.02;
    _minpitch=0;
    _maxpitchforce=10;
    _maxcyclic=0.02;
    _mincyclic=-0.02;
    _delta3=0.5;
    _cyclic=0;
    _collective=0;
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
    _phi=0;
    _len=1;




}


void Rotorpart::inititeration(float dt,float *rot)
{
     //printf("init %5.3f",dt*1000);
     _dt=dt;
     _phi+=_omega*dt;
     while (_phi>(2*pi)) _phi-=2*pi;
     while (_phi<(0   )) _phi+=2*pi;

     //_alphaalt=_alpha;
     //a=skalarprdukt _normal mit rot ergibt drehung um Normale
     //alphaalt=Math::cos(a)*alpha+.5*(Math::sin(a)*alphanachbarnlast-Math::sin(a)*alphanachbanext)
     float a=Math::dot3(rot,_normal);
     if(a>0)
        _alphaalt=_alpha*Math::cos(a)
                  +_nextrp->getrealAlpha()*Math::sin(a);
      else
        _alphaalt=_alpha*Math::cos(a)
                  +_lastrp->getrealAlpha()*Math::sin(-a);
     //jetzt noch Drehung des Rumpfes in gleiche Richtung wie alpha bestimmen
     //und zu _alphaalt hinzufÅgen
     //alpha gibt drehung um normal cross dirofzentf an
     
     float dir[3];
     Math::cross3(_directionofzentipetalforce,_normal,dir);
     


     a=Math::dot3(rot,dir);
     _alphaalt -= a;

     _alphaalt= Math::clamp(_alphaalt,_alphamin,_alphamax);


}
void Rotorpart::setTorque(float torque_max_force,float torque_no_force)
{
    _torque_max_force=torque_max_force;
    _torque_no_force=torque_no_force;
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
    //printf("posforce: %5.3f %5.3f %5.3f ",out[0],out[1],out[2]);
}
void Rotorpart::setSpeed(float* p)
{
    int i;
    for(i=0; i<3; i++) _speed[i] = p[i];
}
void Rotorpart::setDirectionofZentipetalforce(float* p)
{
    int i;
    for(i=0; i<3; i++) _directionofzentipetalforce[i] = p[i];
}

void Rotorpart::setOmega(float value)
{
  _omega=value;
}
void Rotorpart::setOmegaN(float value)
{
  _omegan=value;
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
void Rotorpart::setMaxPitchForce(float f)
{
    _maxpitchforce=f;
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
    if (_alpha2type==1) //yaw or roll
      return (getAlpha(0)-_oppositerp->getAlpha(0))/2;
     else //pitch
      return (getAlpha(0)+_oppositerp->getAlpha(0)+
              _nextrp->getAlpha(0)+_lastrp->getAlpha(0))/4;

}
float Rotorpart::getrealAlpha(void)
{
    return _alpha;
}
void Rotorpart::setAlphaoutput(char *text,int i)
{
   printf("setAlphaoutput rotorpart [%s] typ %i\n",text,i);

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

void Rotorpart::setlastnextrp(Rotorpart*lastrp,Rotorpart*nextrp,Rotorpart *oppositerp)
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

// Calculate the aerodynamic force given a wind vector v (in the
// aircraft's "local" coordinates) and an air density rho.  Returns a
// torque about the Y axis, too.
void Rotorpart::calcForce(float* v, float rho, float* out, float* torque)
{
    {
        _zentipetalforce=_mass*_len*_omega*_omega;
        float vrel[3],vreldir[3];
        Math::sub3(_speed,v,vrel);
        Math::unit3(vrel,vreldir);//direction of blade-movement rel. to air
        float delta=Math::asin(Math::dot3(_normal,vreldir));//Angle of blade which would produce no vertical force

        float cyc=_mincyclic+(_cyclic+1)/2*(_maxcyclic-_mincyclic);
        float col=_minpitch+(_collective+1)/2*(_maxpitch-_minpitch);
        //printf("[%5.3f]",col);
        _incidence=(col+cyc)-_delta3*_alphaalt;
        cyc*=_relamp;
        float beta=cyc+col;
        
        //float c=_maxpitchforce/(_maxpitch*_zentipetalforce);
        float c,alpha,factor;
        if((_omega*10)>_omegan)
        {
          c=_maxpitchforce/_omegan/(_maxpitch*_mass*_len*_omega);
          alpha = c*(beta-delta)/(1+_delta3*c);

          factor=_dt*_dynamic;
          if (factor>1) factor=1;
        }
        else
        {
          alpha=_alpha0;
          factor=_dt*_dynamic/10;
          if (factor>1) factor=1;
        }

        float vz=Math::dot3(_normal,v);
        //alpha+=_c2*vz;
        
        float fcw;
        if(_c2==0)
          fcw==0;
        else
          //fcw=vz/_c2*_maxpitchforce*_omega/_omegan;
          fcw=vz*(_c2-1)*_maxpitchforce*_omega/(_omegan*_omegan*_len*_maxpitch);

        float dirblade[3];
        Math::cross3(_normal,_directionofzentipetalforce,dirblade);
        float vblade=Math::abs(Math::dot3(dirblade,v));
        float tliftfactor=Math::sqrt(1+vblade*_translift);


        alpha=_alphaalt+(alpha-_alphaalt)*factor;
        //alpha=_alphaalt+(alpha-_lastrp->getrealAlpha())*factor;


        _alpha=alpha;


        //float schwenkfactor=1;//  1/Math::cos(_lastrp->getrealAlpha());

        float meancosalpha=(1*Math::cos(_lastrp->getrealAlpha())
                        +1*Math::cos(_nextrp->getrealAlpha())
                        +1*Math::cos(_oppositerp->getrealAlpha())
                        +1*Math::cos(alpha))/4;
        float schwenkfactor=1-(Math::cos(_lastrp->getrealAlpha())-meancosalpha);


        //fehlt: verringerung um rellenhinge
        float xforce = /*schwenkfactor*/ Math::cos(alpha)*_zentipetalforce;// /9.81*.453; //N->poundsofforce
        float zforce = fcw+tliftfactor*schwenkfactor*Math::sin(alpha)*_zentipetalforce;// /9.81*.453;

        float thetorque = _torque_no_force+_torque_max_force*Math::abs(zforce/_maxpitchforce);
        /*
        printf("speed: %5.3f %5.3f %5.3f vwind: %5.3f %5.3f %5.3f sin %5.3f\n",
          _speed[0],_speed[1],_speed[2],
          v[0],v[1],v[2],Math::sin(alpha));
        */

        int i;
        for(i=0; i<3; i++) {
          torque[i] = _normal[i]*thetorque;
          out[i] = _normal[i]*zforce+_directionofzentipetalforce[i]*xforce;
        }
        //printf("alpha: %5.3f force: %5.3f %5.3f %5.3f %5.3f %5.3f\n",alpha*180/3.14,xforce,zforce,out[0],out[1],out[2]);
	return;
    }
}


}; // namespace yasim
