/**************************************************************************
 * sun.c
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

#include <GL/glut.h>
#include "../Time/fg_time.h"
#include "../Main/views.h"
#include "orbits.h"
#include "sun.h"

GLint sun;

static struct CelestialCoord
    sunPos;

float xSun, ySun, zSun;

struct SunPos fgCalcSunPos(struct OrbElements params)
{
    double
    	EccAnom, lonSun,
        xv, yv, v, r;
    struct SunPos
    	solarPosition;

    /* calculate the eccentric anomaly */
    EccAnom = fgCalcEccAnom(params.M, params.e);

    /* calculate the Suns distance (r) and its true anomaly (v) */
	 xv = cos(EccAnom) - params.e;
    yv = sqrt(1.0 - params.e*params.e) * sin(EccAnom);
    v = atan2(yv, xv);
    r = sqrt(xv*xv + yv*yv);

    /* calculate the the Sun's true longitude (lonsun) */
    lonSun = v + params.w;

	/* convert true longitude and distance to ecliptic rectangular geocentric
      coordinates (xs, ys) */
    solarPosition.xs = r * cos(lonSun);
    solarPosition.ys = r * sin(lonSun);
    return solarPosition;
}


struct CelestialCoord fgCalculateSun(struct OrbElements params, struct fgTIME t)
{
	struct CelestialCoord
		result;
    struct SunPos
    	SolarPosition;
    double
    	xe, ye, ze, ecl, actTime;

    /* calculate the angle between ecliptic and equatorial coordinate system */
    actTime = fgCalcActTime(t);
    ecl = fgDegToRad(23.4393 - 3.563E-7 * actTime);			// Angle now in Rads

    /* calculate the sun's ecliptic position */
    SolarPosition = fgCalcSunPos(params);

	/* convert ecliptic coordinates to equatorial rectangular geocentric coordinates */
    xe = SolarPosition.xs;
    ye = SolarPosition.ys * cos(ecl);
    ze = SolarPosition.ys * sin(ecl);

    /* and finally... Calulate Right Ascention and Declination */
    result.RightAscension = atan2( ye, xe);
    result.Declination = atan2(ze, sqrt(xe*xe + ye*ye));
    return result;
}


/* Initialize the Sun */
void fgSunInit()
{
//   int i;
//   sun = glGenLists(1);
//   glNewList(sun, GL_COMPILE );
//	glBegin( GL_POINTS );
   fgSolarSystemUpdate(&(pltOrbElements[0]), cur_time_params);
   sunPos = fgCalculateSun(pltOrbElements[0], cur_time_params);
   #ifdef DEBUG
   printf("Sun found at %f (ra), %f (dec)\n", sunPos.RightAscension, sunPos.Declination);
   #endif
   /* give the moon a temporary color, for testing purposes */
//   glColor3f( 0.0, 1.0, 0.0);
//   glVertex3f( 190000.0 * cos(moonPos.RightAscension) * cos(moonPos.Declination),
 //              190000.0 * sin(moonPos.RightAscension) * cos(moonPos.Declination),
//   	           190000.0 * sin(moonPos.Declination) );
   //glVertex3f(0.0, 0.0, 0.0);
//   glEnd();
//   glColor3f(1.0, 1.0, 1.0);
   //xMoon = 190000.0 * cos(moonPos.RightAscension) * cos(moonPos.Declination);
   //yMoon = 190000.0 * sin(moonPos.RightAscension) * cos(moonPos.Declination);
   //zMoon = 190000.0 * sin(moonPos.Declination);
   xSun = 60000.0 * cos(sunPos.RightAscension) * cos(sunPos.Declination);
   ySun = 60000.0 * sin(sunPos.RightAscension) * cos(sunPos.Declination);
   zSun = 60000.0 * sin(sunPos.Declination);

//   glPushMatrix();
//   glTranslatef(x, y, z);
//   glScalef(16622.8, 16622.8, 16622.8);
//     glBegin(GL_TRIANGLES);
//   for (i = 0; i < 20; i++)
//      subdivide(&vdata[tindices[i][0]][0],
//                &vdata[tindices[i][1]][0],
//                &vdata[tindices[i][2]][0], 3);
//     glutSolidSphere(1.0, 25, 25);

//     glEnd();
   //glPopMatrix();
//   glEndList();
}


/* Draw the Sun */
void fgSunRender() {
    struct VIEW *v;
    struct fgTIME *t;
    GLfloat color[4] = { 0.85, 0.65, 0.05, 1.0 };
    /* double x_2, x_4, x_8, x_10; */
    /* GLfloat ambient; */
    /* GLfloat amb[3], diff[3]; */


    t = &cur_time_params;
    v = &current_view;

    /* x_2 = t->sun_angle * t->sun_angle;
    x_4 = x_2 * x_2;
    x_8 = x_4 * x_4;
    x_10 = x_8 * x_2; */

    /* ambient = (0.4 * pow(1.1, -x_10 / 30.0));
    if ( ambient < 0.3 ) ambient = 0.3;
    if ( ambient > 1.0 ) ambient = 1.0;
    amb[0] = 0.50 + ((ambient * 6.66) - 1.6);
    amb[1] = 0.00 + ((ambient * 6.66) - 1.6);
    amb[2] = 0.00 + ((ambient * 6.66) - 1.6);
    amb[3] = 0.00;
#ifdef DEBUG
    printf("Color of the sun: %f, %f, %f\n"
	   "Ambient value   : %f\n"
	   "Sun Angle       : %f\n" , amb[0], amb[1], amb[2], ambient, t->sun_angle);
#endif
    diff[0] = 0.0;
    diff[1] = 0.0;
    diff[2] = 0.0;
    diff[3] = 0.0; */

    /* set lighting parameters */
    /* glLightfv(GL_LIGHT0, GL_AMBIENT, color );
    glLightfv(GL_LIGHT0, GL_DIFFUSE, color );
    glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diff); */

    glDisable( GL_LIGHTING );
    glPushMatrix();
    glTranslatef(xSun, ySun, zSun);
    glScalef(1400, 1400, 1400);
    glColor3fv( color );
    /* glutSolidSphere(1.0, 25, 25); */
    glutSolidSphere(1.0, 10, 10);
    //glCallList(sun);
    glPopMatrix();
    glEnable( GL_LIGHTING );
}


/* $Log$
/* Revision 1.2  1997/11/25 19:25:39  curt
/* Changes to integrate Durk's moon/sun code updates + clean up.
/*
 * Revision 1.1  1997/10/25 03:16:11  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */





