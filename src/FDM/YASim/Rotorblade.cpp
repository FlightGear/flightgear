#include "Math.hpp"
#include "Rotorblade.hpp"
#include <stdio.h>
//#include <string.h>
//#include <Main/fg_props.hxx>
namespace yasim {
const float pi=3.14159;

Rotorblade::Rotorblade()
{
    /*
    _orient[0] = 1; _orient[1] = 0; _orient[2] = 0;
    _orient[3] = 0; _orient[4] = 1; _orient[5] = 0;
    _orient[6] = 0; _orient[7] = 0; _orient[8] = 1;
    */
    _collective=0;
    _dt=0;
    _speed=0;
    #define set3(x,a,b,c) x[0]=a;x[1]=b;x[2]=c;
    set3 (_directionofzentipetalforce,1,0,0);
    #undef set3
    _zentipetalforce=1;
    _maxpitch=.02;
    _maxpitchforce=10;
    _delta3=0.5;
    _cyclicail=0;
    _cyclicele=0;
    _collective=0;
    _flapatpos[0]=_flapatpos[1]=_flapatpos[2]=_flapatpos[3]=0;
    _flapatpos[0]=.1;
    _flapatpos[1]=.2;
    _flapatpos[2]=.3;
    _flapatpos[3]=.4;
    _len=1;
    _lforceattac=1;
    _calcforcesdone=false;
    _phi=0;
    _omega=0;
    _omegan=1;
    _mass=10;
    _alpha=0;
    _alphaoutputbuf[0][0]=0;
    _alphaoutputbuf[1][0]=0;
    _alpha2type=0;
    _alphaalt=0;
    _alphaomega=0;
    _lastrp=0;
    _nextrp=0;
    _oppositerp=0;
    _translift=0;
    _dynamic=100;
    _c2=1;
    _stepspersecond=240;
    _torque_max_force=0;
    _torque_no_force=0;
    _deltaphi=0;
    _alphamin=-.1;
    _alphamax= .1;
    _alpha0=-.05;
    _alpha0factor=1;
    _rellenhinge=0;
    _teeter=0;
    _ddtteeter=0;
    _teeterdamp=0.00001;
    _maxteeterdamp=0;
    _rellenteeterhinge=0.01;





}


void Rotorblade::inititeration(float dt,float *rot)
{
     //printf("init %5.3f",dt*1000);
     _dt=dt;
     _calcforcesdone=false;
     float a=Math::dot3(rot,_normal);
   _phi+=a;
     _phi+=_omega*dt;
     while (_phi>(2*pi)) _phi-=2*pi;
     while (_phi<(0   )) _phi+=2*pi;
     //jetzt noch Drehung des Rumpfes in gleiche Richtung wie alpha bestimmen
     //und zu _alphaalt hinzufgen
     //alpha gibt drehung um normal cross dirofzentf an
     
     float dir[3];
     Math::cross3(_lright,_normal,dir);
     


     a=-Math::dot3(rot,dir);
   float alphaneu= _alpha+a;
     // alphaneu= Math::clamp(alphaneu,-.5,.5);
     //_alphaomega=(alphaneu-_alphaalt)/_dt;//now calculated in calcforces

     _alphaalt = alphaneu;


     calcFrontRight();
}
void Rotorblade::setTorque(float torque_max_force,float torque_no_force)
{
    _torque_max_force=torque_max_force;
    _torque_no_force=torque_no_force;
}
void Rotorblade::setAlpha0(float f)
{
    _alpha0=f;
}
void Rotorblade::setAlphamin(float f)
{
    _alphamin=f;
}
void Rotorblade::setAlphamax(float f)
{
    _alphamax=f;
}
void Rotorblade::setAlpha0factor(float f)
{
    _alpha0factor=f;
}




void Rotorblade::setWeight(float value)
{
     _mass=value;
}
float Rotorblade::getWeight(void)
{
     return(_mass/.453); //_mass is in kg, returns pounds 
}

void Rotorblade::setPosition(float* p)
{
    int i;
    for(i=0; i<3; i++) _pos[i] = p[i];
}

void Rotorblade::calcFrontRight()
{
    float tmpcf[3],tmpsr[3],tmpsf[3],tmpcr[3];
    Math::mul3(Math::cos(_phi),_right,tmpcr);
    Math::mul3(Math::cos(_phi),_front,tmpcf);
    Math::mul3(Math::sin(_phi),_right,tmpsr);
    Math::mul3(Math::sin(_phi),_front,tmpsf);

    Math::add3(tmpcf,tmpsr,_lfront);
    Math::sub3(tmpcr,tmpsf,_lright);

}

void Rotorblade::getPosition(float* out)
{
    float dir[3];
    Math::mul3(_len,_lfront,dir);
    Math::add3(_pos,dir,out);
}

void Rotorblade::setPositionForceAttac(float* p)
{
    int i;
    for(i=0; i<3; i++) _posforceattac[i] = p[i];
}

void Rotorblade::getPositionForceAttac(float* out)
{
    float dir[3];
    Math::mul3(_len*_rellenhinge*2,_lfront,dir);
    Math::add3(_pos,dir,out);
}
void Rotorblade::setSpeed(float p)
{
    _speed = p;
}
void Rotorblade::setDirectionofZentipetalforce(float* p)
{
    int i;
    for(i=0; i<3; i++) _directionofzentipetalforce[i] = p[i];
}

void Rotorblade::setZentipetalForce(float f)
{
    _zentipetalforce=f;
} 
void Rotorblade::setMaxpitch(float f)
{
    _maxpitch=f;
} 
void Rotorblade::setMaxPitchForce(float f)
{
    _maxpitchforce=f;
} 
void Rotorblade::setDelta(float f)
{
    _delta=f;
} 
void Rotorblade::setDeltaPhi(float f)
{
    _deltaphi=f;
} 
void Rotorblade::setDelta3(float f)
{
    _delta3=f;
} 
void Rotorblade::setTranslift(float f)
{
    _translift=f;
}
void Rotorblade::setDynamic(float f)
{
    _dynamic=f;
}
void Rotorblade::setC2(float f)
{
    _c2=f;
}
void Rotorblade::setStepspersecond(float steps)
{
    _stepspersecond=steps;
}
void Rotorblade::setRelLenTeeterHinge(float f)
{
   _rellenteeterhinge=f;
}

void Rotorblade::setTeeterdamp(float f)
{
    _teeterdamp=f;
}
void Rotorblade::setMaxteeterdamp(float f)
{
    _maxteeterdamp=f;
}
float Rotorblade::getAlpha(int i)
{
  i=i&1;
  if ((i==0)&&(_first))
    return _alpha*180/3.14;//in Grad = 1
  else
    if(i==0)
      return _showa;
    else
      return _showb;

}
float Rotorblade::getrealAlpha(void)
{
    return _alpha;
}
void Rotorblade::setAlphaoutput(char *text,int i)
{
   printf("setAlphaoutput Rotorblade [%s] typ %i\n",text,i);

   strncpy(_alphaoutputbuf[i>0],text,255);

   if (i>0) _alpha2type=i;

                             
}
char* Rotorblade::getAlphaoutput(int i)
{
   #define wstep 30
    if ((i==0)&&(_first))
    {
       int winkel=(int)(.5+_phi/pi*180/wstep);
       winkel%=(360/wstep);
       sprintf(_alphaoutputbuf[0],"/blades/pos%03i",winkel*wstep);
    }
    
    else 
    {
    
       int winkel=(int)(.5+_phi/pi*180/wstep);
       winkel%=(360/wstep);
       if (i==0)
         sprintf(_alphaoutputbuf[i&1],"/blades/showa_%i_%03i",i,winkel*wstep);
       else
         if (_first)
           sprintf(_alphaoutputbuf[i&1],"/blades/damp_%03i",winkel*wstep);
         else
           sprintf(_alphaoutputbuf[i&1],"/blades/showb_%i_%03i",i,winkel*wstep);
    

    }
    return _alphaoutputbuf[i&1];
  #undef wstep
}

void Rotorblade::setNormal(float* p)
{
    int i;
    for(i=0; i<3; i++) _normal[i] = p[i];
}
void Rotorblade::setFront(float* p)
{
    int i;
    for(i=0; i<3; i++) _lfront[i]=_front[i] = p[i];
    printf("front: %5.3f %5.3f %5.3f\n",p[0],p[1],p[2]);
}
void Rotorblade::setRight(float* p)
{
    int i;
    for(i=0; i<3; i++) _lright[i]=_right[i] = p[i];
    printf("right: %5.3f %5.3f %5.3f\n",p[0],p[1],p[2]);
}

void Rotorblade::getNormal(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _normal[i];
}


void Rotorblade::setCollective(float pos)
{
    _collective = pos;
}

void Rotorblade::setCyclicele(float pos)
{
    _cyclicele = -pos;
}
void Rotorblade::setCyclicail(float pos)
{
    _cyclicail = -pos;
}


void Rotorblade::setPhi(float value)
{
  _phi=value;
  if(value==0) _first=1; else _first =0;
}
float Rotorblade::getPhi()
{
  return(_phi);
}
void Rotorblade::setOmega(float value)
{
  _omega=value;
}
void Rotorblade::setOmegaN(float value)
{
  _omegan=value;
}
void Rotorblade::setLen(float value)
{
  _len=value;
}
void Rotorblade::setLenHinge(float value)
{
  _rellenhinge=value;
}
void Rotorblade::setLforceattac(float value)
{
  _lforceattac=value;
}
float Rotorblade::getIncidence()
{
    return(_incidence);
}

float Rotorblade::getFlapatPos(int k)
{
   return  _flapatpos[k%4];
}



/*
void Rotorblade::setlastnextrp(Rotorblade*lastrp,Rotorblade*nextrp,Rotorblade *oppositerp)
{
  _lastrp=lastrp;
  _nextrp=nextrp;
  _oppositerp=oppositerp;
}
*/

void Rotorblade::strncpy(char *dest,const char *src,int maxlen)
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
void Rotorblade::calcForce(float* v, float rho, float* out, float* torque)
{

    //printf("cf: alt:%g aw:%g ",_alphaalt,_alphaomega);
    //if (_first) printf("p: %5.3f e:%5.3f a:%5.3f p:%5.3f",_collective,_cyclicele,_cyclicail,_phi);
    if (_calcforcesdone)
    {
        int i;
        for(i=0; i<3; i++) {
          torque[i] = _oldt[i];
          out[i] = _oldf[i];
        }
        return;
    }

    {
      int k;
      if (_omega>0)
          for (k=1;k<=4;k++)
          {
             if ((_phi<=(float(k)*pi/2))&&((_phi+_omega*_dt)>=(float(k)*pi/2)))
             {
                 _flapatpos[k%4]=_alphaalt;
             }
          }
       else
          for (k=0;k<4;k++)
          {
             if ((_phi>=(float(k)*pi/2))&&((_phi+_omega*_dt)<=(float(k)*pi/2)))
             {
                 _flapatpos[k%4]=_alphaalt;
             }
          }
    }

    float ldt;
    int steps=int(_dt*_stepspersecond);
    if (steps<=0) steps=1;
    ldt=_dt/steps;
    float lphi;
    float f[3];
    f[0]=f[1]=f[2]=0;
    float t[3];
    t[0]=t[1]=t[2]=0;

    //_zentipetalforce=_mass*_omega*_omega*_len*(_rellenhinge+(1-_rellenhinge)*Math::cos(_alphalt));
    //_zentipetalforce=_mass*_omega*_omega*_len/(_rellenhinge+(1-_rellenhinge)*Math::cos(_alphalt)); //incl teeter
    _speed=_omega*_len*(1-_rellenhinge+_rellenhinge*Math::cos(_alphaalt));

    float vrel[3],vreldir[3],speed[3];
    Math::mul3(_speed,_lright,speed);
    Math::sub3(speed,v,vrel);
    Math::unit3(vrel,vreldir);//direction of blade-movement rel. to air
    float delta=Math::asin(Math::dot3(_normal,vreldir));//Angle of blade which would produce no vertical force
    float lalphaalt=_alphaalt;
    float lalpha=_alpha;
    float lalphaomega=_alphaomega;
    if((_phi>0.01)&&(_first)&&(_phi<0.02))
    {
      printf("mass:%5.3f delta: %5.3f _dt: %5.7f ldt: %5.7f st:%i w: %5.3f w0: %5.3f\n",
             _mass,_delta,_dt,ldt,steps,_omega,Math::sqrt(_zentipetalforce*(1-_rellenhinge)/_len/_mass));   
    }
    for (int step=0;step<steps;step++)
    {
       lphi=_phi+(step-steps/2.)*ldt*_omega;
        //_zentipetalforce=_mass*_omega*_omega*_len/(_rellenhinge+(1-_rellenhinge)*Math::cos(lalphaalt)); //incl teeter
        _zentipetalforce=_mass*_omega*_omega*_len; 
        //printf("[%5.3f]",col);
        float beta=-_cyclicele*Math::sin(lphi-0*_deltaphi)+_cyclicail*Math::cos(lphi-0*_deltaphi)+_collective-_delta3*lalphaalt;
        if (step==(steps/2)) _incidence=beta;
        //printf("be:%5.3f de:%5.3f ",beta,delta);
        //printf("\nvd: %5.3f %5.3f %5.3f ",vreldir[0],vreldir[1],vreldir[2]);
        //printf("lr: %5.3f %5.3f %5.3f\n",_lright[0],_lright[1],_lright[2]);
        //printf("no: %5.3f %5.3f %5.3f ",_normal[0],_normal[1],_normal[2]);
        //printf("sp: %5.3f %5.3f %5.3f\n ",speed[0],speed[1],speed[2]);
        //printf("vr: %5.3f %5.3f %5.3f ",vrel[0],vrel[1],vrel[2]);
        //printf("v : %5.3f %5.3f %5.3f ",v[0],v[1],v[2]);
        
        //float c=_maxpitchforce/(_maxpitch*_zentipetalforce);

        float zforcealph=(beta-delta)/_maxpitch*_maxpitchforce*_omega/_omegan;
        float zforcezent=(1-_rellenhinge)*Math::sin(lalphaalt)*_zentipetalforce;
        float zforcelowspeed=(_omegan-_omega)/_omegan*(lalpha-_alpha0)*_mass*_alpha0factor;
        float zf=zforcealph-zforcezent-zforcelowspeed;
        _showa=zforcealph;
        _showb=-zforcezent;
        float vv=Math::sin(lalphaomega)*_len;
        zf-=vv*_delta*2*_mass;
        vv+=zf/_mass*ldt;
        if ((_omega*10)<_omegan)
          vv*=.5+5*(_omega/_omegan);//reduce if omega is low
        //if (_first) _showb=vv*_delta*2*_mass;//for debugging output
        lalpha=Math::asin(Math::sin(lalphaalt)+(vv/_len)*ldt);
        lalpha=Math::clamp(lalpha,_alphamin,_alphamax);
        float vblade=Math::abs(Math::dot3(_lfront,v));
        float tliftfactor=Math::sqrt(1+vblade*_translift);




        float xforce = Math::cos(lalpha)*_zentipetalforce;// 
        float zforce = tliftfactor*Math::sin(lalpha)*_zentipetalforce;// 
        float thetorque = _torque_no_force+_torque_max_force*Math::abs(zforce/_maxpitchforce);
        /*
        printf("speed: %5.3f %5.3f %5.3f vwind: %5.3f %5.3f %5.3f sin %5.3f\n",
          _speed[0],_speed[1],_speed[2],
          v[0],v[1],v[2],Math::sin(alpha));
        */
        int i;
        for(i=0; i<3; i++) {
          t[i] += _normal[i]*thetorque;
          f[i] += _normal[i]*zforce+_lfront[i]*xforce;
        }
        lalphaomega=(lalpha-lalphaalt)/ldt;
        lalphaalt=lalpha;

        /*
        _ddtteeter+=_len*_omega/(1-_rellenhinge)*lalphaomega*ldt;
        
        float teeterforce=-_zentipetalforce*Math::sin(_teeter)*_c2;
        teeterforce-=Math::clamp(_ddtteeter*_teeterdamp,-_maxteeterdamp,_maxteeterdamp);
        _ddtteeter+=teeterforce/_mass;
        
        _teeter+=_ddtteeter*ldt;
        if (_first)  _showb=_teeter*180/pi;
        */
    }
    _alpha=lalpha;
    _alphaomega=lalphaomega;
    /*
        if (_first) printf("aneu: %5.3f zfa:%5.3f vv:%g ao:%.3g xf:%.3g zf:%.3g    \r",_alpha,zforcealph,vv,_alpha
                          ,xforce,zforce);
    */
    int i;
    for(i=0; i<3; i++) {
          torque[i] = _oldt[i]=t[i]/steps;
          out[i] = _oldf[i]=f[i]/steps;
    }
    _calcforcesdone=true;
        //printf("alpha: %5.3f force: %5.3f %5.3f %5.3f %5.3f %5.3f\n",alpha*180/3.14,xforce,zforce,out[0],out[1],out[2]);
}


}; // namespace yasim
