/**************************************************************************
 * moon.c
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

#include "orbits.h"
#include "moon.h"

#include "../Time/fg_time.h"
#include "../GLUT/views.h"
/* #include "../Aircraft/aircraft.h"*/
#include "../general.h"


static GLint moon;

struct CelestialCoord fgCalculateMoon(struct OrbElements params,
                                      struct OrbElements sunParams,
                                      struct fgTIME t)
{
	struct CelestialCoord
          result;

    double
    	eccAnom, ecl, lonecl, latecl, actTime,
        xv, yv, v, r, xh, yh, zh, xg, yg, zg, xe, ye, ze,
        Ls, Lm, D, F;

    /* calculate the angle between ecliptic and equatorial coordinate system */
    actTime = fgCalcActTime(t);
    ecl = fgDegToRad(23.4393 - 3.563E-7 * actTime);  // in radians of course

    /* calculate the eccentric anomaly */
    eccAnom = fgCalcEccAnom(params.M, params.e);

    /* calculate the moon's distance (d) and  true anomaly (v) */
	xv = params.a * ( cos(eccAnom) - params.e);
    yv = params.a * ( sqrt(1.0 - params.e*params.e) * sin(eccAnom));
    v =atan2(yv, xv);
    r = sqrt(xv*xv + yv*yv);

    /* estimate the geocentric rectangular coordinates here */
    xh = r * (cos(params.N) * cos(v + params.w) - sin(params.N) * sin(v + params.w) * cos(params.i));
    yh = r * (sin(params.N) * cos(v + params.w) + cos(params.N) * sin(v + params.w) * cos(params.i));
    zh = r * (sin(v + params.w) * sin(params.i));

    /* calculate the ecliptic latitude and longitude here */
    lonecl = atan2( yh, xh);
    latecl = atan2( zh, sqrt( xh*xh + yh*yh));

    /* calculate a number of perturbations */
    Ls = sunParams.M + sunParams.w;
    Lm =    params.M +    params.w + params.N;
    D = Lm - Ls;
    F = Lm - params.N;

    lonecl += fgDegToRad(
    			- 1.274 * sin (params.M - 2*D)    			// the Evection
                + 0.658 * sin (2 * D)							// the Variation
                - 0.186 * sin (sunParams.M)					// the yearly variation
                - 0.059 * sin (2*params.M - 2*D)
                - 0.057 * sin (params.M - 2*D + sunParams.M)
                + 0.053 * sin (params.M + 2*D)
                + 0.046 * sin (2*D - sunParams.M)
                + 0.041 * sin (params.M - sunParams.M)
                - 0.035 * sin (D)                             // the Parallactic Equation
                - 0.031 * sin (params.M + sunParams.M)
                - 0.015 * sin (2*F - 2*D)
                + 0.011 * sin (params.M - 4*D)
              ); /* Pheeuuwwww */
    latecl += fgDegToRad(
                - 0.173 * sin (F - 2*D)
                - 0.055 * sin (params.M - F - 2*D)
                - 0.046 * sin (params.M + F - 2*D)
                + 0.033 * sin (F + 2*D)
                + 0.017 * sin (2 * params.M + F)
              );  /* Yep */

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

	result.RightAscension = atan2(ye, xe);
    result.Declination = atan2(ze, sqrt(xe*xe + ye*ye));

    return result;
}


void fgMoonInit()
{
   struct CelestialCoord
       moonPos;

   moon = glGenLists(1);
   glNewList(moon, GL_COMPILE );
	glBegin( GL_POINTS );
   moonPos = fgCalculateMoon(pltOrbElements[1], pltOrbElements[0], cur_time_params);
   printf("Moon found at %f (ra), %f (dec)\n", moonPos.RightAscension, moonPos.Declination);
   /* give the moon a temporary color, for testing purposes */
   glColor3f( 0.0, 1.0, 0.0);
   glVertex3f( 190000.0 * cos(moonPos.RightAscension) * cos(moonPos.Declination),
               190000.0 * sin(moonPos.RightAscension) * cos(moonPos.Declination),
   	         190000.0 * sin(moonPos.Declination) );
   glEnd();
   glEndList();
}

void fgMoonRender()
{
    double angle;
    static double warp = 0;
    struct VIEW *v;
    struct fgTIME *t;

    t = &cur_time_params;
    v = &current_view;


   glDisable( GL_FOG );
	glDisable( GL_LIGHTING );
	glPushMatrix();
	glTranslatef( v->view_pos.x, v->view_pos.y, v->view_pos.z );
	angle = t->gst * 15.0;  /* 15 degrees per hour rotation */
	/* warp += 1.0; */
	/* warp = 15.0; */
	warp = 0.0;
	glRotatef( (angle+warp), 0.0, 0.0, -1.0 );
	printf("Rotating moon by %.2f degrees + %.2f degrees\n",angle,warp);

	glCallList(moon);

	glPopMatrix();
	glEnable( GL_LIGHTING );
	glEnable( GL_FOG );
}


/* $Log$
/* Revision 1.1  1997/10/25 03:16:08  curt
/* Initial revision of code contributed by Durk Talsma.
/*
 */
