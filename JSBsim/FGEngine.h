/*******************************************************************************

 Header:       FGEngine.h
 Author:       Jon S. Berndt
 Date started: 01/21/99

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

Based on Flightgear code, which is based on LaRCSim. This class simulates
a generic engine.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created
*******************************************************************************/

/*******************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGEngine_H
#define FGEngine_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

/*******************************************************************************
DEFINES
*******************************************************************************/

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGEngine
{
public:
  FGEngine(void);
  FGEngine(char*);
  ~FGEngine(void);

  float GetThrust(void) {return Thrust;}
  bool  GetStarved(void) {return Starved;}
  bool  GetFlameout(void) {return Flameout;}
  float GetThrottle(void) {return Throttle;}
  char* GetName() {return Name;}

  void SetStarved(bool tt) {Starved = tt;}
  void SetStarved(void) {Starved = true;}

  int GetType(void) {return Type;}

  float CalcThrust(void);
  float CalcFuelNeed(void);
  float CalcOxidizerNeed(void);

private:
  char  Name[30];
  float X, Y, Z;
  int   Type;
  float SLThrustMax;
  float VacThrustMax;
  float SLFuelFlowMax;
  float SLOxiFlowMax;
  float MaxThrottle;
  float MinThrottle;

  float Thrust;
  float Throttle;
  float FuelNeed, OxidizerNeed;
  bool  Starved;
  bool  Flameout;
  float PctPower;

protected:
  float CalcRocketThrust(void);
  float CalcPistonThrust(void);

};

/******************************************************************************/
#endif
