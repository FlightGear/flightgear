/**************************************************************************
 * star.cxx
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

#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <math.h>

#include "star.hxx"

/*************************************************************************
 * Star::Star(fgTIME *t)
 * Public constructor for class Star
 * Argument: The current time.
 * the hard coded orbital elements our sun are passed to 
 * CelestialBody::CelestialBody();
 * note that the word sun is avoided, in order to prevent some compilation
 * problems on sun systems 
 ************************************************************************/
Star::Star(fgTIME *t) :
  CelestialBody (0.000000,  0.0000000000,
		 0.0000,    0.00000,
		 282.9404,  4.7093500E-5,	
		 1.0000000, 0.000000,	
		 0.016709,  -1.151E-9,
		 356.0470,  0.98560025850, t)
{
}
/*************************************************************************
 * void Jupiter::updatePosition(fgTIME *t, Star *ourSun)
 * 
 * calculates the current position of our sun.
 *************************************************************************/
void Star::updatePosition(fgTIME *t)
{
  double 
    actTime, eccAnom, 
    xv, yv, v, r,
    xe, ye, ze, ecl;

  updateOrbElements(t);
  
  actTime = fgCalcActTime(t);
  ecl = DEG_TO_RAD * (23.4393 - 3.563E-7 * actTime); // Angle in Radians
  eccAnom = fgCalcEccAnom(M, e);  // Calculate the eccentric Anomaly (also known as solving Kepler's equation)
  
  xv = cos(eccAnom) - e;
  yv = sqrt (1.0 - e*e) * sin(eccAnom);
  v = atan2 (yv, xv);                   // the sun's true anomaly
  distance = r = sqrt (xv*xv + yv*yv);  // and its distance

  longitude = v + w; // the sun's true longitude
  
  // convert the sun's true longitude to ecliptic rectangular 
  // geocentric coordinates (xs, ys)
  xs = r * cos (longitude);
  ys = r * sin (longitude);

  // convert ecliptic coordinates to equatorial rectangular
  // geocentric coordinates

  xe = xs;
  ye = ys * cos (ecl);
  ze = ys * sin (ecl);

  // And finally, calculate right ascension and declination
  rightAscension = atan2 (ye, xe);
  declination = atan2 (ze, sqrt (xe*xe + ye*ye));
}
  

  
  
  
