/*******************************************************************************

 Header:       FGAircraft.h
 Author:       Jon S. Berndt
 Date started: 12/12/98

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

HISTORY
--------------------------------------------------------------------------------
12/12/98   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGAIRCRAFT_H
#define FGAIRCRAFT_H

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
*******************************************************************************/
/**
The aerodynamic coefficients used in this model typically are:
<PRE>
<b>Longitudinal</b>
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

<b>Lateral</b>
  Cyb - Side force due to sideslip
  Cyr - Side force due to yaw rate

  Clb - Dihedral effect (roll moment due to sideslip)
  Clp - Roll damping (roll moment due to roll rate)
  Clr - Roll moment due to yaw rate
  Cnb - Weathercocking stability (yaw moment due to sideslip)
  Cnp - Rudder adverse yaw (yaw moment due to roll rate)
  Cnr - Yaw damping (yaw moment due to yaw rate)

<b>Control</b>
  CLDe - Lift due to elevator
  CDDe - Drag due to elevator
  CyDr - Side force due to rudder
  CyDa - Side force due to aileron

  CmDe - Pitch moment due to elevator
  ClDa - Roll moment due to aileron
  ClDr - Roll moment due to rudder
  CnDr - Yaw moment due to rudder
  CnDa - Yaw moment due to aileron
</PRE>
This class expects to be run in a directory which contains the subdirectory
structure shown below (where example aircraft X-15 is shown):

<PRE>
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
</PRE>

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
@author Jon S. Berndt
@memo  Encompasses all aircraft functionality and objects
@see <ll>
<li>[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
	 Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
	 School, January 1994</li>
<li>[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
	 JSC 12960, July 1977</li>
<li>[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
	 NASA-Ames", NASA CR-2497, January 1975</li>
<li>[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
	 Wiley & Sons, 1979 ISBN 0-471-03032-5</li>
<li>[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
	 1982 ISBN 0-471-08936-2</li>
</ll>
*/

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <fstream>
#  else
#    include <fstream.h>
#  endif
#  include STL_STRING
   FG_USING_STD(string);
#else
#  include <fstream>
#endif

#include "FGModel.h"
#include "FGCoefficient.h"
#include "FGEngine.h"
#include "FGTank.h"

/*******************************************************************************
DEFINITIONS
*******************************************************************************/

#ifndef FGFS
using namespace std;
#endif

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGAircraft : public FGModel
{
public:
  // ***************************************************************************
  /** @memo Constructor
      @param FGFDMExec* - a pointer to the "owning" FDM Executive
  */
  FGAircraft(FGFDMExec*);
  
  // ***************************************************************************
  /** Destructor */
  ~FGAircraft(void);

  // ***************************************************************************
  /** This must be called for each dt to execute the model algorithm */
  bool Run(void);

  // ***************************************************************************
  /** This function must be called with the name of an aircraft which
      has an associated .dat file in the appropriate subdirectory. The paths
      to the appropriate subdirectories are given as the first two parameters.
      @memo Loads the given aircraft.
      @param string Path to the aircraft files
      @param string Path to the engine files
      @param string The name of the aircraft to be loaded, e.g. "X15".
      @return True - if successful
  */
  bool LoadAircraft(string, string, string);

  // ***************************************************************************
  /** This function must be called with the name of an aircraft which
      has an associated .dat file in the appropriate subdirectory. The paths
      to the appropriate subdirectories are given as the first two parameters.
      @memo Loads the given aircraft.
      @param string Path to the aircraft files
      @param string Path to the engine files
      @param string The name of the aircraft to be loaded, e.g. "X15".
      @return True - if successful
  */
  bool LoadAircraftEx(string, string, string);

  // ***************************************************************************
  /** @memo Gets the aircraft name as defined in the aircraft config file.
      @param
      @return string Aircraft name.
  */
  inline string GetAircraftName(void) {return AircraftName;}

  // ***************************************************************************
  /** @memo Sets the GearUp flag
      @param boolean true or false
      @return
  */
  inline void SetGearUp(bool tt) {GearUp = tt;}

  // ***************************************************************************
  /** @memo Returns the state of the GearUp flag
      @param
      @return boolean true or false
  */
  inline bool GetGearUp(void) {return GearUp;}

  // ***************************************************************************
  /** @memo Returns the area of the wing
      @param
      @return float wing area S, in square feet
  */
  inline float GetWingArea(void) {return WingArea;}

  // ***************************************************************************
  /** @memo Returns the wing span
      @param
      @return float wing span in feet
  */
  inline float GetWingSpan(void) {return WingSpan;}

  // ***************************************************************************
  /** @memo Returns the average wing chord
      @param
      @return float wing chord in feet
  */
  inline float Getcbar(void) {return cbar;}

  // ***************************************************************************
  /** @memo Returns an engine object
      @param int The engine number
      @return FGEengine* The pointer to the requested engine object.
  */
  inline FGEngine* GetEngine(int tt) {return Engine[tt];}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline FGTank* GetTank(int tt) {return Tank[tt];}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetWeight(void) {return Weight;}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetMass(void) {return Mass;}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetL(void) {return Moments[0];}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetM(void) {return Moments[1];}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetN(void) {return Moments[2];}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetFx(void) {return Forces[0];}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetFy(void) {return Forces[1];}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetFz(void) {return Forces[2];}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetIxx(void) {return Ixx;}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetIyy(void) {return Iyy;}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetIzz(void) {return Izz;}

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  inline float GetIxz(void) {return Ixz;}

private:
  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void GetState(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void PutState(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void FAero(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void FGear(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void FMass(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void FProp(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void MAero(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void MGear(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void MMass(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void MProp(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  void MassChange(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  float Moments[3];

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  float Forces[3];

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  string AircraftName;

  // ***************************************************************************
  ///
  float Ixx, Iyy, Izz, Ixz, EmptyMass, Mass;
  ///
  float Xcg, Ycg, Zcg;
  ///
  float Xep, Yep, Zep;
  ///
  float rho, qbar, Vt;
  ///
  float alpha, beta;
  ///
  float WingArea, WingSpan, cbar;
  ///
  float phi, tht, psi;
  ///
  float Weight, EmptyWeight;
  ///
  float dt;

  ///
  int numTanks;
  ///
  int numEngines;
  ///
  int numSelectedOxiTanks;
  ///
  int numSelectedFuelTanks;
  ///
  FGTank* Tank[MAX_TANKS];
  ///
  FGEngine *Engine[MAX_ENGINES];

  ///
  FGCoefficient *Coeff[6][10];
  ///
  int coeff_ctr[6];

  ///
  bool GearUp;

  ///
  enum Param {LiftCoeff,
              DragCoeff,
              SideCoeff,
              RollCoeff,
              PitchCoeff,
              YawCoeff,
              numCoeffs};

  ///
  string Axis[6];

protected:

};

/******************************************************************************/
#endif
