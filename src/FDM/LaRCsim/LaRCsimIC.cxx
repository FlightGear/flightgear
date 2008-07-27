/*******************************************************************************
 
 Header:       LaRCsimIC.cxx
 Author:       Tony Peden
 Date started: 10/9/99
 
 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------
 
 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.
 
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.
*/ 
 

#include <simgear/compiler.h>

#include <math.h>
#include <iostream>

#include "ls_cockpit.h"
#include "ls_generic.h"
#include "ls_interface.h"
#include "atmos_62.h"
#include "ls_constants.h"
#include "ls_geodesy.h"

#include "LaRCsimIC.hxx"

using std::cout;
using std::endl;


LaRCsimIC::LaRCsimIC(void) {
  vt=vtg=vw=vc=ve=0;
  mach=0;
  alpha=beta=gamma=0;
  theta=phi=psi=0;
  altitude=runway_altitude=hdot=alt_agl=0;
  latitude_gd=latitude_gc=longitude=0;
  u=v=w=0;  
  vnorth=veast=vdown=0;
  vnorthwind=veastwind=vdownwind=0;
  lastSpeedSet=lssetvt;
  lastAltSet=lssetasl;
  get_atmosphere();
  ls_geod_to_geoc( latitude_gd,altitude,&sea_level_radius,&latitude_gc); 
  
}


LaRCsimIC::~LaRCsimIC(void) {}

void LaRCsimIC::get_atmosphere(void) {
  Altitude=altitude; //set LaRCsim variable
  ls_atmos(Altitude, &density_ratio, &soundspeed, &T, &p);
  rho=density_ratio*SEA_LEVEL_DENSITY;
}  

void LaRCsimIC::SetVcalibratedKtsIC( SCALAR tt) {
    vc=tt*KTS_TO_FPS;
    lastSpeedSet=lssetvc;
    vt=sqrt(1/density_ratio*vc*vc);
}

void LaRCsimIC::SetVtrueFpsIC( SCALAR tt) {
  vt=tt;
  lastSpeedSet=lssetvt;
}

void LaRCsimIC::SetMachIC( SCALAR tt) {
  mach=tt;
  vt=mach*soundspeed;
  lastSpeedSet=lssetmach;
}

void LaRCsimIC::SetVequivalentKtsIC(SCALAR tt) {
  ve=tt*KTS_TO_FPS;
  lastSpeedSet=lssetve;
  vt=sqrt(SEA_LEVEL_DENSITY/rho)*ve;
}  

void LaRCsimIC::SetClimbRateFpmIC( SCALAR tt) {
  SetClimbRateFpsIC(tt/60.0);
}

void LaRCsimIC::SetClimbRateFpsIC( SCALAR tt) {
  vtg=vt+vw;
  if(vtg > 0.1) {
    hdot = tt - vdownwind;
    gamma=asin(hdot/vtg);
    getTheta();
    vdown=-1*hdot;
  }
}

void LaRCsimIC::SetFlightPathAngleRadIC( SCALAR tt) {
  gamma=tt;
  getTheta();
  vtg=vt+vw;
  hdot=vtg*sin(tt);
  vdown=-1*hdot;
}

void LaRCsimIC::SetPitchAngleRadIC(SCALAR tt) { 
  if(alt_agl < (DEFAULT_AGL_ALT + 0.1) || vt < 10 ) 
    theta=DEFAULT_PITCH_ON_GROUND; 
  else
    theta=tt;  
  getAlpha();
}

void LaRCsimIC::SetUVWFpsIC(SCALAR vu, SCALAR vv, SCALAR vw) {
  u=vu; v=vv; w=vw;
  vt=sqrt(u*u+v*v+w*w);
  lastSpeedSet=lssetuvw;
}

  
void LaRCsimIC::SetVNEDAirmassFpsIC(SCALAR north, SCALAR east, SCALAR down ) {
  vnorthwind=north; veastwind=east; vdownwind=down;
  vw=sqrt(vnorthwind*vnorthwind + veastwind*veastwind + vdownwind*vdownwind);
  vtg=vt+vw;
  SetClimbRateFpsIC(-1*(vdown+vdownwind));
}  

void LaRCsimIC::SetAltitudeFtIC( SCALAR tt) {
  if(tt > (runway_altitude + DEFAULT_AGL_ALT)) {
    altitude=tt;
  } else {
    altitude=runway_altitude + DEFAULT_AGL_ALT;
    alt_agl=DEFAULT_AGL_ALT;
    theta=DEFAULT_PITCH_ON_GROUND; 
  }  
  lastAltSet=lssetasl;
  get_atmosphere();
  //lets try to make sure the user gets what they intended

  switch(lastSpeedSet) {
  case lssetned:
    calcVtfromNED();
    break;  
  case lssetuvw:
  case lssetvt:
    SetVtrueFpsIC(vt);
    break;
  case lssetvc:
    SetVcalibratedKtsIC(vc*V_TO_KNOTS);
    break;
  case lssetve:
    SetVequivalentKtsIC(ve*V_TO_KNOTS);
    break;
  case lssetmach:
    SetMachIC(mach);
    break;
  }
}

void LaRCsimIC::SetAltitudeAGLFtIC( SCALAR tt) {
  alt_agl=tt;
  lastAltSet=lssetagl;
  altitude=runway_altitude + alt_agl;
}

void LaRCsimIC::SetRunwayAltitudeFtIC( SCALAR tt) {
  runway_altitude=tt;
  if(lastAltSet == lssetagl) 
      altitude = runway_altitude + alt_agl;
}  
        
void LaRCsimIC::calcVtfromNED(void) {
  //take the earth's rotation out of veast first
  //float veastner = veast- OMEGA_EARTH*sea_level_radius*cos( latitude_gd );
  float veastner=veast;
  u = (vnorth - vnorthwind)*cos(theta)*cos(psi) + 
      (veastner - veastwind)*cos(theta)*sin(psi) - 
      (vdown - vdownwind)*sin(theta);
  v = (vnorth - vnorthwind)*(sin(phi)*sin(theta)*cos(psi) - cos(phi)*sin(psi)) +
      (veastner - veastwind)*(sin(phi)*sin(theta)*sin(psi) + cos(phi)*cos(psi)) +
      (vdown - vdownwind)*sin(phi)*cos(theta);
  w = (vnorth - vnorthwind)*(cos(phi)*sin(theta)*cos(psi) + sin(phi)*sin(psi)) +
      (veastner - veastwind)*(cos(phi)*sin(theta)*sin(psi) - sin(phi)*cos(psi)) +
      (vdown - vdownwind)*cos(phi)*cos(theta);
  vt = sqrt(u*u + v*v + w*w);
} 

void LaRCsimIC::calcNEDfromVt(void) {
  float veastner;
  
  //get the body components of vt
  u = GetUBodyFpsIC();
  v = GetVBodyFpsIC();   
  w = GetWBodyFpsIC();
  
  //transform them to local axes and add on the wind components
  vnorth = u*cos(theta)*cos(psi) +
           v*(sin(phi)*sin(theta)*cos(psi) - cos(phi)*sin(psi)) +
           w*(cos(phi)*sin(theta)*cos(psi) + sin(phi)*sin(psi)) +
           vnorthwind;
  
  //need to account for the earth's rotation here
  veastner = u*cos(theta)*sin(psi) +
             v*(sin(phi)*sin(theta)*sin(psi) + cos(phi)*cos(psi)) +
             w*(cos(phi)*sin(theta)*sin(psi) - sin(phi)*cos(psi)) +
             veastwind;
  veast = veastner;
  //veast = veastner + OMEGA_EARTH*sea_level_radius*cos( latitude_gd );  
  
  vdown =  u*sin(theta) +
           v*sin(phi)*cos(theta) +
           w*cos(phi)*cos(theta) +
           vdownwind;
}           

void LaRCsimIC::SetVNEDFpsIC( SCALAR north, SCALAR east, SCALAR down) {
  vnorth=north;
  veast=east;
  vdown=down;
  SetClimbRateFpsIC(-1*vdown);
  lastSpeedSet=lssetned;
  calcVtfromNED();
}        
  
void LaRCsimIC::SetLatitudeGDRadIC(SCALAR tt) {
  latitude_gd=tt;
  ls_geod_to_geoc(latitude_gd,altitude,&sea_level_radius, &latitude_gc);
}
  
bool LaRCsimIC::getAlpha(void) {
  bool result=false;
  float guess=theta-gamma;
  xlo=xhi=0;
  xmin=-89;
  xmax=89;
  sfunc=&LaRCsimIC::GammaEqOfAlpha;
  if(findInterval(0,guess)){
    if(solve(&alpha,0)){
      result=true;
    }
  }
  return result;
}      

      
bool LaRCsimIC::getTheta(void) {
  bool result=false;
  float guess=alpha+gamma;
  xlo=xhi=0;
  xmin=-89;xmax=89;
  sfunc=&LaRCsimIC::GammaEqOfTheta;
  if(findInterval(0,guess)){
    if(solve(&theta,0)){
      result=true;
    }
  }
  return result;
}      
  


SCALAR LaRCsimIC::GammaEqOfTheta( SCALAR theta_arg) {
  SCALAR a,b,c;
  
  a=cos(alpha)*cos(beta)*sin(theta_arg);
  b=sin(beta)*sin(phi);
  c=sin(alpha)*cos(beta)*cos(phi);
  return sin(gamma)-a+(b+c)*cos(theta_arg);
}

SCALAR LaRCsimIC::GammaEqOfAlpha( SCALAR alpha_arg) {
  float a,b,c;
  
  a=cos(alpha_arg)*cos(beta)*sin(theta);
  b=sin(beta)*sin(phi);
  c=sin(alpha_arg)*cos(beta)*cos(phi);
  return sin(gamma)-a+(b+c)*cos(theta);
}

  




bool LaRCsimIC::findInterval( SCALAR x,SCALAR guess) {
  //void find_interval(inter_params &ip,eqfunc f,float y,float constant, int &flag){

  int i=0;
  bool found=false;
  float flo,fhi,fguess;
  float lo,hi,step;
  step=0.1;
  fguess=(this->*sfunc)(guess)-x;
  lo=hi=guess;
  do {
    step=2*step;
    lo-=step;
    hi+=step;
    if(lo < xmin) lo=xmin;
    if(hi > xmax) hi=xmax;
    i++;
    flo=(this->*sfunc)(lo)-x;
    fhi=(this->*sfunc)(hi)-x;
    if(flo*fhi <=0) {  //found interval with root
      found=true;
      if(flo*fguess <= 0) {  //narrow interval down a bit
        hi=lo+step;    //to pass solver interval that is as
        //small as possible
      }
      else if(fhi*fguess <= 0) {
        lo=hi-step;
      }
    }
    //cout << "findInterval: i=" << i << " Lo= " << lo << " Hi= " << hi << endl;
  }
  while((found == 0) && (i <= 100));
  xlo=lo;
  xhi=hi;
  return found;
}




bool LaRCsimIC::solve( SCALAR *y,SCALAR x) {  
  float x1,x2,x3,f1,f2,f3,d,d0;
  float eps=1E-5;
  float const relax =0.9;
  int i;
  bool success=false;
  
   //initializations
  d=1;
  
    x1=xlo;x3=xhi;
    f1=(this->*sfunc)(x1)-x;
    f3=(this->*sfunc)(x3)-x;
    d0=fabs(x3-x1);
  
    //iterations
    i=0;
    while ((fabs(d) > eps) && (i < 100)) {
      d=(x3-x1)/d0;
      x2=x1-d*d0*f1/(f3-f1);
      
      f2=(this->*sfunc)(x2)-x;
      //cout << "solve x1,x2,x3: " << x1 << "," << x2 << "," << x3 << endl;
      //cout << "                " << f1 << "," << f2 << "," << f3 << endl;

      if(fabs(f2) <= 0.001) {
        x1=x3=x2;
      } else if(f1*f2 <= 0.0) {
        x3=x2;
        f3=f2;
        f1=relax*f1;
      } else if(f2*f3 <= 0) {
        x1=x2;
        f1=f2;
        f3=relax*f3;
      }
      //cout << i << endl;
      i++;
    }//end while
    if(i < 100) {
      success=true;
      *y=x2;
    }

  //cout << "Success= " << success << " Vcas: " << vcas*V_TO_KNOTS << " Mach: " << x2 << endl;
  return success;
}
