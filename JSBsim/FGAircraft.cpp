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
Models the aircraft reactions and forces.

ARGUMENTS
--------------------------------------------------------------------------------


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
#include "FGAircraft.h"

#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGAircraft::FGAircraft(void) : FGModel()
{
  int i;

  strcpy(Name,"FGAircraft");

  for (i=0;i<6;i++) Axis[i] = (char*)malloc(7);
  for (i=0;i<6;i++) coeff_ctr[i] = 0;

  strcpy(Axis[LiftCoeff],"CLIFT");
  strcpy(Axis[DragCoeff],"CDRAG");
  strcpy(Axis[SideCoeff],"CSIDE");
  strcpy(Axis[RollCoeff],"CROLL");
  strcpy(Axis[PitchCoeff],"CPITCH");
  strcpy(Axis[YawCoeff],"CYAW");
}


FGAircraft::~FGAircraft(void)
{
}


bool FGAircraft::LoadAircraft(char* fname)
{
  char path[250];
  char fullpath[275];
  char filename[275];
  char aircraftDef[2100];
  char tag[220];
  DIR* dir;
  DIR* coeffdir;
  struct dirent* dirEntry = 0L;
  struct dirent* coeffdirEntry = 0L;
  struct stat st;
  struct stat st2;
  ifstream coeffInFile;

  char scratch[250];

  sprintf(aircraftDef, "/h/curt/projects/FlightGear/Simulator/FDM/JSBsim/aircraft/%s/%s.dat", fname, fname);
  ifstream aircraftfile(aircraftDef);

  if (aircraftfile) {
    aircraftfile >> AircraftName;
    aircraftfile >> WingArea;
    aircraftfile >> WingSpan;
    aircraftfile >> cbar;
    aircraftfile >> Ixx;
    aircraftfile >> Iyy;
    aircraftfile >> Izz;
    aircraftfile >> Ixz;
    aircraftfile >> Weight;
    m = Weight / 32.174;
    aircraftfile >> tag;

    numTanks = numEngines = 0;
    numSelectedOxiTanks = numSelectedFuelTanks = 0;

    while (strstr(tag,"EOF") == 0) {
      if (strstr(tag,"CGLOC")) {
        aircraftfile >> Xcg;
        aircraftfile >> Ycg;
        aircraftfile >> Zcg;
      } else if (strstr(tag,"EYEPOINTLOC")) {
        aircraftfile >> Xep;
        aircraftfile >> Yep;
        aircraftfile >> Zep;
      } else if (strstr(tag,"TANK")) {
        Tank[numTanks] = new FGTank(aircraftfile);
        switch(Tank[numTanks]->GetType()) {
        case 0:
          numSelectedOxiTanks++;
          break;
        case 1:
          numSelectedFuelTanks++;
          break;
        }
        numTanks++;
      } else if (strstr(tag,"ENGINE")) {
        aircraftfile >> tag;
        Engine[numEngines] = new FGEngine(tag);
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

    sprintf(path,"/h/curt/projects/FlightGear/Simulator/FDM/JSBsim/aircraft/%s/",AircraftName);
    if ( dir = opendir(path) ) {

      while (dirEntry = readdir(dir)) {
        sprintf(fullpath,"%s%s",path,dirEntry->d_name);
        stat(fullpath,&st);
        if ((st.st_mode & S_IFMT) == S_IFDIR) {
          for (int axis_ctr=0; axis_ctr < 6; axis_ctr++) {
            if (strstr(dirEntry->d_name,Axis[axis_ctr])) {
              if (coeffdir = opendir(fullpath)) {
                while (coeffdirEntry = readdir(coeffdir)) {
                  if (coeffdirEntry->d_name[0] != '.') {
                    sprintf(filename,"%s%s/%s",path,Axis[axis_ctr],coeffdirEntry->d_name);
                    stat(filename,&st2);
                    if (st2.st_size > 6) {
                      Coeff[axis_ctr][coeff_ctr[axis_ctr]] = new FGCoefficient(filename);
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

    FProp(); FAero(); FGear(); FMass();
    MProp(); MAero(); MGear(); MMass();

    PutState();
  } else {                               // skip Run() execution this time
  }
  return false;
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
  Forces[0] += -g*sin(tht) * m;
  Forces[1] +=  g*sin(phi)*cos(tht) * m;
  Forces[2] +=  g*cos(phi)*cos(tht) * m;
}


void FGAircraft::FProp(void)
{
  float Oshortage, Fshortage;

  for (int i=0;i<numEngines;i++) {
    Forces[0] += Engine[i]->CalcThrust();
  }

  //
  // UPDATE TANK CONTENTS
  //
  // For each engine, cycle through the tanks and draw an equal amount of
  // fuel (or oxidizer) from each active tank. The needed amount of fuel is
  // determined by the engine in the FGEngine class. If more fuel is needed
  // than is available in the tank, then that amount is considered a shortage,
  // and will be drawn from the next tank. If the engine cannot be fed what it
  // needs, it will be considered to be starved, and will shut down.

  for (int e=0; e<numEngines; e++) {
    Fshortage = Oshortage = 0.0;
    for (int t=0; t<numTanks; t++) {
      switch(Engine[e]->GetType()) {
      case 0: // Rocket
        switch(Tank[t]->GetType()) {
        case 0: // Fuel
          if (Tank[t]->GetSelected()) {
            Fshortage = Tank[t]->Reduce((Engine[e]->CalcFuelNeed()/numSelectedFuelTanks)*(dt*rate) + Fshortage);
          }
          break;
        case 1: // Oxidizer
          if (Tank[t]->GetSelected()) {
            Oshortage = Tank[t]->Reduce((Engine[e]->CalcOxidizerNeed()/numSelectedOxiTanks)*(dt*rate) + Oshortage);
          }
          break;
        }
        break;
      default: // piston, turbojet, turbofan, etc.
        if (Tank[t]->GetSelected()) {
          Fshortage = Tank[t]->Reduce((Engine[e]->CalcFuelNeed()/numSelectedFuelTanks)*(dt*rate) + Fshortage);
        }
        break;
      }
    }
    if ((Fshortage < 0.0) || (Oshortage < 0.0)) Engine[e]->SetStarved();
    else Engine[e]->SetStarved(false);
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
  Ixx = State->GetIxx();
  Iyy = State->GetIyy();
  Izz = State->GetIzz();
  Ixz = State->GetIxz();
  alpha = State->Getalpha();
  beta = State->Getbeta();
  m   = State->Getm();
  phi = State->Getphi();
  tht = State->Gettht();
  psi = State->Getpsi();
  g = State->Getg();
  dt = State->Getdt();
}


void FGAircraft::PutState(void)
{
  State->SetIxx(Ixx);
  State->SetIyy(Iyy);
  State->SetIzz(Izz);
  State->SetIxz(Ixz);
  State->SetFx(Forces[0]);
  State->SetFy(Forces[1]);
  State->SetFz(Forces[2]);
  State->SetL(Moments[0]);
  State->SetM(Moments[1]);
  State->SetN(Moments[2]);
  State->Setm(m);
}

