/*******************************************************************************

 Module:       FGAircraft.cpp
 Author:       Jon S. Berndt
 Date started: 12/12/98                                   
 Purpose:      Encapsulates an aircraft
 Called by:    FGFDMExec

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
Models the aircraft reactions and forces. This class is instantiated by the
FGFDMExec class and scheduled as an FDM entry. LoadAircraft() is supplied with a
name of a valid, registered aircraft, and the data file is parsed.

HISTORY
--------------------------------------------------------------------------------
12/12/98   JSB   Created

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
      Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
      School, January 1994
[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
      JSC 12960, July 1977
[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
      NASA-Ames", NASA CR-2497, January 1975
[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
      Wiley & Sons, 1979 ISBN 0-471-03032-5
[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
      1982 ISBN 0-471-08936-2

The aerodynamic coefficients used in this model are:

Longitudinal
  CL0 - Reference lift at zero alpha
  CD0 - Reference drag at zero alpha
  CDM - Drag due to Mach
  CLa - Lift curve slope (w.r.t. alpha)
  CDa - Drag curve slope (w.r.t. alpha)
  CLq - Lift due to pitch rate
  CLM - Lift due to Mach
  CLadt - Lift due to alpha rate

  Cmadt - Pitching Moment due to alpha rate
  Cm0 - Reference Pitching moment at zero alpha
  Cma - Pitching moment slope (w.r.t. alpha)
  Cmq - Pitch damping (pitch moment due to pitch rate)
  CmM - Pitch Moment due to Mach

Lateral
  Cyb - Side force due to sideslip
  Cyr - Side force due to yaw rate

  Clb - Dihedral effect (roll moment due to sideslip)
  Clp - Roll damping (roll moment due to roll rate)
  Clr - Roll moment due to yaw rate
  Cnb - Weathercocking stability (yaw moment due to sideslip)
  Cnp - Rudder adverse yaw (yaw moment due to roll rate)
  Cnr - Yaw damping (yaw moment due to yaw rate)

Control
  CLDe - Lift due to elevator
  CDDe - Drag due to elevator
  CyDr - Side force due to rudder
  CyDa - Side force due to aileron

  CmDe - Pitch moment due to elevator
  ClDa - Roll moment due to aileron
  ClDr - Roll moment due to rudder
  CnDr - Yaw moment due to rudder
  CnDa - Yaw moment due to aileron

This class expects to be run in a directory which contains the subdirectory
structure shown below (where example aircraft X-15 is shown):

aircraft/
         X-15/
              X-15.dat reset00 reset01 reset02 ...
              CDRAG/
                 a0 a M De
              CSIDE/
                 b r Dr Da
              CLIFT/
                 a0 a M adt De
              CROLL/
                 b p r Da Dr
              CPITCH/
                 a0 a adt q M De
              CYAW/
                 b p r Dr Da
         F-16/
              F-16.dat reset00 reset01 ...
              CDRAG/
                 a0 a M De
              ...

The General Idea

The file structure is arranged so that various modeled aircraft are stored in
their own subdirectory. Each aircraft subdirectory is named after the aircraft.
There should be a file present in the specific aircraft subdirectory (e.g.
aircraft/X-15) with the same name as the directory with a .dat appended. This
file contains mass properties information, name of aircraft, etc. for the
aircraft. In that same directory are reset files numbered starting from 0 (two
digit numbers), e.g. reset03. Within each reset file are values for important
state variables for specific flight conditions (altitude, airspeed, etc.). Also
within this directory are the directories containing lookup tables for the
stability derivatives for the aircraft.

********************************************************************************
INCLUDES
*******************************************************************************/
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef FGFS
#  include <Include/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  include <cmath>
#endif

#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGAircraft::FGAircraft(FGFDMExec* fdmex) : FGModel(fdmex)
{
  int i;

  Name = "FGAircraft";

  for (i=0;i<6;i++) coeff_ctr[i] = 0;

  Axis[LiftCoeff]  = "CLIFT";
  Axis[DragCoeff]  = "CDRAG";
  Axis[SideCoeff]  = "CSIDE";
  Axis[RollCoeff]  = "CROLL";
  Axis[PitchCoeff] = "CPITCH";
  Axis[YawCoeff]   = "CYAW";
}


FGAircraft::~FGAircraft(void)
{
}


bool FGAircraft::LoadAircraft(string aircraft_path, string engine_path, string fname)
{
  string path;
  string fullpath;
  string filename;
  string aircraftDef;
  string tag;
  DIR* dir;
  DIR* coeffdir;
  struct dirent* dirEntry;
  struct dirent* coeffdirEntry;
  struct stat st;
  struct stat st2;
  ifstream coeffInFile;

  aircraftDef = aircraft_path + "/" + fname + "/" + fname + ".dat";
  ifstream aircraftfile(aircraftDef.c_str());

  if (aircraftfile) {
    aircraftfile >> AircraftName;   // String with no embedded spaces
    aircraftfile >> WingArea;       // square feet
    aircraftfile >> WingSpan;       // feet
    aircraftfile >> cbar;           // feet
    aircraftfile >> Ixx;            // slug ft^2
    aircraftfile >> Iyy;            // "
    aircraftfile >> Izz;            // "
    aircraftfile >> Ixz;            // "
    aircraftfile >> EmptyWeight;    // pounds
    EmptyMass = EmptyWeight / GRAVITY;
    aircraftfile >> tag;

    numTanks = numEngines = 0;
    numSelectedOxiTanks = numSelectedFuelTanks = 0;

    while ( !(tag == "EOF") ) {
      if (tag == "CGLOC") {
        aircraftfile >> Xcg;        // inches
        aircraftfile >> Ycg;        // inches
        aircraftfile >> Zcg;        // inches
      } else if (tag == "EYEPOINTLOC") {
        aircraftfile >> Xep;        // inches
        aircraftfile >> Yep;        // inches
        aircraftfile >> Zep;        // inches
      } else if (tag == "TANK") {
        Tank[numTanks] = new FGTank(aircraftfile);
        switch(Tank[numTanks]->GetType()) {
        case FGTank::ttFUEL:
          numSelectedFuelTanks++;
          break;
        case FGTank::ttOXIDIZER:
          numSelectedOxiTanks++;
          break;
        }
        numTanks++;
      } else if (tag == "ENGINE") {
        aircraftfile >> tag;
        Engine[numEngines] = new FGEngine(FDMExec, engine_path, tag, numEngines);
        numEngines++;
      }
      aircraftfile >> tag;
    }
    aircraftfile.close();
    PutState();

    // Read subdirectory for this aircraft for stability derivative lookup tables:
    //
    // Build up the path name to the aircraft file by appending the aircraft
    // name to the "aircraft/" initial path. Initialize the directory entry
    // structure dirEntry in preparation for reading through the directory.
    // Build up a path to each file in the directory sequentially and "stat" it
    // to see if the entry is a directory or a file. If the entry is a file, then
    // compare it to each string in the Axis[] array to see which axis the
    // directory represents: Lift, Drag, Side, Roll, Pitch, Yaw. When the match
    // is found, go into that directory and search for any coefficient files.
    // Build a new coefficient by passing the full pathname to the coefficient
    // file to the FGCoefficient constructor.
    //
    // Note: axis_ctr=0 for the Lift "axis", 1 for Drag, 2 for Side force, 3 for
    //       Roll, 4 for Pitch, and 5 for Yaw. The term coeff_ctr merely keeps
    //       track of the number of coefficients registered for each of the
    //       previously mentioned axis.

    path = aircraft_path + "/" + AircraftName + "/";
    if (dir = opendir(path.c_str())) {

      while (dirEntry = readdir(dir)) {
        fullpath = path + dirEntry->d_name;
        stat(fullpath.c_str(),&st);
        if ((st.st_mode & S_IFMT) == S_IFDIR) {
          for (int axis_ctr=0; axis_ctr < 6; axis_ctr++) {
            if (dirEntry->d_name == Axis[axis_ctr]) {
              if (coeffdir = opendir(fullpath.c_str())) {
                while (coeffdirEntry = readdir(coeffdir)) {
                  if (coeffdirEntry->d_name[0] != '.') {
                    filename = path + Axis[axis_ctr] + "/" + coeffdirEntry->d_name;
                    stat(filename.c_str(),&st2);
                    if (st2.st_size > 6) {
                      Coeff[axis_ctr][coeff_ctr[axis_ctr]] = new FGCoefficient(FDMExec, filename);
                      coeff_ctr[axis_ctr]++;
                    }
                  }
                }
              }
            }
          }
        }
      }
    } else {
      cerr << "Could not open directory " << path << " for reading" << endl;
    }
    return true;

  } else {
    cerr << "Unable to open aircraft definition file " << fname << endl;
    return false;
  }

}


bool FGAircraft::Run(void)
{
  if (!FGModel::Run()) {                 // if false then execute this Run()
    GetState();

    for (int i = 0; i < 3; i++)  Forces[i] = Moments[i] = 0.0;

    MassChange();

    FProp(); FAero(); FGear(); FMass();
    MProp(); MAero(); MGear(); MMass();

    PutState();
  } else {                               // skip Run() execution this time
  }
  return false;
}


void FGAircraft::MassChange()
{
  // UPDATE TANK CONTENTS
  //
  // For each engine, cycle through the tanks and draw an equal amount of
  // fuel (or oxidizer) from each active tank. The needed amount of fuel is
  // determined by the engine in the FGEngine class. If more fuel is needed
  // than is available in the tank, then that amount is considered a shortage,
  // and will be drawn from the next tank. If the engine cannot be fed what it
  // needs, it will be considered to be starved, and will shut down.

  float Oshortage, Fshortage;

  for (int e=0; e<numEngines; e++) {
    Fshortage = Oshortage = 0.0;
    for (int t=0; t<numTanks; t++) {
      switch(Engine[e]->GetType()) {
      case FGEngine::etRocket:

        switch(Tank[t]->GetType()) {
        case FGTank::ttFUEL:
          if (Tank[t]->GetSelected()) {
            Fshortage = Tank[t]->Reduce((Engine[e]->CalcFuelNeed()/
                                   numSelectedFuelTanks)*(dt*rate) + Fshortage);
          }
          break;
        case FGTank::ttOXIDIZER:
          if (Tank[t]->GetSelected()) {
            Oshortage = Tank[t]->Reduce((Engine[e]->CalcOxidizerNeed()/
                                    numSelectedOxiTanks)*(dt*rate) + Oshortage);
          }
          break;
        }
        break;

      case FGEngine::etPiston:
      case FGEngine::etTurboJet:
      case FGEngine::etTurboProp:

        if (Tank[t]->GetSelected()) {
          Fshortage = Tank[t]->Reduce((Engine[e]->CalcFuelNeed()/
                                   numSelectedFuelTanks)*(dt*rate) + Fshortage);
        }
        break;
      }
    }
    if ((Fshortage <= 0.0) || (Oshortage <= 0.0)) Engine[e]->SetStarved();
    else Engine[e]->SetStarved(false);
  }

  Weight = EmptyWeight;
  for (int t=0; t<numTanks; t++)
    Weight += Tank[t]->GetContents();

  Mass = Weight / GRAVITY;
}


void FGAircraft::FAero(void)
{
  float F[3];

  F[0] = F[1] = F[2] = 0.0;

  for (int axis_ctr = 0; axis_ctr < 3; axis_ctr++)
    for (int ctr=0; ctr < coeff_ctr[axis_ctr]; ctr++)
      F[axis_ctr] += Coeff[axis_ctr][ctr]->Value();

  Forces[0] +=  F[LiftCoeff]*sin(alpha) - F[DragCoeff]*cos(alpha) - F[SideCoeff]*sin(beta);
  Forces[1] +=  F[SideCoeff]*cos(beta);
  Forces[2] += -F[LiftCoeff]*cos(alpha) - F[DragCoeff]*sin(alpha);
}


void FGAircraft::FGear(void)
{
  if (GearUp) {
  } else {
  }
}


void FGAircraft::FMass(void)
{
  Forces[0] += -GRAVITY*sin(tht) * Mass;
  Forces[1] +=  GRAVITY*sin(phi)*cos(tht) * Mass;
  Forces[2] +=  GRAVITY*cos(phi)*cos(tht) * Mass;
}


void FGAircraft::FProp(void)
{
  for (int i=0;i<numEngines;i++) {
    Forces[0] += Engine[i]->CalcThrust();
  }
}


void FGAircraft::MAero(void)
{
  for (int axis_ctr = 0; axis_ctr < 3; axis_ctr++)
    for (int ctr = 0; ctr < coeff_ctr[axis_ctr+3]; ctr++)
      Moments[axis_ctr] += Coeff[axis_ctr+3][ctr]->Value();
}


void FGAircraft::MGear(void)
{
  if (GearUp) {
  } else {
  }
}


void FGAircraft::MMass(void)
{
}


void FGAircraft::MProp(void)
{
}


void FGAircraft::GetState(void)
{
  dt = State->Getdt();

  alpha = Translation->Getalpha();
  beta = Translation->Getbeta();
  phi = Rotation->Getphi();
  tht = Rotation->Gettht();
  psi = Rotation->Getpsi();
}


void FGAircraft::PutState(void)
{
}

