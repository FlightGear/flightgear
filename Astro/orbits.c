/**************************************************************************
 * orbits.c - calculates the orbital elements of the sun, moon and planets.
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


#include <math.h>
#include <string.h>

#include <Astro/orbits.h>

#include <Include/general.h>
#include <Time/fg_time.h>


struct OrbElements pltOrbElements[9];


double fgCalcActTime(struct fgTIME t)
{
   double
         actTime, UT;
   int year;

   /* a hack. This one introduces the 2000 problem into the program */
   year = t.gmt->tm_year + 1900;

   /* calculate the actual time, rember to add 1 to tm_mon! */
   actTime = 367 * year - 7 *
	          (year + ((t.gmt->tm_mon+1) + 9) / 12) / 4 + 275 *
	           (t.gmt->tm_mon+1) / 9 + t.gmt->tm_mday - 730530;

    UT = t.gmt->tm_hour + ((double) t.gmt->tm_min / 60);
    /*printf("UT = %f\n", UT); */
    actTime += (UT / 24.0);
    #define DEBUG 1
    #ifdef DEBUG
    /* printf("  Actual Time:\n"); */
    /* printf("  current day = %f\t", actTime); */
    /* printf("  GMT = %d, %d, %d, %d, %d, %d\n",
	   year, t.gmt->tm_mon, t.gmt->tm_mday,
	   t.gmt->tm_hour, t.gmt->tm_min, t.gmt->tm_sec); */
    #endif
    return actTime;
}

/* convert degrees to radians */
double fgDegToRad(double angle)
{
	return (angle * PIOVER180);
}

double fgCalcEccAnom(double M, double e)
{
    double
    	eccAnom, E0, E1, diff;

    eccAnom = M + e * sin(M) * (1.0 + e * cos(M));
    /* iterate to achieve a greater precision for larger eccentricities */
    if (e > 0.05)
    {
    	E0 = eccAnom;
        do
        {
        	E1 = E0 - (E0 - e * sin(E0) - M) / (1 - e * cos(E0));
            diff = fabs(E0 - E1);
            E0 = E1;
		}
        while (diff > fgDegToRad(0.001));
        return E0;
	}
    return eccAnom;
}



void fgReadOrbElements(struct OrbElements *dest, FILE *src)
{
	char line[256];
    int i,j;
    j = 0;
    do
    {
    	fgets(line, 256,src);
        for (i = 0; i < 256; i++)
        {
        	if (line[i] == '#')
            	line[i] = 0;
       	}
       	/*printf("Reading line %d\n", j++); */
    }
    while (!(strlen(line)));
    sscanf(line, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",
    		&dest->NFirst, &dest->NSec,
            &dest->iFirst, &dest->iSec,
            &dest->wFirst, &dest->wSec,
            &dest->aFirst, &dest->aSec,
            &dest->eFirst, &dest->eSec,
            &dest->MFirst, &dest->MSec);
}


void fgSolarSystemInit(struct fgTIME t)
{
   struct fgGENERAL *g;
   char path[80];
   int i;
   FILE *data;

   printf("Initializing solar system\n");

   /* build the full path name to the orbital elements database file */
   g = &general;
   path[0] = '\0';
   strcat(path, g->root_dir);
   strcat(path, "/Scenery/");
   strcat(path, "Planets.dat");

   if ( (data = fopen(path, "r")) == NULL )
   {
	    printf("Cannot open data file: '%s'\n", path);
	    return;
   }
   #ifdef DEBUG
   printf("  reading datafile %s\n", path);
   #endif

   /* for all the objects... */
   for (i = 0; i < 9; i ++)
   {
      /* ...read from the data file ... */
      fgReadOrbElements(&pltOrbElements[i], data);
      /* ...and calculate the actual values */
      fgSolarSystemUpdate(&pltOrbElements[i], t);
   }

}


void fgSolarSystemUpdate(struct OrbElements *planet, struct fgTIME t)
{
   double
         actTime;

   actTime = fgCalcActTime(t);

   /* calculate the actual orbital elements */
    planet->M = fgDegToRad(planet->MFirst + (planet->MSec * actTime));	// angle in radians
    planet->w = fgDegToRad(planet->wFirst + (planet->wSec * actTime));	// angle in radians
    planet->N = fgDegToRad(planet->NFirst + (planet->NSec * actTime));	// angle in radians
    planet->i = fgDegToRad(planet->iFirst + (planet->iSec * actTime));  // angle in radians
    planet->e = planet->eFirst + (planet->eSec * actTime);
    planet->a = planet->aFirst + (planet->aSec * actTime);
}


/* $Log$
/* Revision 1.3  1998/01/22 02:59:27  curt
/* Changed #ifdef FILE_H to #ifdef _FILE_H
/*
 * Revision 1.2  1998/01/19 19:26:58  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.1  1998/01/07 03:16:17  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.6  1997/12/30 20:47:52  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.5  1997/12/15 23:55:02  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.4  1997/12/10 22:37:51  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.3  1997/11/25 23:20:44  curt
 * Changed planets.dat Planets.dat
 *
 * Revision 1.2  1997/11/25 19:25:36  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.1  1997/10/25 03:16:10  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */
