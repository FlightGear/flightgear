/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
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
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTRANSLATION_H
#define FGTRANSLATION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
#endif

#include "FGModel.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

#define ID_TRANSLATION "$Id$"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGTranslation : public FGModel {
public:
  FGTranslation(FGFDMExec*);
  ~FGTranslation();
  
    /** Bound Properties
        GetUVW(1): velocities/u-fps
        GetUVW(2): velocities/v-fps
        GetUVW(3): velocities/w-fps
    */
  inline double           GetUVW   (int idx) const { return vUVW(idx); }
  inline FGColumnVector3& GetUVW   (void)    { return vUVW; }
  inline FGColumnVector3& GetUVWdot(void)    { return vUVWdot; }
    /** Bound Properties
        GetUVWdot(1): accelerations/udot-fps
        GetUVWdot(2): accelerations/vdot-fps
        GetUVWdot(3): accelerations/wdot-fps
    */
  inline double           GetUVWdot(int idx) const { return vUVWdot(idx); }
  inline FGColumnVector3& GetAeroUVW (void)    { return vAeroUVW; }
    /** Bound Properties
        GetAeroUVW(1): velocities/u-aero-fps
        GetAeroUVW(2): velocities/v-aero-fps
        GetAeroUVW(3): velocities/w-aero-fps
    */
  inline double           GetAeroUVW (int idx) const { return vAeroUVW(idx); }

    /** Bound Property:  aero/alpha-rad
    */
  double Getalpha(void) const { return alpha; }
    /** Bound Property:  aero/beta-rad
    */
  double Getbeta (void) const { return beta; }
    /** Bound Property:  aero/mag-beta-rad
    */
  inline double GetMagBeta(void) const { return fabs(beta); }
    /** Bound Property:  aero/qbar-psf
    */
  double Getqbar (void) const { return qbar; }
    /** Bound Property:  aero/qbarUW-psf
    */
  double GetqbarUW (void) const { return qbarUW; }
    /** Bound Property:  aero/qbarUV-psf
    */
  double GetqbarUV (void) const { return qbarUV; }
    /** Bound Property:  velocities/vt-fps
    */
  inline double GetVt   (void) const { return Vt; }
    /** Bound Property:  velocities/mach-norm
    */
  double GetMach (void) const { return Mach; }
    /** Bound Property:  aero/alphadot-rad_sec
    */
  double Getadot (void) const { return adot; }
    /** Bound Property:  aero/betadot-rad_sec
    */
  double Getbdot (void) const { return bdot; }

    /** Bound Properties
        SetUVW(1): velocities/u-fps
        SetUVW(2): velocities/v-fps
        SetUVW(3): velocities/w-fps
    */
  void SetUVW(FGColumnVector3 tt) { vUVW = tt; }
  void SetAeroUVW(FGColumnVector3 tt) { vAeroUVW = tt; }

    /** Bound Property:  aero/alpha-rad
    */
  inline void Setalpha(double tt) { alpha = tt; }
    /** Bound Property:  aero/beta-rad
    */
  inline void Setbeta (double tt) { beta  = tt; }
    /** Bound Property:  aero/qbar-psf
    */
  inline void Setqbar (double tt) { qbar = tt; }
    /** Bound Property:  aero/qbarUW-psf
    */
  inline void SetqbarUW (double tt) { qbarUW = tt; }
    /** Bound Property:  aero/qbarUV-psf
    */
  inline void SetqbarUV (double tt) { qbarUV = tt; }
    /** Bound Property:  velocities/vt-fps
    */
  inline void SetVt   (double tt) { Vt = tt; }
    /** Bound Property:  velocities/mach-norm
    */
  inline void SetMach (double tt) { Mach=tt; }
    /** Bound Property:  aero/alphadot-rad_sec
    */
  inline void Setadot (double tt) { adot = tt; }
    /** Bound Property:  aero/betadot-rad_sec
    */
  inline void Setbdot (double tt) { bdot = tt; }

  inline void SetAB(double t1, double t2) { alpha=t1; beta=t2; }
  
  bool Run(void);

  void bind(void);
  void unbind(void);

private:
  FGColumnVector3 vUVW;
  FGColumnVector3 vUVWdot;
  FGColumnVector3 vUVWdot_prev[3];
  FGMatrix33      mVel;
  FGColumnVector3 vAeroUVW;

  double Vt, Mach;
  double qbar, qbarUW, qbarUV;
  double dt;
  double alpha, beta;
  double adot,bdot;
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

