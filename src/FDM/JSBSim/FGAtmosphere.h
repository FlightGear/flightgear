/*******************************************************************************

 Header:       FGAtmosphere.h
 Author:       Jon Berndt
               Implementation of 1959 Standard Atmosphere added by Tony Peden
 Date started: 11/24/98

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
11/24/98   JSB   Created
07/23/99   TP   Added implementation of 1959 Standard Atmosphere
           Moved calculation of Mach number to FGTranslation


********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGAtmosphere_H
#define FGAtmosphere_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGModel.h"

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************

[1]    Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0

*******************************************************************************
CLASS DECLARATION
*******************************************************************************/


using namespace std;

class FGAtmosphere : public FGModel
{
public:

  FGAtmosphere(FGFDMExec*);
  ~FGAtmosphere(void);
  bool Run(void);

  inline float Getrho(void) {return density;}
  float CalcRho(float altitude);

  inline float GetTemperature(void){return temperature;}
  inline float GetDensity(void)    {return density;}     // use only after Run() has been called
  inline float GetPressure(void)   {return pressure;}
  inline float GetSoundSpeed(void) {return soundspeed;}

  float GetTemperature(float altitude); //Rankine, altitude in feet
  float GetDensity(float altitude);     //slugs/ft^3
  float GetPressure(float altitude);    //lbs/ft^2
  float GetSoundSpeed(float altitude);  //ft/s

protected:

private:
  float rho;

  float h;
  float temperature,T;
  float pressure,p;
  float density,rhos;
  float soundspeed,a;
  void Calculate(float altitude);


};

/******************************************************************************/
#endif
