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

ARGUMENTS
--------------------------------------------------------------------------------


HISTORY
--------------------------------------------------------------------------------
01/09/99   JSB   Created
*******************************************************************************/

/*******************************************************************************
DEFINES
*******************************************************************************/

/********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGUtility.h"
#include "FGState.h"
#include <math.h>

#ifdef HAVE_NCURSES
  #include <ncurses.h>
#endif

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

const float EarthRadSqrd   = 437882827922500.0;
const float OneSecond      = 4.848136811E-6;
const float Eccentricity   = 0.996647186;
const float EccentSqrd     = Eccentricity*Eccentricity;
const float EPS            = 0.081819221;

FGUtility::FGUtility()
{
}


FGUtility::~FGUtility()
{
}


float FGUtility::ToGeodetic()
{
  float GeodeticLat, Latitude, Radius, Altitude, SeaLevelR;
  float tanLat, xAlpha, muAlpha, sinmuAlpha, denom, rhoAlpha, dMu;
  float lPoint, lambdaSL, sinlambdaSL, dLambda, rAlpha;

  Latitude = State->Getlatitude();
  Radius = State->Geth() + State->EarthRad;

  if (( M_PI_2 - Latitude < OneSecond) ||
      ( M_PI_2 + Latitude < OneSecond)) { // Near a pole
    GeodeticLat = Latitude;
    SeaLevelR   = State->EarthRad * Eccentricity;
    Altitude    = Radius - SeaLevelR;
  } else {
    tanLat = tan(Latitude);
    xAlpha = Eccentricity*State->EarthRad /
                                sqrt(tanLat*tanLat + EccentSqrd);
    muAlpha = atan2(sqrt(EarthRadSqrd - xAlpha*xAlpha), Eccentricity*xAlpha);

    if (Latitude < 0.0) muAlpha = -muAlpha;

    sinmuAlpha  = sin(muAlpha);
    dLambda     = muAlpha - Latitude;
    rAlpha      = xAlpha / cos(Latitude);
    lPoint      = Radius - rAlpha;
    Altitude    = lPoint*cos(dLambda);
    denom       = sqrt(1-EPS*EPS*sinmuAlpha*sinmuAlpha);
    rhoAlpha    = State->EarthRad*(1.0 - EPS) / (denom*denom*denom);
    dMu         = atan2(lPoint*sin(dLambda),rhoAlpha + Altitude);
    State->SetGeodeticLat(muAlpha - dMu);
    lambdaSL    = atan(EccentSqrd*tan(GeodeticLat));
    sinlambdaSL = sin(lambdaSL);
    SeaLevelR   = sqrt(EarthRadSqrd / (1 + (1/EccentSqrd - 1.0)* sinlambdaSL*sinlambdaSL));
  }
  return 0.0;
}


float FGUtility:: FromGeodetic()
{
  float lambdaSL, sinlambdaSL, coslambdaSL, sinMu, cosMu, py, px;
  float Altitude, SeaLevelR;

  lambdaSL = atan(EccentSqrd*tan(State->GetGeodeticLat()));
  sinlambdaSL = sin(lambdaSL);
  coslambdaSL = cos(lambdaSL);
  sinMu = sin(State->GetGeodeticLat());
  cosMu = cos(State->GetGeodeticLat());
  SeaLevelR = sqrt(EarthRadSqrd /
             (1 + ((1/EccentSqrd)-1)*sinlambdaSL*sinlambdaSL));
  px = SeaLevelR*coslambdaSL + Altitude*cosMu;
  py = SeaLevelR*sinlambdaSL + Altitude*sinMu;
  State->Setlatitude(atan2(py,px));
  return 0.0;
}

