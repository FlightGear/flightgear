/*******************************************************************************

 Module:       FGState.cpp
 Author:       Jon Berndt
 Date started: 11/17/98
 Called by:    FGFDMExec and accessed by all models.

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
See header file.

ARGUMENTS
--------------------------------------------------------------------------------


HISTORY
--------------------------------------------------------------------------------

11/17/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGState.h"
#include "FGAircraft.h"

#include <math.h>

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGState::FGState(void)
{
  U = V = W = Fx = Fy = Fz = 0.0;
  P = Q = R = L = M = N = 0.0;
  Q0 = Q1 = Q2 = Q3 = 0.0;
  Ixx = Iyy = Izz = Ixz = 0.0;
  Vt = 0.0;
  latitude = longitude = 0.0;
  alpha = beta = gamma = 0.0;
  adot = bdot = 0.0;
  phi = tht = psi = 0.0;
  Udot = Vdot = Wdot = 0.0;
  Pdot = Qdot = Rdot = 0.0;
  h = 0.0;
  a = 1000.0;
  rho = qbar = 0.0;
  sim_time = dt = 0.1;
  m = 0.0;
  g = 32.174;

  const float EarthRad     = 20925650.0;
}


FGState::~FGState(void)
{
}


bool FGState::Reset(char* fname)
{
  char resetDef[200];

  sprintf(resetDef, "/h/curt/projects/FlightGear/Simulator/FDM/JSBsim/aircraft/%s/%s", Aircraft->GetAircraftName(), fname);

  ifstream resetfile(resetDef);

  if (resetfile) {
    resetfile >> U;
    resetfile >> V;
    resetfile >> W;
    resetfile >> latitude;
    resetfile >> longitude;
    resetfile >> phi;
    resetfile >> tht;
    resetfile >> psi;
    resetfile >> h;
    resetfile.close();

// Change all angular measurements from degrees (as in config file) to radians

    if (W != 0.0)
      alpha = U*U > 0.0 ? atan2(W, U) : 0.0;
    if (V != 0.0)
      beta = U*U+W*W > 0.0 ? atan2(V, (fabs(U)/U)*sqrt(U*U + W*W)) : 0.0;

    latitude  *= M_PI / 180.0;
    longitude *= M_PI / 180.0;
    alpha     *= M_PI / 180.0;
    beta      *= M_PI / 180.0;
    gamma     *= M_PI / 180.0;
    phi       *= M_PI / 180.0;
    tht       *= M_PI / 180.0;
    psi       *= M_PI / 180.0;

    Vt = sqrt(U*U + V*V + W*W);
    qbar = sqrt(U*U + V*V + W*W);

    Q0 =  sin(psi*0.5)*sin(tht*0.5)*sin(phi*0.5) + cos(psi*0.5)*cos(tht*0.5)*cos(phi*0.5);
    Q1 = -sin(psi*0.5)*sin(tht*0.5)*cos(phi*0.5) + cos(psi*0.5)*cos(tht*0.5)*sin(phi*0.5);
    Q2 =  sin(psi*0.5)*cos(tht*0.5)*sin(phi*0.5) + cos(psi*0.5)*sin(tht*0.5)*cos(phi*0.5);
    Q3 =  sin(psi*0.5)*cos(tht*0.5)*cos(phi*0.5) - cos(psi*0.5)*sin(tht*0.5)*sin(phi*0.5);

    T[1][1] = Q0*Q0 + Q1*Q1 - Q2*Q2 - Q3*Q3;
    T[1][2] = 2*(Q1*Q2 + Q0*Q3);
    T[1][3] = 2*(Q1*Q3 - Q0*Q2);
    T[2][1] = 2*(Q1*Q2 - Q0*Q3);
    T[2][2] = Q0*Q0 - Q1*Q1 + Q2*Q2 - Q3*Q3;
    T[2][3] = 2*(Q2*Q3 + Q0*Q1);
    T[3][1] = 2*(Q1*Q3 + Q0*Q2);
    T[3][2] = 2*(Q2*Q3 - Q0*Q1);
    T[3][3] = Q0*Q0 - Q1*Q1 - Q2*Q2 + Q3*Q3;

    return true;
  } else {
    cerr << "Unable to load reset file " << fname << endl;
    return false;
  }
}


bool FGState::StoreData(char* fname)
{
  ofstream datafile(fname);

  if (datafile) {
    datafile << U;
    datafile << V;
    datafile << W;
    datafile << Fx;
    datafile << Fy;
    datafile << Fz;
    datafile << P;
    datafile << Q;
    datafile << R;
    datafile << L;
    datafile << M;
    datafile << N;
    datafile << latitude;
    datafile << longitude;
    datafile << alpha;
    datafile << beta;
    datafile << gamma;
    datafile << phi;
    datafile << tht;
    datafile << psi;
    datafile << Udot;
    datafile << Vdot;
    datafile << Wdot;
    datafile << Pdot;
    datafile << Qdot;
    datafile << Rdot;
    datafile << h;
    datafile << a;
    datafile << rho;
    datafile << qbar;
    datafile << sim_time;
    datafile << dt;
    datafile << g;
    datafile.close();
    return true;
  } else {
    cerr << "Could not open dump file " << fname << endl;
    return false;
  }
}


bool FGState::DumpData(char* fname)
{
  ofstream datafile(fname);

  if (datafile) {
    datafile << "U: " << U << endl;
    datafile << "V: " << V << endl;
    datafile << "W: " << W << endl;
    datafile << "Fx: " << Fx << endl;
    datafile << "Fy: " << Fy << endl;
    datafile << "Fz: " << Fz << endl;
    datafile << "P: " << P << endl;
    datafile << "Q: " << Q << endl;
    datafile << "R: " << R << endl;
    datafile << "L: " << L << endl;
    datafile << "M: " << M << endl;
    datafile << "N: " << N << endl;
    datafile << "latitude: " << latitude << endl;
    datafile << "longitude: " << longitude << endl;
    datafile << "alpha: " << alpha << endl;
    datafile << "beta: " << beta << endl;
    datafile << "gamma: " << gamma << endl;
    datafile << "phi: " << phi << endl;
    datafile << "tht: " << tht << endl;
    datafile << "psi: " << psi << endl;
    datafile << "Udot: " << Udot << endl;
    datafile << "Vdot: " << Vdot << endl;
    datafile << "Wdot: " << Wdot << endl;
    datafile << "Pdot: " << Pdot << endl;
    datafile << "Qdot: " << Qdot << endl;
    datafile << "Rdot: " << Rdot << endl;
    datafile << "h: " << h << endl;
    datafile << "a: " << a << endl;
    datafile << "rho: " << rho << endl;
    datafile << "qbar: " << qbar << endl;
    datafile << "sim_time: " << sim_time << endl;
    datafile << "dt: " << dt << endl;
    datafile << "g: " << g << endl;
    datafile << "m: " << m << endl;
    datafile << "Ixx: " << Ixx << endl;
    datafile << "Iyy: " << Iyy << endl;
    datafile << "Izz: " << Izz << endl;
    datafile << "Ixz: " << Ixz << endl;
    datafile.close();
    return true;
  } else {
    return false;
  }
}


bool FGState::DisplayData(void)
{
  cout << "U: " << U << endl;
  cout << "V: " << V << endl;
  cout << "W: " << W << endl;
  cout << "Fx: " << Fx << endl;
  cout << "Fy: " << Fy << endl;
  cout << "Fz: " << Fz << endl;
  cout << "P: " << P << endl;
  cout << "Q: " << Q << endl;
  cout << "R: " << R << endl;
  cout << "L: " << L << endl;
  cout << "M: " << M << endl;
  cout << "N: " << N << endl;
  cout << "Vt: " << Vt << endl;
  cout << "latitude: " << latitude << endl;
  cout << "longitude: " << longitude << endl;
  cout << "alpha: " << alpha << endl;
  cout << "beta: " << beta << endl;
  cout << "gamma: " << gamma << endl;
  cout << "phi: " << phi << endl;
  cout << "tht: " << tht << endl;
  cout << "psi: " << psi << endl;
  cout << "Udot: " << Udot << endl;
  cout << "Vdot: " << Vdot << endl;
  cout << "Wdot: " << Wdot << endl;
  cout << "Pdot: " << Pdot << endl;
  cout << "Qdot: " << Qdot << endl;
  cout << "Rdot: " << Rdot << endl;
  cout << "h: " << h << endl;
  cout << "a: " << a << endl;
  cout << "rho: " << rho << endl;
  cout << "qbar: " << qbar << endl;
  cout << "sim_time: " << sim_time << endl;
  cout << "dt: " << dt << endl;
  cout << "g: " << g << endl;
  cout << "m: " << m << endl;
  cout << "Ixx: " << Ixx << endl;
  cout << "Iyy: " << Iyy << endl;
  cout << "Izz: " << Izz << endl;
  cout << "Ixz: " << Ixz << endl;

  return true;
}
