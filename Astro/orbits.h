/**************************************************************************
 * orbits.h - calculates the orbital elements of the sun, moon and planets.
 *            For inclusion in flight gear
 *
 * Written 1997 by Durk Talsma, started October 19, 1997.
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


#ifndef _ORBITS_H
#define _ORBITS_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


#include <stdio.h>
#include <math.h>

#include <Time/fg_time.h>



//#define STANDARDEPOCH 2000

typedef struct {
    double xs;
    double ys;
    double dist;
    double lonSun;
} fgSUNPOS;

extern fgSUNPOS solarPosition;


struct OrbElements {
    double NFirst;		/* longitude of the ascending node first part */
    double NSec;		/* longitude of the ascending node second part */
    double iFirst;		/* inclination to the ecliptic first part */
    double iSec;		/* inclination to the ecliptic second part */
    double wFirst;		/* first part of argument of perihelion */
    double wSec;		/* second part of argument of perihelion */
    double aFirst;		/* semimayor axis first part*/
    double aSec;		/* semimayor axis second part */
    double eFirst;		/* eccentricity first part */
    double eSec;		/* eccentricity second part */
    double MFirst;		/* Mean anomaly first part */
    double MSec;		/* Mean anomaly second part */

    double N, i, w, a, e, M;	/* the resultant orbital elements, obtained from the former */
};

struct CelestialCoord {
    double RightAscension;
    double Declination;
    double distance;
    double magnitude;
};


double fgCalcEccAnom(double M, double e);
double fgCalcActTime(struct fgTIME t);

int fgReadOrbElements (struct OrbElements *dest, FILE * src);
int  fgSolarSystemInit(struct fgTIME t);
void fgSolarSystemUpdate(struct OrbElements *planets, struct fgTIME t);


#ifdef __cplusplus
}
#endif


#endif /* _ORBITS_H */


/* $Log$
/* Revision 1.7  1998/04/21 17:02:31  curt
/* Prepairing for C++ integration.
/*
 * Revision 1.6  1998/02/23 19:07:55  curt
 * Incorporated Durk's Astro/ tweaks.  Includes unifying the sun position
 * calculation code between sun display, and other FG sections that use this
 * for things like lighting.
 *
 * Revision 1.5  1998/02/12 21:59:35  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.4  1998/02/02 20:53:22  curt
 * To version 0.29
 *
 * Revision 1.3  1998/01/22 02:59:27  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.2  1998/01/19 19:26:58  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.1  1998/01/07 03:16:17  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.2  1997/12/30 16:36:52  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.1  1997/10/25 03:16:10  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */
