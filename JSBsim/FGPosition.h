/*******************************************************************************

 Header:       FGPosition.h
 Author:       Jon S. Berndt
 Date started: 1/5/99

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
01/05/99   JSB   Created

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
*******************************************************************************/

/*******************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGPOSITION_H
#define FGPOSITION_H

/*******************************************************************************
INCLUDES
*******************************************************************************/
#include "FGModel.h"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGPosition : public FGModel
{
public:
  FGPosition(void);
  ~FGPosition(void);

  float GetFn() {return Fn;}
  float GetFe() {return Fe;}
  float GetFd() {return Fd;}

  bool Run(void);

protected:

private:
  float T[4][4];
  float Q0, Q1, Q2, Q3;
  float Fn, Fe, Fd;
  float Fx, Fy, Fz;
  float invMass, invRadius;
  float EarthRad, OmegaEarth, Radius;
  float AccelN, AccelE, AccelD;
  float lastAccelN, lastAccelE, lastAccelD;
  float LatitudeDot, LongitudeDot, RadiusDot;
  float lastLatitudeDot, lastLongitudeDot, lastRadiusDot;
  float Longitude, Latitude;
  float U, V, W;
  float Vn, Ve, Vd, Vee;
  float dt;

  void GetState(void);
  void PutState(void);
};

#ifndef FDM_MAIN
extern FGPosition* Position;
#else
FGPosition* Position;
#endif

/******************************************************************************/
#endif
