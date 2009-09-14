/**************************************************************************
 * saturn.cxx
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
 **************************************************************************/

#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <math.h>

#include "saturn.hxx"

/*************************************************************************
 * Saturn::Saturn(FGTime *t)
 * Public constructor for class Saturn
 * Argument: The current time.
 * the hard coded orbital elements for Saturn are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Saturn::Saturn(FGTime *t) :
  CelestialBody(113.6634,   2.3898000E-5,
		2.4886,	   -1.081E-7,
		339.3939,   2.9766100E-5,
		9.5547500,  0.000000,
		0.055546,  -9.499E-9,
		316.9670,   0.03344422820, t)
{
}

/*************************************************************************
 * void Saturn::updatePosition(FGTime *t, Star *ourSun)
 * 
 * calculates the current position of Saturn, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Saturn specific equation
 *************************************************************************/
void Saturn::updatePosition(FGTime *t, Star *ourSun)
{
  CelestialBody::updatePosition(t, ourSun);
  
  double actTime = fgCalcActTime(t);
  double ir = 0.4897394;
  double Nr = 2.9585076 + 6.6672E-7*actTime;
  double B = asin (sin(declination) * cos(ir) - 
		   cos(declination) * sin(ir) *
		   sin(rightAscension - Nr));
  double ring_magn = -2.6 * sin(fabs(B)) + 1.2 * pow(sin(B), 2);
  magnitude = -9.0 + 5*log10(r*R) + 0.044 * FV + ring_magn;
}

