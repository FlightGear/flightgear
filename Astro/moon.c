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
static GLint moon;

/*
static GLfloat vdata[12][3] =
{
   {-X, 0.0, Z }, { X, 0.0, Z }, {-X, 0.0, -Z}, {X, 0.0, -Z },
   { 0.0, Z, X }, { 0.0, Z, -X}, {0.0, -Z, -X}, {0.0, -Z, -X},
   { Z, X, 0.0 }, { -Z, X, 0.0}, {Z, -X, 0.0 }, {-Z, -X, 0.0}
};

static GLuint tindices[20][3] =
{
   {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},
   {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
   {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},
   {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11}
};*/

/* -------------------------------------------------------------
      This section contains the code that generates a yellow
      Icosahedron. It's under development... (of Course)
______________________________________________________________*/
/*
void NormalizeVector(float v[3])
{
   GLfloat d = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
   if (d == 0.0)
   {
      printf("zero length vector\n");
      return;
   }
   v[0] /= d;
   v[1] /= d;
   v[2] /= d;
}

void drawTriangle(float *v1, float *v2, float *v3)
{
   xglBegin(GL_POLYGON);
   //xglBegin(GL_POINTS);
      xglNormal3fv(v1);
      xglVertex3fv(v1);
      xglNormal3fv(v2);
      xglVertex3fv(v2);
      xglNormal3fv(v3);
      xglVertex3fv(v3);
   xglEnd();
}

void subdivide(float *v1, float *v2, float *v3, long depth)
{
   GLfloat v12[3], v23[3], v31[3];
   GLint i;

   if (!depth)
   {
     drawTriangle(v1, v2, v3);
     return;
   }
   for (i = 0; i < 3; i++)
   {
       v12[i] = (v1[i] + v2[i]);
       v23[i] = (v2[i] + v3[i]);
       v31[i] = (v3[i] + v1[i]);
   }
   NormalizeVector(v12);
   NormalizeVector(v23);
   NormalizeVector(v31);
   subdivide(v1, v12, v31, depth - 1);
   subdivide(v2, v23, v12, depth - 1);
   subdivide(v3, v31, v23, depth - 1);
   subdivide(v12, v23, v31,depth - 1);

} */
/*
void display(void)
{
   int i;
   xglClear(GL_COLOR_BUFFER_BIT);
   xglPushMatrix();
   xglRotatef(spin, 0.0, 0.0, 0.0);
   xglColor3f(1.0, 1.0, 0.0);
//   xglBegin(GL_LINE_LOOP);
   for (i = 0; i < 20; i++)
   {

       //xglVertex3fv(&vdata[tindices[i][0]][0]);
       //xglVertex3fv(&vdata[tindices[i][1]][0]);
       //xglVertex3fv(&vdata[tindices[i][2]][0]);

       subdivide(&vdata[tindices[i][0]][0],
                 &vdata[tindices[i][1]][0],
                 &vdata[tindices[i][2]][0], 3);


   }
//   xglEnd();
  // xglFlush();
  xglPopMatrix();
  glutSwapBuffers();
} */

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
  
  struct fgAIRCRAFT *a;
  struct fgFLIGHT *f;

  a = &current_aircraft;
  f = &a->flight;
  
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
	); /* Ok! */

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

  mpar = asin( 1 / r); /* calculate the moon's parrallax, i.e. the apparent size of the
			  (equatorial) radius of the Earth, as seen from the moon */
  gclat = FG_Latitude - 0.083358 * sin (2 * fgDegToRad( FG_Latitude));
  rho = 0.99883 + 0.00167 * cos(2 * fgDegToRad(FG_Latitude));

  if (geocCoord.RightAscension < 0)
    geocCoord.RightAscension += (2*FG_PI);

  HA = t.lst - (3.8197186 * geocCoord.RightAscension);

  g = atan (tan(gclat) / cos( (HA / 3.8197186))); 

     

  topocCoord.RightAscension = geocCoord.RightAscension - mpar * rho * cos(gclat) * sin(HA) / cos(geocCoord.Declination);
  topocCoord.Declination = geocCoord.Declination - mpar * rho * sin(gclat) * sin(g - geocCoord.Declination) / sin(g);
  return topocCoord;
}


void fgMoonInit( void ) {
    struct fgLIGHT *l;
    static int dl_exists = 0;

    fgPrintf( FG_ASTRO, FG_INFO, "Initializing the Moon\n");

    l = &cur_light_params;

    /* position the moon */
    fgSolarSystemUpdate(&(pltOrbElements[1]), cur_time_params);
    moonPos = fgCalculateMoon(pltOrbElements[1], pltOrbElements[0], 
			      cur_time_params);
#ifdef DEBUG
    fgPrintf( FG_ASTRO, FG_DEBUG, 
	      "Moon found at %f (ra), %f (dec)\n", moonPos.RightAscension, 
	      moonPos.Declination);
#endif

    xMoon = 60000.0 * cos(moonPos.RightAscension) * cos(moonPos.Declination);
    yMoon = 60000.0 * sin(moonPos.RightAscension) * cos(moonPos.Declination);
    zMoon = 60000.0 * sin(moonPos.Declination);

    if ( !dl_exists ) {
	dl_exists = 1;

	/* printf("First time through, creating moon display list\n"); */

	moon = xglGenLists(1);
	xglNewList(moon, GL_COMPILE );

	/* xglMaterialfv(GL_FRONT, GL_AMBIENT, l->scene_clear);
	   xglMaterialfv(GL_FRONT, GL_DIFFUSE, moon_color); */


	glutSolidSphere(1.0, 10, 10);

	xglEndList();
    }
}


/* Draw the moon */
void fgMoonRender( void ) {
    struct fgLIGHT *l;
    GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };

    /* printf("Rendering moon\n"); */

    l = &cur_light_params;

    xglMaterialfv(GL_FRONT, GL_AMBIENT, l->sky_color );
    xglMaterialfv(GL_FRONT, GL_DIFFUSE, white);

    xglPushMatrix();
    xglTranslatef(xMoon, yMoon, zMoon);
    xglScalef(1400, 1400, 1400);

    xglCallList(moon);

    xglPopMatrix();
}


/* $Log$
/* Revision 1.4  1998/01/27 00:47:46  curt
/* Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
/* system and commandline/config file processing code.
/*
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
