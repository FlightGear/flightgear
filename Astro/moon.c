/**************************************************************************
 * moon.c
 * Written by Durk Talsma. Started October 1997, for the flight gear project.
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


#include <math.h>
#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Astro/orbits.h>
#include <Astro/moon.h>

#include <Aircraft/aircraft.h>
#include <Include/fg_constants.h>
#include <Include/general.h>
#include <Main/views.h>
#include <Time/fg_time.h>
#include <Main/fg_debug.h>

struct CelestialCoord moonPos;

static float xMoon, yMoon, zMoon;
static GLint moon = 0;



/* --------------------------------------------------------------
      This section contains the code that calculates the actual
      position of the moon in the night sky.
----------------------------------------------------------------*/
struct CelestialCoord fgCalculateMoon(struct OrbElements params,
                                      struct OrbElements sunParams,
                                      struct fgTIME t)
{
  struct CelestialCoord
    geocCoord, topocCoord; 
  
  double
    eccAnom, ecl, lonecl, latecl, actTime,
    xv, yv, v, r, xh, yh, zh, xg, yg, zg, xe, ye, ze,
    Ls, Lm, D, F, mpar, gclat, rho, HA, g;
  
  fgAIRCRAFT *a;
  fgFLIGHT *f;

  a = &current_aircraft;
  f = a->flight;
  
  /* calculate the angle between ecliptic and equatorial coordinate
   * system, in Radians */
  actTime = fgCalcActTime(t);
  ecl = ((DEG_TO_RAD * 23.4393) - (DEG_TO_RAD * 3.563E-7) * actTime);
  /*ecl = 0.409093 - 6.2186E-9 * actTime; */
							
  /* calculate the eccentric anomaly */
  eccAnom = fgCalcEccAnom(params.M, params.e);

  /* calculate the moon's distance (r) and  true anomaly (v) */
  xv = params.a * ( cos(eccAnom) - params.e);
  yv = params.a * ( sqrt(1.0 - params.e*params.e) * sin(eccAnom));
  v =atan2(yv, xv);
  r = sqrt(xv*xv + yv*yv);
  
  /* estimate the geocentric rectangular coordinates here */
  xh = r * (cos (params.N) * cos (v + params.w) -
	    sin (params.N) * sin (v + params.w) * cos (params.i));
  yh = r * (sin (params.N) * cos (v + params.w) +
	    cos (params.N) * sin (v + params.w) * cos (params.i));
  zh = r * (sin(v + params.w) * sin(params.i));
  
  /* calculate the ecliptic latitude and longitude here */
  lonecl = atan2( yh, xh);
  latecl = atan2( zh, sqrt( xh*xh + yh*yh));

  /* calculate a number of perturbations, i.e. disturbances caused by
   * the gravitational influence of the sun and the other mayor
   * planets. The largest of these even have their own names */
  Ls = sunParams.M + sunParams.w;
  Lm =    params.M +    params.w + params.N;
  D = Lm - Ls;
  F = Lm - params.N;
  
  lonecl += DEG_TO_RAD * (
			  - 1.274 * sin (params.M - 2*D)    			/* the Evection         */
			  + 0.658 * sin (2 * D)				        /* the Variation        */
			  - 0.186 * sin (sunParams.M)				/* the yearly variation */
			  - 0.059 * sin (2*params.M - 2*D)
			  - 0.057 * sin (params.M - 2*D + sunParams.M)
			  + 0.053 * sin (params.M + 2*D)
			  + 0.046 * sin (2*D - sunParams.M)
			  + 0.041 * sin (params.M - sunParams.M)
			  - 0.035 * sin (D)                                      /* the Parallactic Equation */
			  - 0.031 * sin (params.M + sunParams.M)
			  - 0.015 * sin (2*F - 2*D)
			  + 0.011 * sin (params.M - 4*D)
			  );
  latecl += DEG_TO_RAD * (
			  - 0.173 * sin (F - 2*D)
			  - 0.055 * sin (params.M - F - 2*D)
			  - 0.046 * sin (params.M + F - 2*D)
			  + 0.033 * sin (F + 2*D)
			  + 0.017 * sin (2 * params.M + F)
			  );
  
  r += (
	- 0.58 * cos(params.M - 2*D)
	- 0.46 * cos(2*D)
	);
  
  xg = r * cos(lonecl) * cos(latecl);
  yg = r * sin(lonecl) * cos(latecl);
  zg = r *               sin(latecl);

  xe  = xg;
  ye = yg * cos(ecl) - zg * sin(ecl);
  ze = yg * sin(ecl) + zg * cos(ecl);
  

  

  geocCoord.RightAscension = atan2(ye, xe);
  geocCoord.Declination = atan2(ze, sqrt(xe*xe + ye*ye));
  
  /* New since 25 december 1997 */
  /* Calculate the moon's topocentric position instead of it's geocentric! */

  /* calculate the moon's parrallax, i.e. the apparent size of the
   * (equatorial) radius of the Earth, as seen from the moon */
  mpar = asin( 1 / r); 
  gclat = FG_Latitude - 0.083358 * sin (2 * DEG_TO_RAD *  FG_Latitude);
  rho = 0.99883 + 0.00167 * cos(2 * DEG_TO_RAD * FG_Latitude);

  if (geocCoord.RightAscension < 0)
    geocCoord.RightAscension += (2*FG_PI);

  HA = t.lst - (3.8197186 * geocCoord.RightAscension);

  g = atan (tan(gclat) / cos( (HA / 3.8197186))); 

     

  topocCoord.RightAscension = geocCoord.RightAscension -
      mpar * rho * cos (gclat) * sin (HA) / cos (geocCoord.Declination);
  topocCoord.Declination = geocCoord.Declination -
      mpar * rho * sin (gclat) * sin (g - geocCoord.Declination) / sin (g);
  return topocCoord;
}


void fgMoonInit( void ) {
    GLfloat moonColor[4] = {0.85, 0.75, 0.35, 1.0};
    GLfloat black[4] = { 0.0, 0.0, 0.0, 1.0 };

    fgPrintf( FG_ASTRO, FG_INFO, "Initializing the Moon\n");
    fgSolarSystemUpdate(&(pltOrbElements[1]), cur_time_params);
    moonPos = fgCalculateMoon(pltOrbElements[1], pltOrbElements[0], 
			      cur_time_params);
    fgPrintf( FG_ASTRO, FG_DEBUG, 
	      "Moon found at %f (ra), %f (dec)\n", moonPos.RightAscension, 
	      moonPos.Declination);

    xMoon = 60000.0 * cos(moonPos.RightAscension) * cos(moonPos.Declination);
    yMoon = 60000.0 * sin(moonPos.RightAscension) * cos(moonPos.Declination);
    zMoon = 60000.0 * sin(moonPos.Declination);

    if (moon) {
 	xglDeleteLists (moon, 1);
    }

    moon = xglGenLists (1);
    xglNewList (moon, GL_COMPILE);
  
    xglMaterialfv (GL_FRONT, GL_AMBIENT, black);
    xglMaterialfv (GL_FRONT, GL_DIFFUSE, moonColor);
    xglPushMatrix ();
    xglTranslatef (xMoon, yMoon, zMoon);
    xglScalef (1400, 1400, 1400);
  
    glutSolidSphere (1.0, 10, 10);
    xglPopMatrix ();
    xglEndList ();
}


/* Draw the moon */
void fgMoonRender( void ) {
    xglCallList(moon);
}


/* $Log$
/* Revision 1.7  1998/02/23 19:07:54  curt
/* Incorporated Durk's Astro/ tweaks.  Includes unifying the sun position
/* calculation code between sun display, and other FG sections that use this
/* for things like lighting.
/*
 * Revision 1.6  1998/02/07 15:29:32  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.5  1998/02/02 20:53:21  curt
 * To version 0.29
 *
 * Revision 1.4  1998/01/27 00:47:46  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.3  1998/01/19 19:26:57  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.2  1998/01/19 18:40:16  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.1  1998/01/07 03:16:16  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.16  1998/01/06 01:20:24  curt
 * Tweaks to help building with MSVC++
 *
 * Revision 1.15  1998/01/05 18:44:35  curt
 * Add an option to advance/decrease time from keyboard.
 *
 * Revision 1.14  1997/12/30 20:47:50  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.13  1997/12/30 16:41:00  curt
 * Added log at end of file.
 *
 */
