/**************************************************************************
 * mercury.cxx
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

#include "mercury.hxx"

/*************************************************************************
 * Mercury::Mercury(FGTime *t)
 * Public constructor for class Mercury
 * Argument: The current time.
 * the hard coded orbital elements for Mercury are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Mercury::Mercury(FGTime *t) :
  CelestialBody (48.33130,   3.2458700E-5,
                  7.0047,    5.00E-8,
                  29.12410,  1.0144400E-5,
                  0.3870980, 0.000000,
                  0.205635,  5.59E-10,
                  168.6562,  4.09233443680, t)
{
}
/*************************************************************************
 * void Mercury::updatePosition(FGTime *t, Star *ourSun)
 * 
 * calculates the current position of Mercury, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Mercury specific equation
 *************************************************************************/
void Mercury::updatePosition(FGTime *t, Star *ourSun)
{
  CelestialBody::updatePosition(t, ourSun);
  magnitude = -0.36 + 5*log10( r*R ) + 0.027 * FV + 2.2E-13 * pow(FV, 6); 
}


