/*******************************************************************************

 Module:       FGAtmosphere.cpp
 Author:       Jon Berndt
               Implementation of 1959 Standard Atmosphere added by Tony Peden 
 Date started: 11/24/98
 Purpose:      Models the atmosphere
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
Models the atmosphere. The equation used below was determined by a third order
curve fit using Excel. The data is from the ICAO atmosphere model.

HISTORY
--------------------------------------------------------------------------------
11/24/98   JSB   Created
07/23/99   TP    Added implementation of 1959 Standard Atmosphere
                 Moved calculation of Mach number to FGTranslation
********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
[1]   Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGDefs.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGAtmosphere::FGAtmosphere(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAtmosphere";
  h = 0;
  Calculate(h);
  temperature = T;
  pressure    = p;
  density     = rho;
  soundspeed  = a;
}


FGAtmosphere::~FGAtmosphere()
{
}


bool FGAtmosphere::Run(void)
{
  if (!FGModel::Run()) {                 // if false then execute this Run()
    h = State->Geth();

    Calculate(h);

    temperature = T;
    pressure    = p;
    density     = rhos;
    soundspeed  = a;
    State->Seta(soundspeed);
  } else {                               // skip Run() execution this time
  }
  return false;
}


float FGAtmosphere::CalcRho(float altitude)
{
  //return (0.00237 - 7.0E-08*altitude
  //      + 7.0E-13*altitude*altitude
  //      - 2.0E-18*altitude*altitude*altitude);
  return GetDensity(altitude);
}


void FGAtmosphere::Calculate(float altitude)
{
  //see reference [1]

  float slope,reftemp,refpress,refdens;
  int i=0;
  float htab[]={0,36089,82020,154198,173882,259183,295272,344484}; //ft.

  if (altitude <= htab[0]) {
    altitude=0;
  } else if (altitude >= htab[7]){
    i = 7;
    altitude = htab[7];
  } else {
    while (htab[i+1] < altitude) {
      i++;
    }
  }

  switch(i) {
  case 0:     // sea level
    slope     = -0.0035662; // R/ft.
    reftemp   = 518.688;    // R
    refpress  = 2116.17;    // psf
    refdens   = 0.0023765;  // slugs/cubic ft.
    break;
  case 1:     // 36089 ft.
    slope     = 0;
    reftemp   = 389.988;
    refpress  = 474.1;
    refdens   = 0.0007078;
    break;
  case 2:     // 82020 ft.
    slope     = 0.00164594;
    reftemp   = 389.988;
    refpress  = 52.7838;
    refdens   = 7.8849E-5;
    break;
  case 3:     // 154198 ft.
    slope     = 0;
    reftemp   = 508.788;
    refpress  = 2.62274;
    refdens   = 3.01379E-6;
    break;
  case 4:     // 173882 ft.
    slope     = -0.00246891;
    reftemp   = 508.788;
    refpress  = 1.28428;
    refdens   = 1.47035e-06;
    break;
  case 5:     // 259183 ft.
    slope     = 0;
    reftemp   = 298.188;
    refpress  = 0.0222008;
    refdens   = 4.33396e-08;
    break;
  case 6:     // 295272 ft.
    slope     = 0.00219459;
    reftemp   = 298.188;
    refpress  = 0.00215742;
    refdens   = 4.21368e-09;
    break;
  case 7:     // 344484 ft.
    slope     = 0;
    reftemp   = 406.188;
    refpress  = 0.000153755;
    refdens   = 2.20384e-10;
    break;
  }


  if (slope == 0) {
    T = reftemp;
    p = refpress*exp(-GRAVITY/(reftemp*Reng)*(altitude-htab[i]));
    rhos = refdens*exp(-GRAVITY/(reftemp*Reng)*(altitude-htab[i]));
  } else {
    T = reftemp+slope*(altitude-htab[i]);
    p = refpress*pow(T/reftemp,-GRAVITY/(slope*Reng));
    rhos = refdens*pow(T/reftemp,-(GRAVITY/(slope*Reng)+1));
  }

  a = sqrt(SHRATIO*Reng*T);

}


float FGAtmosphere::GetTemperature(float altitude)
{
    Calculate(altitude);
    return T;
}


float FGAtmosphere::GetPressure(float altitude)
{
    Calculate(altitude);
    return p;
}

float FGAtmosphere::GetDensity(float altitude)
{
    Calculate(altitude);
    return rhos;
}


float FGAtmosphere::GetSoundSpeed(float altitude)
{
    Calculate(altitude);
    return a;
}

