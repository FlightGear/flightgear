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
#include <XGL/xgl.h>

#include <Time/fg_time.h>
#include <Main/views.h>
#include <Astro/orbits.h>
#include <Astro/sun.h>
#include <Main/fg_debug.h>
#include <Include/fg_constants.h>

GLint sun_obj = 0;

static struct CelestialCoord sunPos;

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
    solarPosition.dist = r;
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
    ecl = DEG_TO_RAD * (23.4393 - 3.563E-7 * actTime);			// Angle now in Rads

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
void fgSunInit( void ) {
    struct fgLIGHT *l;
    struct fgTIME *t;
    struct fgVIEW *v;  

    /* GLfloat color[4] = { 1.00, 1.00, 1.00, 1.00 }; */
    double x_2, x_4, x_8, x_10;
    GLfloat ambient;
    GLfloat amb[4];

    l = &cur_light_params;
    t = &cur_time_params;
    v = &current_view;

    fgPrintf( FG_ASTRO, FG_INFO, "  Initializing the Sun\n");

    fgSolarSystemUpdate(&(pltOrbElements[0]), cur_time_params);
    sunPos = fgCalculateSun(pltOrbElements[0], cur_time_params);
    fgPrintf( FG_ASTRO, FG_INFO, 
	      "Sun found at %f (ra), %f (dec)\n", 
	      sunPos.RightAscension, sunPos.Declination);

    xSun = 60000.0 * cos(sunPos.RightAscension) * cos(sunPos.Declination);
    ySun = 60000.0 * sin(sunPos.RightAscension) * cos(sunPos.Declination);
    zSun = 60000.0 * sin(sunPos.Declination);


    if (sun_obj) {
	xglDeleteLists(sun_obj, 1);
    }

    /* printf("First time through, creating sun display list\n"); */

    sun_obj = xglGenLists(1);
    xglNewList(sun_obj, GL_COMPILE );


    t = &cur_time_params;
    v = &current_view;
    l = &cur_light_params;

    x_2 = l->sun_angle * l->sun_angle;
    x_4 = x_2 * x_2;
    x_8 = x_4 * x_4;
    x_10 = x_8 * x_2;

    ambient = (0.4 * pow(1.1, -x_10 / 30.0));
    if ( ambient < 0.3 ) ambient = 0.3;
    if ( ambient > 1.0 ) ambient = 1.0;

    amb[0] = 0.00 + ((ambient * 6.0) - 1.0);     /* minimum val = 0.8 */
    amb[1] = 0.00 + ((ambient * 11.0) - 3.0);     /* minimum val = 0.3 */ 
    amb[2] = 0.00 + ((ambient * 12.0) - 3.6);    /* minimum val = 0.0 */ 
    amb[3] = 1.00;   

    if (amb[0] > 1.0) amb[0] = 1.0;
    if (amb[1] > 1.0) amb[1] = 1.0;
    if (amb[2] > 1.0) amb[2] = 1.0;

    fgPrintf( FG_ASTRO, FG_DEBUG, 
	    "Color of the sun: %f, %f, %f\n"
	    "Ambient value   : %f\n"
	    "Sun Angle       : %f\n" , 
	    amb[0], amb[1], amb[2], ambient, l->sun_angle);
    
    /* set lighting parameters */
    /*xglLightfv(GL_LIGHT0, GL_AMBIENT, color );
      xglLightfv(GL_LIGHT0, GL_DIFFUSE, color );
      xglMaterialfv(GL_FRONT, GL_AMBIENT, amb);
      xglMaterialfv(GL_FRONT, GL_DIFFUSE, diff); 
      xglMaterialfv(GL_FRONT, GL_SHININESS, diff);
      xglMaterialfv(GL_FRONT, GL_EMISSION, diff);
      xglMaterialfv(GL_FRONT, GL_SPECULAR, diff); */

    /* xglDisable( GL_LIGHTING ); */

    xglPushMatrix();
    xglTranslatef(xSun, ySun, zSun);
    xglScalef(1400, 1400, 1400);

    /*xglColor3f(0.85, 0.65, 0.05);*/
    xglColor3f(amb[0], amb[1], amb[2]); 
    glutSolidSphere(1.0, 10, 10);

    xglPopMatrix();

    /* xglEnable( GL_LIGHTING ); */

    xglEndList();
}


/* Draw the Sun */
void fgSunRender( void ) {
    xglCallList(sun_obj);
}


/* $Log$
/* Revision 1.5  1998/02/02 20:53:24  curt
/* To version 0.29
/*
 * Revision 1.4  1998/01/27 00:47:50  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.3  1998/01/19 19:27:00  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.2  1998/01/19 18:40:18  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.1  1998/01/07 03:16:20  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.12  1998/01/05 18:44:36  curt
 * Add an option to advance/decrease time from keyboard.
 *
 * Revision 1.11  1997/12/30 23:09:40  curt
 * Worked on winding problem without luck, so back to calling glFrontFace()
 * 3 times for each scenery area.
 *
 * Revision 1.10  1997/12/30 20:47:54  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.9  1997/12/30 16:36:54  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.8  1997/12/19 23:35:00  curt
 * Lot's of tweaking with sky rendering and lighting.
 *
 * Revision 1.7  1997/12/17 23:12:16  curt
 * Fixed so moon and sun display lists aren't recreate periodically.
 *
 * Revision 1.6  1997/12/15 23:55:04  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.5  1997/12/12 21:41:31  curt
 * More light/material property tweaking ... still a ways off.
 *
 * Revision 1.4  1997/12/10 22:37:53  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.3  1997/12/09 05:11:56  curt
 * Working on tweaking lighting.
 *
 * Revision 1.2  1997/11/25 19:25:39  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.1  1997/10/25 03:16:11  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */





