/**************************************************************************
 * celestialBody.hxx
 * Written by Durk Talsma. Originally started October 1997, for distribution  
 * with the FlightGear project. Version 2 was written in August and 
 * September 1998. This code is based upon algorithms and data kindly 
 * provided by Mr. Paul Schlyter. (pausch@saaf.se). 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef _CELESTIALBODY_H_
#define _CELESTIALBODY_H_

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Time/fg_time.hxx>
#include <Include/fg_constants.h>

class Star;

class CelestialBody
{
protected:              // make the data protected, in order to give the inherited
                        // classes direct access to the data
  double NFirst;       	/* longitude of the ascending node first part */
  double NSec;		/* longitude of the ascending node second part */
  double iFirst;       	/* inclination to the ecliptic first part */
  double iSec;		/* inclination to the ecliptic second part */
  double wFirst;       	/* first part of argument of perihelion */
  double wSec;		/* second part of argument of perihelion */
  double aFirst;       	/* semimayor axis first part*/
  double aSec;		/* semimayor axis second part */
  double eFirst;       	/* eccentricity first part */
  double eSec;		/* eccentricity second part */
  double MFirst;       	/* Mean anomaly first part */
  double MSec;		/* Mean anomaly second part */

  double N, i, w, a, e, M; /* the resulting orbital elements, obtained from the former */

  double rightAscension, declination;
  double r, R, s, FV;
  double magnitude;
  double lonEcl, latEcl;

  double fgCalcEccAnom(double M, double e);
  double fgCalcActTime(fgTIME *t);
  void updateOrbElements(fgTIME *t);

public:
  CelestialBody(double Nf, double Ns,
		double If, double Is,
		double wf, double ws,
		double af, double as,
		double ef, double es,
		double Mf, double Ms, fgTIME *t);
  void getPos(double *ra, double *dec);
  void getPos(double *ra, double *dec, double *magnitude);
  double getLon();
  double getLat(); 
  void updatePosition(fgTIME *t, Star *ourSun);
};

/*****************************************************************************
 * inline CelestialBody::CelestialBody
 * public constructor for a generic celestialBody object.
 * initializes the 6 primary orbital elements. The elements are:
 * N: longitude of the ascending node
 * i: inclination to the ecliptic
 * w: argument of perihelion
 * a: semi-major axis, or mean distance from the sun
 * e: eccenticity
 * M: mean anomaly
 * Each orbital element consists of a constant part and a variable part that 
 * gradually changes over time. 
 *
 * Argumetns:
 * the 13 arguments to the constructor constitute the first, constant 
 * ([NiwaeM]f) and the second variable ([NiwaeM]s) part of the orbital 
 * elements. The 13th argument is the current time. Note that the inclination
 * is written with a capital (If, Is), because 'if' is a reserved word in the 
 * C/C++ programming language.
 ***************************************************************************/ 
inline CelestialBody::CelestialBody(double Nf, double Ns,
				    double If, double Is,
				    double wf, double ws,
				    double af, double as,
				    double ef, double es,
				    double Mf, double Ms, fgTIME *t)
{
  NFirst = Nf;     NSec = Ns;
  iFirst = If;     iSec = Is;
  wFirst = wf;     wSec = ws;
  aFirst = af;     aSec = as;
  eFirst = ef;     eSec = es;
  MFirst = Mf;     MSec = Ms;
  updateOrbElements(t);
};

/****************************************************************************
 * inline void CelestialBody::updateOrbElements(fgTIME *t)
 * given the current time, this private member calculates the actual 
 * orbital elements
 *
 * Arguments: fgTIME *t: the current time:
 *
 * return value: none
 ***************************************************************************/
inline void CelestialBody::updateOrbElements(fgTIME *t)
{
  double actTime = fgCalcActTime(t);
   M = DEG_TO_RAD * (MFirst + (MSec * actTime));
   w = DEG_TO_RAD * (wFirst + (wSec * actTime));
   N = DEG_TO_RAD * (NFirst + (NSec * actTime));
   i = DEG_TO_RAD * (iFirst + (iSec * actTime));
   e = eFirst + (eSec * actTime);
   a = aFirst + (aSec * actTime);
}
/*****************************************************************************
 * inline double CelestialBody::fgCalcActTime(fgTIME *t)
 * this private member function returns the offset in days from the epoch for
 * wich the orbital elements are calculated (Jan, 1st, 2000).
 * 
 * Argument: the current time
 *
 * return value: the (fractional) number of days until Jan 1, 2000.
 ****************************************************************************/
inline double CelestialBody::fgCalcActTime(fgTIME *t)
{
  return (t->mjd - 36523.5);
}

/*****************************************************************************
 * inline void CelestialBody::getPos(double* ra, double* dec)
 * gives public access to Right Ascension and declination
 *
 ****************************************************************************/
inline void CelestialBody::getPos(double* ra, double* dec)
{
  *ra  = rightAscension;
  *dec = declination;
}

/*****************************************************************************
 * inline void CelestialBody::getPos(double* ra, double* dec, double* magnitude
 * gives public acces to the current Right ascension, declination, and 
 * magnitude
 ****************************************************************************/
inline void CelestialBody::getPos(double* ra, double* dec, double* magn)
{
  *ra = rightAscension;
  *dec = declination;
  *magn = magnitude;
}

inline double CelestialBody::getLon()
{
  return lonEcl;
}

inline double CelestialBody::getLat()
{
  return latEcl;
}

#endif // _CELESTIALBODY_H_












