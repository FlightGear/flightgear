/*******************************************************************************

 Module:       FGUtility.cpp
 Author:       Jon Berndt
 Date started: 01/09/99
 Purpose:      Contains utility classes for the FG FDM
 Called by:    FGPosition, et. al.

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
This class is a container for all utility classes used by the flight dynamics
model.

HISTORY
--------------------------------------------------------------------------------
01/09/99   JSB   Created

********************************************************************************
DEFINES
*******************************************************************************/
                                                        
/*******************************************************************************
INCLUDES
*******************************************************************************/

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

#include "FGUtility.h"
#include "FGState.h"
#include "FGFDMExec.h"

#ifndef M_PI
/* get a definition for pi */
#include <Include/fg_constants.h>
#define M_PI FG_PI
#endif

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGUtility::FGUtility()
{
  // Move constant stuff to here (if any) so it won't take CPU time
  // in the methods below.
  SeaLevelR   = EARTHRAD * ECCENT;
}


FGUtility::~FGUtility()
{
}
                       

float FGUtility::ToGeodetic()
{
  float Latitude, Radius, Altitude;
  float tanLat, xAlpha, muAlpha, sinmuAlpha, denom, rhoAlpha, dMu;
  float lPoint, lambdaSL, sinlambdaSL, dLambda, rAlpha;

  Latitude = State->Getlatitude();
  Radius = State->Geth() + EARTHRAD;

  if (( M_PI_2 - Latitude < ONESECOND) ||
      ( M_PI_2 + Latitude < ONESECOND)) { // Near a pole
  } else {
    tanLat = tan(Latitude);
    xAlpha = ECCENT*EARTHRAD /
                                sqrt(tanLat*tanLat + ECCENTSQRD);
    muAlpha = atan2(sqrt(EARTHRADSQRD - xAlpha*xAlpha), ECCENT*xAlpha);

    if (Latitude < 0.0) muAlpha = -muAlpha;

    sinmuAlpha  = sin(muAlpha);
    dLambda     = muAlpha - Latitude;
    rAlpha      = xAlpha / cos(Latitude);
    lPoint      = Radius - rAlpha;
    Altitude    = lPoint*cos(dLambda);
    denom       = sqrt(1-EPS*EPS*sinmuAlpha*sinmuAlpha);
    rhoAlpha    = EARTHRAD*(1.0 - EPS) / (denom*denom*denom);
    dMu         = atan2(lPoint*sin(dLambda),rhoAlpha + Altitude);
    State->SetGeodeticLat(muAlpha - dMu);
    lambdaSL    = atan(ECCENTSQRD*tan(muAlpha - dMu));
    sinlambdaSL = sin(lambdaSL);
    SeaLevelR   = sqrt(EARTHRADSQRD / (1 + INVECCENTSQRDM1* sinlambdaSL*sinlambdaSL));
  }
  return 0.0;
}


float FGUtility:: FromGeodetic()
{
  float lambdaSL, sinlambdaSL, coslambdaSL, sinMu, cosMu, py, px;
  float Altitude, SeaLevelR, Radius;

  Radius = State->Geth() + EARTHRAD;
  lambdaSL = atan(ECCENTSQRD*tan(State->GetGeodeticLat()));
  sinlambdaSL = sin(lambdaSL);
  coslambdaSL = cos(lambdaSL);
  sinMu = sin(State->GetGeodeticLat());
  cosMu = cos(State->GetGeodeticLat());
  SeaLevelR = sqrt(EARTHRADSQRD /
             (1 + INVECCENTSQRDM1*sinlambdaSL*sinlambdaSL));
  Altitude  = Radius - SeaLevelR;
  px = SeaLevelR*coslambdaSL + Altitude*cosMu;
  py = SeaLevelR*sinlambdaSL + Altitude*sinMu;
  State->Setlatitude(atan2(py,px));
  return 0.0;
}

