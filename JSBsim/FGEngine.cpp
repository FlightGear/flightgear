/*******************************************************************************

 Module:       FGEngine.cpp
 Author:       Jon Berndt
 Date started: 01/21/99
 Called by:    FGAircraft

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

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include <fstream.h>

#include "FGEngine.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGEngine::FGEngine(FGFDMExec* fdex, string enginePath, string engineName, 
		   int num)
{
  string fullpath;
  string tag;

  FDMExec = fdex;

  State       = FDMExec->GetState();
  Atmosphere  = FDMExec->GetAtmosphere();
  FCS         = FDMExec->GetFCS();
  Aircraft    = FDMExec->GetAircraft();
  Translation = FDMExec->GetTranslation();
  Rotation    = FDMExec->GetRotation();
  Position    = FDMExec->GetPosition();
  Auxiliary   = FDMExec->GetAuxiliary();
  Output      = FDMExec->GetOutput();

  Name = engineName;
  fullpath = enginePath + "/" + engineName + ".dat";
  ifstream enginefile(fullpath.c_str());

  if (enginefile) {
    enginefile >> tag;

    if      (tag == "ROCKET")    Type = etRocket;
    else if (tag == "PISTON")    Type = etPiston;
    else if (tag == "TURBOPROP") Type = etTurboProp;
    else if (tag == "TURBOJET")  Type = etTurboJet;
    else                         Type = etUnknown;

    enginefile >> X;
    enginefile >> Y;
    enginefile >> Z;
    enginefile >> SLThrustMax;
    enginefile >> VacThrustMax;
    enginefile >> MaxThrottle;
    enginefile >> MinThrottle;
    enginefile >> SLFuelFlowMax;
    if (Type == 1)
      enginefile >> SLOxiFlowMax;
    enginefile.close();
  } else {
    cerr << "Unable to open engine definition file " << engineName << endl;
  }

  EngineNumber = num;
  Thrust = 0.0;
  Starved = Flameout = false;
}


FGEngine::~FGEngine(void)
{
}


float FGEngine::CalcRocketThrust(void)
{
  float lastThrust;

  Throttle = FCS->GetThrottle(EngineNumber);
  lastThrust = Thrust;                 // last actual thrust

  if (Throttle < MinThrottle || Starved) {
    PctPower = Thrust = 0.0; // desired thrust
    Flameout = true;
  } else {
    PctPower = Throttle / MaxThrottle;
    Thrust = PctPower*((1.0 - Atmosphere->Getrho() / 0.002378)*(VacThrustMax - SLThrustMax) +
	                       SLThrustMax); // desired thrust
    Flameout = false;
  }

  Thrust += 0.8*(Thrust - lastThrust); // actual thrust

  return Thrust;
}


float FGEngine::CalcPistonThrust(void)
{
  return Thrust;
}


float FGEngine::CalcThrust(void)
{
  switch(Type) {
  case etRocket:
    return CalcRocketThrust();
    // break;
  case etPiston:
    return CalcPistonThrust();
    // break;
  default:
    return 9999.0;
    // break;
  }
}

float FGEngine::CalcFuelNeed() {
  FuelNeed = SLFuelFlowMax*PctPower;
  return FuelNeed;
}


float FGEngine::CalcOxidizerNeed() {
  OxidizerNeed = SLOxiFlowMax*PctPower;
  return OxidizerNeed;
}

