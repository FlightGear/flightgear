/*******************************************************************************

 Header:       FGState.h
 Author:       Jon S. Berndt
 Date started: 11/17/98

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

Based on Flightgear code, which is based on LaRCSim. This class wraps all
global state variables (such as velocity, position, orientation, etc.).

HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created
*******************************************************************************/

/*******************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGSTATE_H
#define FGSTATE_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include <stdio.h>
#include <fstream.h>

/*******************************************************************************
DEFINES
*******************************************************************************/

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGState
{
public:
   FGState(void);
  ~FGState(void);

  bool Reset(char*);
  bool StoreData(char*);
  bool DumpData(char*);
  bool DisplayData(void);
  
  inline float GetU(void) {return U;}
  inline float GetV(void) {return V;}
  inline float GetW(void) {return W;}

  inline float GetVn(void) {return Vn;}
  inline float GetVe(void) {return Ve;}
  inline float GetVd(void) {return Vd;}

  inline float GetVt(void) {return Vt;}

  inline float GetFx(void) {return Fx;}
  inline float GetFy(void) {return Fy;}
  inline float GetFz(void) {return Fz;}

  inline float GetP(void) {return P;}
  inline float GetQ(void) {return Q;}
  inline float GetR(void) {return R;}

  inline float GetQ0(void) {return Q0;}
  inline float GetQ1(void) {return Q1;}
  inline float GetQ2(void) {return Q2;}
  inline float GetQ3(void) {return Q3;}

  inline float GetL(void) {return L;}
  inline float GetM(void) {return M;}
  inline float GetN(void) {return N;}

  inline float GetIxx(void) const {return Ixx;}
  inline float GetIyy(void) const {return Iyy;}
  inline float GetIzz(void) const {return Izz;}
  inline float GetIxz(void) const {return Ixz;}

  inline float Getlatitude(void) {return latitude;}
  inline float Getlongitude(void) {return longitude;}
  inline float GetGeodeticLat(void) {return GeodeticLat;}
  
  inline float Getalpha(void) {return alpha;}
  inline float Getbeta(void) {return beta;}
  inline float Getgamma(void) {return gamma;}

  inline float Getadot(void) {return adot;}
  inline float Getbdot(void) {return bdot;}
  
  inline float GetUdot(void) {return Udot;}
  inline float GetVdot(void) {return Vdot;}
  inline float GetWdot(void) {return Wdot;}

  inline float GetPdot(void) {return Pdot;}
  inline float GetQdot(void) {return Qdot;}
  inline float GetRdot(void) {return Rdot;}

  inline float Geth(void) {return h;}
  inline float Geta(void) {return a;}
  inline float GetMach(void) {return Mach;}
  
  inline float Getrho(void) {return rho;}
  inline float Getsim_time(void) {return sim_time;}
  inline float Getdt(void) {return dt;}

  inline float Getphi(void) {return phi;}
  inline float Gettht(void) {return tht;}
  inline float Getpsi(void) {return psi;}

  inline float Getg(void) {return g;}
  inline float Getm(void) {return m;}

  inline float Getqbar(void) {return qbar;}
  inline float GetT(int r, int c) {return T[r][c];}

  inline void SetU(float tt) {U = tt;}
  inline void SetV(float tt) {V = tt;}
  inline void SetW(float tt) {W = tt;}

  inline void SetVt(float tt) {Vt = tt;}

  inline void SetVn(float tt) {Vn = tt;}
  inline void SetVe(float tt) {Ve = tt;}
  inline void SetVd(float tt) {Vd = tt;}

  inline void SetFx(float tt) {Fx = tt;}
  inline void SetFy(float tt) {Fy = tt;}
  inline void SetFz(float tt) {Fz = tt;}

  inline void SetP(float tt) {P = tt;}
  inline void SetQ(float tt) {Q = tt;}
  inline void SetR(float tt) {R = tt;}

  inline void SetL(float tt) {L = tt;}
  inline void SetM(float tt) {M = tt;}
  inline void SetN(float tt) {N = tt;}

  inline void SetIxx(float tt) {Ixx = tt;}
  inline void SetIyy(float tt) {Iyy = tt;}
  inline void SetIzz(float tt) {Izz = tt;}
  inline void SetIxz(float tt) {Ixz = tt;}

  inline void Setlatitude(float tt) {latitude = tt;}
  inline void Setlongitude(float tt) {longitude = tt;}
  inline void SetGeodeticLat(float tt) {GeodeticLat = tt;}

  inline void Setalpha(float tt) {alpha = tt;}
  inline void Setbeta(float tt) {beta = tt;}
  inline void Setgamma(float tt) {gamma = tt;}

  inline void Setadot(float tt) {adot = tt;}
  inline void Setbdot(float tt) {bdot = tt;}

  inline void SetUdot(float tt) {Udot = tt;}
  inline void SetVdot(float tt) {Vdot = tt;}
  inline void SetWdot(float tt) {Wdot = tt;}

  inline void SetPdot(float tt) {Pdot = tt;}
  inline void SetQdot(float tt) {Qdot = tt;}
  inline void SetRdot(float tt) {Rdot = tt;}

  inline void Setphi(float tt) {phi = tt;}
  inline void Settht(float tt) {tht = tt;}
  inline void Setpsi(float tt) {psi = tt;}

  inline void Setg(float tt) {g = tt;}
  inline void Setm(float tt) {m = tt;}
  inline void Setqbar(float tt) {qbar = tt;}

  inline void Seth(float tt) {h = tt;}
  inline void Seta(float tt) {a = tt;}
  inline void SetMach(float tt) {Mach = tt;}

  inline void Setrho(float tt) {rho = tt;}
  inline float Setsim_time(float tt) {sim_time = tt; return sim_time;}
  inline void Setdt(float tt) {dt = tt;}
  inline void SetQ0123(float q0, float q1, float q2, float q3) {Q0=q0;Q1=q1;Q2=q2;Q3=q3;}
  inline void SetT(int r, int c, float tt) {T[r][c] = tt;}

  inline float IncrTime(void) {sim_time+=dt;return sim_time;}

  const float EarthRad;

private:

  float U, V, W, Fx, Fy, Fz;        // A/C body axis velocities and forces
  float P, Q, R, L, M, N;           // A/C body axis rates and moments
  float Q0, Q1, Q2, Q3;             // Quaternion elements
  float Ixx, Iyy, Izz, Ixz;         // Moments of Inertia
  float Vt;                         // Total velocity
  float Vn, Ve, Vd;                 // North, East, and Down local velocities
  float latitude, longitude;        // position
  float GeodeticLat;                // Geodetic Latitude
  float alpha, beta, gamma;         // angle of attack, sideslip, and roll
  float adot, bdot;                 // alpha dot and beta dot
  float phi, tht, psi;              // Euler angles
  float Udot, Vdot, Wdot;           // A/C body axis accelerations
  float Pdot, Qdot, Rdot;           // A/C body axis rotational accelerations
  float h, a;                       // altitude above sea level, speed of sound
  float rho, qbar;                  // density of air in lb/ft^3, dynamic pressure
  float sim_time, dt;
  float m, g;                       // mass and acceleration of gravity
  float T[4][4];                    // Local to Body transformation matrix
  float Mach;                       // Mach number

protected:


};

#ifndef FDM_MAIN
extern FGState* State;
#else
FGState* State;
#endif

/******************************************************************************/
#endif
