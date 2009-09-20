/*******************************************************************************
 
 Header:       LaRCsimIC.hxx
 Author:       Tony Peden
 Date started: 10/9/00
 
 ------------- Copyright (C) 2000 Anthony K. Peden (apeden@earthlink.net) -------------
 
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
#ifndef _LARCSIMIC_HXX
#define _LARCSIMIC_HXX

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "ls_constants.h"
#include "ls_types.h"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

#define KTS_TO_FPS 1.6889
#define M_TO_FT 3.2808399
#define DEFAULT_AGL_ALT 3.758099
#define DEFAULT_PITCH_ON_GROUND 0.0074002
#define DEGREES_TO_RADIANS (M_PI / 180.0)

enum lsspeedset { lssetvt, lssetvc, lssetve, lssetmach, lssetuvw, lssetned };
enum lsaltset { lssetasl, lssetagl };


class LaRCsimIC {
public:

  LaRCsimIC(void);
  ~LaRCsimIC(void);

  void SetVcalibratedKtsIC(SCALAR tt);
  void SetMachIC(SCALAR tt);
  
  void SetVtrueFpsIC(SCALAR tt);
  
  void SetVequivalentKtsIC(SCALAR tt);

  void SetUVWFpsIC(SCALAR vu, SCALAR vv, SCALAR vw);
 
  void SetVNEDFpsIC(SCALAR north, SCALAR east, SCALAR down);
  
  void SetVNEDAirmassFpsIC(SCALAR north, SCALAR east, SCALAR down );
  
  void SetAltitudeFtIC(SCALAR tt);
  void SetAltitudeAGLFtIC(SCALAR tt);
  
  //"vertical" flight path, recalculate theta
  inline void SetFlightPathAngleDegIC(SCALAR tt) { SetFlightPathAngleRadIC(tt*DEGREES_TO_RADIANS); }
  void SetFlightPathAngleRadIC(SCALAR tt);
  
  //set speed first
  void SetClimbRateFpmIC(SCALAR tt);
  void SetClimbRateFpsIC(SCALAR tt);
  
  //use currently stored gamma, recalcualte theta
  inline void SetAlphaDegIC(SCALAR tt)      { alpha=tt*DEGREES_TO_RADIANS; getTheta(); }
  inline void SetAlphaRadIC(SCALAR tt)      { alpha=tt; getTheta(); }
  
  //use currently stored gamma, recalcualte alpha
  inline void SetPitchAngleDegIC(SCALAR tt) { SetPitchAngleRadIC(tt*DEGREES_TO_RADIANS); }
         void SetPitchAngleRadIC(SCALAR tt);

  inline void SetBetaDegIC(SCALAR tt)       { beta=tt*DEGREES_TO_RADIANS; getTheta();}
  inline void SetBetaRadIC(SCALAR tt)       { beta=tt; getTheta(); }
  
  inline void SetRollAngleDegIC(SCALAR tt) { phi=tt*DEGREES_TO_RADIANS; getTheta(); }
  inline void SetRollAngleRadIC(SCALAR tt) { phi=tt; getTheta(); }

  inline void SetHeadingDegIC(SCALAR tt)   { psi=tt*DEGREES_TO_RADIANS; }
  inline void SetHeadingRadIC(SCALAR tt)   { psi=tt; }

  inline void SetLatitudeGDDegIC(SCALAR tt)  { SetLatitudeGDRadIC(tt*DEGREES_TO_RADIANS); }
         void SetLatitudeGDRadIC(SCALAR tt);

  inline void SetLongitudeDegIC(SCALAR tt) { longitude=tt*DEGREES_TO_RADIANS; }
  inline void SetLongitudeRadIC(SCALAR tt) { longitude=tt; }
  
  void SetRunwayAltitudeFtIC(SCALAR tt);

  inline SCALAR GetVcalibratedKtsIC(void) { return sqrt(density_ratio*vt*vt)*V_TO_KNOTS; }
  inline SCALAR GetVequivalentKtsIC(void) { return sqrt(density_ratio)*vt*V_TO_KNOTS; }
  inline SCALAR GetVtrueKtsIC(void) { return vt*V_TO_KNOTS; }
  inline SCALAR GetVtrueFpsIC(void) { return vt; }
  inline SCALAR GetMachIC(void) { return vt/soundspeed; }

  inline SCALAR GetAltitudeFtIC(void)    { return altitude; }
  inline SCALAR GetAltitudeAGLFtIC(void) { return alt_agl; }
  
  inline SCALAR GetRunwayAltitudeFtIC(void) { return runway_altitude; }

  inline SCALAR GetFlightPathAngleDegIC(void) { return gamma*DEGREES_TO_RADIANS; }
  inline SCALAR GetFlightPathAngleRadIC(void) { return gamma; }

  inline SCALAR GetClimbRateFpmIC(void) { return hdot*60; }
  inline SCALAR GetClimbRateFpsIC(void) { return hdot; }

  inline SCALAR GetAlphaDegIC(void)      { return alpha*DEGREES_TO_RADIANS; }
  inline SCALAR GetAlphaRadIC(void)      { return alpha; }

  inline SCALAR GetPitchAngleDegIC(void) { return theta*DEGREES_TO_RADIANS; }
  inline SCALAR GetPitchAngleRadIC(void) { return theta; }


  inline SCALAR GetBetaDegIC(void)       { return beta*DEGREES_TO_RADIANS; }
  inline SCALAR GetBetaRadIC(void)       { return beta*DEGREES_TO_RADIANS; }

  inline SCALAR GetRollAngleDegIC(void) { return phi*DEGREES_TO_RADIANS; }
  inline SCALAR GetRollAngleRadIC(void) { return phi; }

  inline SCALAR GetHeadingDegIC(void)   { return psi*DEGREES_TO_RADIANS; }
  inline SCALAR GetHeadingRadIC(void)   { return psi; }

  inline SCALAR GetLatitudeGDDegIC(void)  { return latitude_gd*DEGREES_TO_RADIANS; }
  inline SCALAR GetLatitudeGDRadIC(void) { return latitude_gd; }

  inline SCALAR GetLongitudeDegIC(void) { return longitude*DEGREES_TO_RADIANS; }
  inline SCALAR GetLongitudeRadIC(void) { return longitude; }

  inline SCALAR GetUBodyFpsIC(void) { return vt*cos(alpha)*cos(beta); }
  inline SCALAR GetVBodyFpsIC(void) { return vt*sin(beta); }
  inline SCALAR GetWBodyFpsIC(void) { return vt*sin(alpha)*cos(beta); }
  
  inline SCALAR GetVnorthFpsIC(void) { calcNEDfromVt();return vnorth; }
  inline SCALAR GetVeastFpsIC(void)  { calcNEDfromVt();return veast; }
  inline SCALAR GetVdownFpsIC(void)  { calcNEDfromVt();return vdown; }
  
  inline SCALAR GetVnorthAirmassFpsIC(void) { return vnorthwind; }
  inline SCALAR GetVeastAirmassFpsIC(void)  { return veastwind; }
  inline SCALAR GetVdownAirmassFpsIC(void)  { return vdownwind; }
  
  inline SCALAR GetThetaRadIC(void) { return theta; }
  inline SCALAR GetPhiRadIC(void)   { return phi; }
  inline SCALAR GetPsiRadIC(void)   { return psi; }



private:
  SCALAR vt,vtg,vw,vc,ve;
  SCALAR alpha,beta,gamma,theta,phi,psi;
  SCALAR mach;
  SCALAR altitude,runway_altitude,hdot,alt_agl,sea_level_radius;
  SCALAR latitude_gd,latitude_gc,longitude;
  SCALAR u,v,w;
  SCALAR vnorth,veast,vdown;
  SCALAR vnorthwind, veastwind, vdownwind;
  SCALAR p,T,soundspeed,density_ratio,rho;
  
  SCALAR xlo, xhi,xmin,xmax;
  
  typedef SCALAR (LaRCsimIC::*fp)(SCALAR x);
  fp sfunc;

  lsspeedset lastSpeedSet;
  lsaltset lastAltSet;
  
  
  void calcVtfromNED(void);
  void calcNEDfromVt(void);
  void calcSpeeds(void);
  
  
  bool getAlpha(void);
  bool getTheta(void);
  SCALAR GammaEqOfTheta(SCALAR tt);
  SCALAR GammaEqOfAlpha(SCALAR tt);
  void get_atmosphere(void);
  
  
  bool findInterval(SCALAR x,SCALAR guess);
  bool solve(SCALAR *y, SCALAR x);
};

#endif
