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
                                        struct fgTIME t, int idx)
{
   struct CelestialCoord
		result;

    struct SunPos
    	SolarPosition;

	double
        eccAnom, r, v, ecl, actTime, R, s, ir, Nr, B, FV, ring_magn,
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

	 
    /* Let's calculate the brightness of the planet */
    R = sqrt ( xg*xg + yg*yg + zg*zg);
    s = SolarPosition.dist;
    FV = acos(  (r*r + R*R - s*s) / (2*r*R));
    FV  *= 57.29578;  /* convert radians to degrees */ 
    switch(idx)
      {
      case 2: /* mercury */
	result.magnitude = -0.36 + 5*log10( r*R ) + 0.027 * FV + 2.2E-13 * pow(FV, 6);
	break;
      case 3: /*venus */
	result.magnitude = -4.34 + 5*log10( r*R ) + 0.013 * FV + 4.2E-07 * pow(FV,3);
	break;
      case 4: /* mars */
	result.magnitude = -1.51 + 5*log10( r*R ) + 0.016 * FV;
	break;
      case 5: /* Jupiter */
	result.magnitude = -9.25 + 5*log10( r*R ) + 0.014 * FV;
	break;
      case 6: /* Saturn */
	
	ir = 0.4897394;
	Nr = 2.9585076 + 6.6672E-7*actTime;
	
	B = asin ( sin (result.Declination) * cos(ir) - cos(result.Declination) * sin (ir) * sin (result.RightAscension - Nr));
	ring_magn = -2.6 * sin (abs(B)) + 1.2 * pow(sin(B),2);
	result.magnitude = -9.0 + 5*log10( r*R ) + 0.044 * FV + ring_magn;
	break;
      case 7: /* Uranus */
	result.magnitude = -7.15 + 5*log10( r*R) + 0.001 * FV;
	break;
      case 8:  /* Neptune */
	result.magnitude = -6.90 + 5*log10 (r*R) + 0.001 *FV;
	break;
      default:
	printf("index %d out of range !!!!\n", idx);
      }
    printf("    Planet found at %f (ra), %f (dec)\n", 
	   result.RightAscension, result.Declination);
    printf("      Geocentric dist     %f\n"
           "      Heliocentric dist   %f\n"
	   "      Distance to the sun %f\n"
	   "      Phase angle         %f\n"
	   "      Brightness          %f\n", R, r, s, FV, result.magnitude);
    return result;
}



/* $Log$
/* Revision 1.4  1997/12/30 20:47:52  curt
/* Integrated new event manager with subsystem initializations.
/*
 * Revision 1.3  1997/12/30 16:36:52  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.2  1997/12/12 21:41:29  curt
 * More light/material property tweaking ... still a ways off.
 *
 * Revision 1.1  1997/10/25 03:16:10  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */


