/*******************************************************************************

 Header:       FGTranslation.h
 Author:       Jon Berndt
 Date started: 12/02/98

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
12/02/98   JSB   Created

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

  The order of rotations used in this class corresponds to a 3-2-1 sequence,
  or Y-P-R, or Z-Y-X, if you prefer.
*******************************************************************************/

/*******************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGTRANSLATION_H
#define FGTRANSLATION_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGModel.h"
#include <math.h>

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGTranslation : public FGModel
{
public:
   FGTranslation(void);
   ~FGTranslation(void);

   bool Run(void);

protected:

private:
  float U, V, W;
  float P, Q, R;
  float Vt, qbar;
  float Udot, Vdot, Wdot;
  float lastUdot, lastVdot, lastWdot;
  float phi, tht, psi;
  float Fx, Fy, Fz;
  float m, g, dt;
  float alpha, beta;
  float rho;

  void GetState(void);
  void PutState(void);
};

#ifndef FDM_MAIN
extern FGTranslation* Translation;
#else
FGTranslation* Translation;
#endif

/******************************************************************************/
#endif
