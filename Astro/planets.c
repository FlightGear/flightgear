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


#include <config.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Time/fg_time.h>
#include <Astro/orbits.h>
#include <Astro/planets.h>
#include <Astro/sun.h>
#include <Include/fg_constants.h>
#include <Main/fg_debug.h>

GLint planets = 0;

struct CelestialCoord fgCalculatePlanet(struct OrbElements planet,
                                        struct OrbElements theSun,
                                        struct fgTIME t, int idx)
{
    struct CelestialCoord result;

    fgSUNPOS SolarPosition;

    double eccAnom, r, v, ecl, actTime, R, s, ir, Nr, B, FV, ring_magn,
        xv, yv, xh, yh, zh, xg, yg, zg, xe, ye, ze;

    actTime = fgCalcActTime(t);
    /*calculate the angle between ecliptic and equatorial coordinate system */
    ecl = DEG_TO_RAD * (23.4393 - 3.563E-7 * actTime);

    /* calculate the eccentric anomaly */
    eccAnom  = fgCalcEccAnom(planet.M, planet.e);

    /* calculate the planets distance (r) and true anomaly (v) */
    xv = planet.a * (cos(eccAnom) - planet.e);
    yv = planet.a * (sqrt(1.0 - planet.e*planet.e) * sin(eccAnom));
    v = atan2(yv, xv);
    r = sqrt ( xv*xv + yv*yv);
    
    /* calculate the planets position in 3-dimensional space */
    xh = r * (cos (planet.N) * cos (v + planet.w) -
 	      sin (planet.N) * sin (v + planet.w) * cos (planet.i));
    yh = r * (sin (planet.N) * cos (v + planet.w) +
 	      cos (planet.N) * sin (v + planet.w) * cos (planet.i));
    zh = r * ( sin(v+planet.w) * sin(planet.i));

    /* calculate the ecliptic longitude and latitude */

    xg = xh + solarPosition.xs;
    yg = yh + solarPosition.ys;
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
	
	B = asin (sin (result.Declination) * cos (ir) -
		  cos (result.Declination) * sin (ir) *
		  sin (result.RightAscension - Nr));
	ring_magn = -2.6 * sin (fabs(B)) + 1.2 * pow(sin(B),2);
	result.magnitude = -9.0 + 5*log10( r*R ) + 0.044 * FV + ring_magn;
	break;
      case 7: /* Uranus */
	result.magnitude = -7.15 + 5*log10( r*R) + 0.001 * FV;
	break;
      case 8:  /* Neptune */
	result.magnitude = -6.90 + 5*log10 (r*R) + 0.001 *FV;
	break;
      default:
	fgPrintf( FG_ASTRO, FG_ALERT, "index %d out of range !!!!\n", idx);
      }
    fgPrintf( FG_ASTRO, FG_DEBUG,
	      "    Planet found at %f (ra), %f (dec)\n",
	      result.RightAscension, result.Declination);
    fgPrintf( FG_ASTRO, FG_DEBUG,
	      "      Geocentric dist     %f\n"
	      "      Heliocentric dist   %f\n"
	      "      Distance to the sun %f\n"
	      "      Phase angle         %f\n"
	      "      Brightness          %f\n", R, r, s, FV, result.magnitude);
    return result;
}


void fgPlanetsInit( void )
{
  struct fgLIGHT *l;
  int i;
  struct CelestialCoord pltPos;
  double magnitude;

  l = &cur_light_params;
  /* if the display list was already built during a previous init,
     then recycle it */

  if (planets) {
      xglDeleteLists(planets, 1);
  }

  planets = xglGenLists(1);
  xglNewList( planets, GL_COMPILE );
  xglBegin( GL_POINTS );


  /* Add the planets to all four display lists */
  for ( i = 2; i < 9; i++ ) {
    fgSolarSystemUpdate(&(pltOrbElements[i]), cur_time_params);
    pltPos = fgCalculatePlanet(pltOrbElements[i], 
			       pltOrbElements[0], cur_time_params, i);
    
    magnitude = (0.0 - pltPos.magnitude) / 5.0 + 1.0;
    
    /* scale magnitudes again so they look ok */
    /* magnitude = 
       magnitude * 0.7 + (((FG_STAR_LEVELS - 1) - i) * 0.1); */

    /* the following statement could be made a little more sopisticated
       for the moment: Stick to this one */
    magnitude = magnitude * 0.7 + (3 * 0.1);
    if ( magnitude > 1.0 ) { magnitude = 1.0; }
    if ( magnitude < 0.0 ) { magnitude = 0.0; }
    
    /* Add planets to the display list, based on sun_angle and current
       magnitude It's pretty experimental... */

    if ((double) (l->sun_angle - FG_PI_2) > 
	((magnitude - 1.0) * -20 * DEG_TO_RAD))
 	{
 	    xglColor3f (magnitude, magnitude, magnitude);
 	    printf ("Sun Angle to Horizon (in Rads) = %f\n", 
		    (double) (l->sun_angle - FG_PI_2));
 	    printf ("Transformed Magnitude is :%f  %f\n", 
		    magnitude, (magnitude - 1.0) * -20 * DEG_TO_RAD);
 
 	    xglVertex3f (50000.0 * cos (pltPos.RightAscension) *
 			 cos (pltPos.Declination),
 			 50000.0 * sin (pltPos.RightAscension) *
 			 cos (pltPos.Declination),
 			 50000.0 * sin (pltPos.Declination));
 	}
  }
  xglEnd();
  xglEndList();

}


void fgPlanetsRender( void ) {
	xglCallList(planets);
}


/* $Log$
/* Revision 1.8  1998/04/03 21:52:50  curt
/* Converting to Gnu autoconf system.
/*
 * Revision 1.7  1998/02/23 19:07:55  curt
 * Incorporated Durk's Astro/ tweaks.  Includes unifying the sun position
 * calculation code between sun display, and other FG sections that use this
 * for things like lighting.
 *
 * Revision 1.6  1998/02/12 21:59:36  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.5  1998/02/03 23:20:12  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.4  1998/02/02 20:53:23  curt
 * To version 0.29
 *
 * Revision 1.3  1998/01/27 00:47:47  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.2  1998/01/19 19:26:59  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.1  1998/01/07 03:16:18  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.4  1997/12/30 20:47:52  curt
 * Integrated new event manager with subsystem initializations.
 *
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


