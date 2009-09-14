/**************************************************************************
 * neptune.cxx
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

#include "neptune.hxx"

/*************************************************************************
 * Neptune::Neptune(FGTime *t)
 * Public constructor for class Neptune
 * Argument: The current time.
 * the hard coded orbital elements for Neptune are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Neptune::Neptune(FGTime *t) :
  CelestialBody(131.7806,   3.0173000E-5,
		1.7700,	   -2.550E-7,
		272.8461,  -6.027000E-6,	
		30.058260,  3.313E-8,
		0.008606,   2.150E-9,
		260.2471,   0.00599514700, t)
{
}
/*************************************************************************
 * void Neptune::updatePosition(FGTime *t, Star *ourSun)
 * 
 * calculates the current position of Neptune, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Neptune specific equation
 *************************************************************************/
void Neptune::updatePosition(FGTime *t, Star *ourSun)
{
  CelestialBody::updatePosition(t, ourSun);
  magnitude = -6.90 + 5*log10 (r*R) + 0.001 *FV;
}
