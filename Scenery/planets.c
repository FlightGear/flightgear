/**************************************************************************
 * planets.c
 *
 * Written 1997 by Durk Talsma, started October, 1997.  For the flight gear
 * project.
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


#include "../Time/fg_time.h"
#include "orbits.h"
#include "planets.h"
#include "sun.h"


struct CelestialCoord fgCalculatePlanet(struct OrbElements planet,
                                        struct OrbElements theSun,
                                        struct fgTIME t)
{
   struct CelestialCoord
		result;

    struct SunPos
    	SolarPosition;

	double
        eccAnom, r, v, ecl, actTime,
        xv, yv, xh, yh, zh, xg, yg, zg, xe, ye, ze;

      actTime = fgCalcActTime(t);
      /* calculate the angle between ecliptic and equatorial coordinate system */
      ecl = fgDegToRad(23.4393 - 3.563E-7 * actTime);


    /* calculate the eccentric anomaly */
	eccAnom  = fgCalcEccAnom(planet.M, planet.e);

    /* calculate the planets distance (r) and true anomaly (v) */
    xv = planet.a * (cos(eccAnom) - planet.e);
    yv = planet.a * (sqrt(1.0 - planet.e*planet.e) * sin(eccAnom));
    v = atan2(yv, xv);
    r = sqrt ( xv*xv + yv*yv);

    /* calculate the planets position in 3-dimensional space */
    xh = r * ( cos(planet.N) * cos(v+planet.w) - sin(planet.N) * sin(v+planet.w) * cos(planet.i));
    yh = r * ( sin(planet.N) * cos(v+planet.w) + cos(planet.N) * sin(v+planet.w) * cos(planet.i));
    zh = r * ( sin(v+planet.w) * sin(planet.i));

    /* calculate the ecleptic longitude and latitude */

    /*
    lonecl = atan2(yh, xh);
    latecl = atan2(zh, sqrt ( xh*xh + yh*yh));
    */
    /* calculate the solar position */

    SolarPosition = fgCalcSunPos(theSun);
    xg = xh + SolarPosition.xs;
    yg = yh + SolarPosition.ys;
    zg = zh;

    xe = xg;
    ye = yg * cos(ecl) - zg * sin(ecl);
	 ze = yg * sin(ecl) + zg * cos(ecl);
    result.RightAscension = atan2(ye,xe);
    result.Declination = atan2(ze, sqrt(xe*xe + ye*ye));
    return result;
}


/* $Log$
/* Revision 1.2  1997/12/12 21:41:29  curt
/* More light/material property tweaking ... still a ways off.
/*
 * Revision 1.1  1997/10/25 03:16:10  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */
