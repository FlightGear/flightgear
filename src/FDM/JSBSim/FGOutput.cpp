/*******************************************************************************

 Module:       FGOutput.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Manage output of sim parameters to file or stdout
 Called by:    FGSimExec

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
This is the place where you create output routines to dump data for perusal
later. Some machines may not support the ncurses console output. Borland is one
of those environments which does not, so the ncurses stuff is commented out.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGOutput.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGOutput::FGOutput(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGOutput";
  FirstPass = true;
}


FGOutput::~FGOutput(void)
{
}


bool FGOutput::Run(void)
{
  if (!FGModel::Run()) {
    DelimitedOutput("JSBSimData.csv");
  } else {
  }
  return false;
}


void FGOutput::DelimitedOutput(void)
{
  if (FirstPass) {
    cout << "Time,";
    cout << "Altitude,";
    cout << "Phi,";
    cout << "Tht,";
    cout << "Psi,";
    cout << "Rho,";
    cout << "Vtotal,";
    cout << "U,";
    cout << "V,";
    cout << "W,";
    cout << "Vn,";
    cout << "Ve,";
    cout << "Vd,";
    cout << "Udot,";
    cout << "Vdot,";
    cout << "Wdot,";
    cout << "P,";
    cout << "Q,";
    cout << "R,";
    cout << "PDot,";
    cout << "QDot,";
    cout << "RDot,";
    cout << "Fx,";
    cout << "Fy,";
    cout << "Fz,";
    cout << "Latitude,";
    cout << "Longitude,";
    cout << "QBar,";
    cout << "Alpha,";
    cout << "L,";
    cout << "M,";
    cout << "N";
    cout << endl;
    FirstPass = false;
  }

  cout << State->Getsim_time() << ",";
  cout << State->Geth() << ",";
  cout << Rotation->Getphi() << ",";
  cout << Rotation->Gettht() << ",";
  cout << Rotation->Getpsi() << ",";
  cout << Atmosphere->GetDensity() << ",";
  cout << State->GetVt() << ",";
  cout << Translation->GetU() << ",";
  cout << Translation->GetV() << ",";
  cout << Translation->GetW() << ",";
  cout << Position->GetVn() << ",";
  cout << Position->GetVe() << ",";
  cout << Position->GetVd() << ",";
  cout << Translation->GetUdot() << ",";
  cout << Translation->GetVdot() << ",";
  cout << Translation->GetWdot() << ",";
  cout << Rotation->GetP() << ",";
  cout << Rotation->GetQ() << ",";
  cout << Rotation->GetR() << ",";
  cout << Rotation->GetPdot() << ",";
  cout << Rotation->GetQdot() << ",";
  cout << Rotation->GetRdot() << ",";
  cout << Aircraft->GetFx() << ",";
  cout << Aircraft->GetFy() << ",";
  cout << Aircraft->GetFz() << ",";
  cout << State->Getlatitude() << ",";
  cout << State->Getlongitude() << ",";
  cout << State->Getqbar() << ",";
  cout << Translation->Getalpha() << ",";
  cout << Aircraft->GetL() << ",";
  cout << Aircraft->GetM() << ",";
  cout << Aircraft->GetN() << "";
  cout << endl;

}


void FGOutput::DelimitedOutput(string fname)
{
  if (FirstPass) {
    datafile.open(fname.c_str());
    datafile << "Time,";
    datafile << "Altitude,";
    datafile << "Phi,";
    datafile << "Tht,";
    datafile << "Psi,";
    datafile << "Rho,";
    datafile << "Vtotal,";
    datafile << "U,";
    datafile << "V,";
    datafile << "W,";
    datafile << "Vn,";
    datafile << "Ve,";
    datafile << "Vd,";
    datafile << "Udot,";
    datafile << "Vdot,";
    datafile << "Wdot,";
    datafile << "P,";
    datafile << "Q,";
    datafile << "R,";
    datafile << "PDot,";
    datafile << "QDot,";
    datafile << "RDot,";
    datafile << "Fx,";
    datafile << "Fy,";
    datafile << "Fz,";
    datafile << "Latitude,";
    datafile << "Longitude,";
    datafile << "QBar,";
    datafile << "Alpha,";
    datafile << "L,";
    datafile << "M,";
    datafile << "N";
    datafile << endl;
    FirstPass = false;
  }

  datafile << State->Getsim_time() << ",";
  datafile << State->Geth() << ",";
  datafile << Rotation->Getphi() << ",";
  datafile << Rotation->Gettht() << ",";
  datafile << Rotation->Getpsi() << ",";
  datafile << Atmosphere->GetDensity() << ",";
  datafile << State->GetVt() << ",";
  datafile << Translation->GetU() << ",";
  datafile << Translation->GetV() << ",";
  datafile << Translation->GetW() << ",";
  datafile << Position->GetVn() << ",";
  datafile << Position->GetVe() << ",";
  datafile << Position->GetVd() << ",";
  datafile << Translation->GetUdot() << ",";
  datafile << Translation->GetVdot() << ",";
  datafile << Translation->GetWdot() << ",";
  datafile << Rotation->GetP() << ",";
  datafile << Rotation->GetQ() << ",";
  datafile << Rotation->GetR() << ",";
  datafile << Rotation->GetPdot() << ",";
  datafile << Rotation->GetQdot() << ",";
  datafile << Rotation->GetRdot() << ",";
  datafile << Aircraft->GetFx() << ",";
  datafile << Aircraft->GetFy() << ",";
  datafile << Aircraft->GetFz() << ",";
  datafile << State->Getlatitude() << ",";
  datafile << State->Getlongitude() << ",";
  datafile << State->Getqbar() << ",";
  datafile << Translation->Getalpha() << ",";
  datafile << Aircraft->GetL() << ",";
  datafile << Aircraft->GetM() << ",";
  datafile << Aircraft->GetN() << "";
  datafile << endl;
  datafile.flush();
}

