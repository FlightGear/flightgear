/**************************************************************************
 * celestialBody.cxx
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

#include "celestialBody.hxx"
#include "star.hxx"
#include <Debug/logstream.hxx>

#ifdef FG_MATH_EXCEPTION_CLASH
#  define exception c_exception
#endif
#include <math.h>

/**************************************************************************
 * void CelestialBody::updatePosition(fgTIME *t, Star *ourSun)
 *
 * Basically, this member function provides a general interface for 
 * calculating the right ascension and declinaion. This function is 
 * used for calculating the planetary positions. For the planets, an 
 * overloaded member function is provided to additionally calculate the
 * planet's magnitude. 
 * The sun and moon have their own overloaded updatePosition member, as their
 * position is calculated an a slightly different manner.  
 *
 * arguments:
 * fgTIME t: provides the current time.
 * Star *ourSun: the sun's position is needed to convert heliocentric 
 *               coordinates into geocentric coordinates.
 *
 * return value: none
 *
 *************************************************************************/
void CelestialBody::updatePosition(fgTIME *t, Star *ourSun)
{
  double eccAnom, v, ecl, actTime, 
    xv, yv, xh, yh, zh, xg, yg, zg, xe, ye, ze;

  updateOrbElements(t);
  actTime = fgCalcActTime(t);

  // calcualate the angle bewteen ecliptic and equatorial coordinate system
  ecl = DEG_TO_RAD * (23.4393 - 3.563E-7 *actTime);
  
  eccAnom = fgCalcEccAnom(M, e);  //calculate the eccentric anomaly
  xv = a * (cos(eccAnom) - e);
  yv = a * (sqrt (1.0 - e*e) * sin(eccAnom));
  v = atan2(yv, xv);           // the planet's true anomaly
  r = sqrt (xv*xv + yv*yv);    // the planet's distance
  
  // calculate the planet's position in 3D space
  xh = r * (cos(N) * cos(v+w) - sin(N) * sin(v+w) * cos(i));
  yh = r * (sin(N) * cos(v+w) + cos(N) * sin(v+w) * cos(i));
  zh = r * (sin(v+w) * sin(i));

  // calculate the ecliptic longitude and latitude
  xg = xh + ourSun->getxs();
  yg = yh + ourSun->getys();
  zg = zh;

  lonEcl = atan2(yh, xh);
  latEcl = atan2(zh, sqrt(xh*xh+yh*yh));

  xe = xg;
  ye = yg * cos(ecl) - zg * sin(ecl);
  ze = yg * sin(ecl) + zg * cos(ecl);
  rightAscension = atan2(ye, xe);
  declination = atan2(ze, sqrt(xe*xe + ye*ye));
  FG_LOG(FG_GENERAL, FG_INFO, "Planet found at : " 
	 << rightAscension << " (ra), " << declination << " (dec)" );

  //calculate some variables specific to calculating the magnitude 
  //of the planet
  R = sqrt (xg*xg + yg*yg + zg*zg);
  s = ourSun->getDistance();

  // It is possible from these calculations for the argument to acos
  // to exceed the valid range for acos(). So we do a little extra
  // checking.

  double tmp = (r*r + R*R - s*s) / (2*r*R);
  if ( tmp > 1.0) { 
      tmp = 1.0;
  } else if ( tmp < -1.0) {
      tmp = -1.0;
  }

  FV = RAD_TO_DEG * acos( tmp );
};

/****************************************************************************
 * double CelestialBody::fgCalcEccAnom(double M, double e)
 * this private member calculates the eccentric anomaly of a celestial body, 
 * given its mean anomaly and eccentricity.
 * 
 * -Mean anomaly: the approximate angle between the perihelion and the current
 *  position. this angle increases uniformly with time.
 *
 * True anomaly: the actual angle between perihelion and current position.
 *
 * Eccentric anomaly: this is an auxilary angle, used in calculating the true
 * anomaly from the mean anomaly.
 * 
 * -eccentricity. Indicates the amount in which the orbit deviates from a 
 *  circle (0 = circle, 0-1, is ellipse, 1 = parabola, > 1 = hyperbola).
 *
 * This function is also known as solveKeplersEquation()
 *
 * arguments: 
 * M: the mean anomaly
 * e: the eccentricity
 *
 * return value:
 * the eccentric anomaly
 *
 ****************************************************************************/
double CelestialBody::fgCalcEccAnom(double M, double e)
{
  double 
    eccAnom, E0, E1, diff;
  
  eccAnom = M + e * sin(M) * (1.0 + e * cos (M));
  // iterate to achieve a greater precision for larger eccentricities 
  if (e > 0.05)
    {
      E0 = eccAnom;
      do
	{
	  E1 = E0 - (E0 - e * sin(E0) - M) / (1 - e *cos(E0));
	  diff = fabs(E0 - E1);
	  E0 = E1;
	}
      while (diff > (DEG_TO_RAD * 0.001));
      return E0;
    }
  return eccAnom;
}




















