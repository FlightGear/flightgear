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


#include <config.h>

#include <math.h>
#include <string.h>

#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Include/general.h>
#include <Time/fg_time.h>

#include "orbits.hxx"


struct OrbElements pltOrbElements[9];


double fgCalcActTime(struct fgTIME t)
{
  return (t.mjd - 36523.5);
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
        while (diff > (DEG_TO_RAD * 0.001));
        return E0;
	}
    return eccAnom;
}


/* This function assumes that if the FILE ptr is valid that the
   contents will be valid. Should we check the file for validity? */
  
/* Sounds like a good idea to me. What type of checks are you thinking
   of, other than feof(FILE*)? That's currently the only check I can
   think of (Durk) */

int fgReadOrbElements(struct OrbElements *dest, FILE *src)
{
	char line[256];
    int i,j;
    j = 0;
    do
    {
 	if (feof (src)) {
	    fgPrintf (FG_ASTRO, FG_ALERT,
		      "End of file found while reading planetary positions:\n");
 	    return 0;
 	}
 
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

    return(1);
}


int fgSolarSystemInit(struct fgTIME t)
{
    fgGENERAL *g;
    char path[80];
    int i;
    FILE *data;
    int ret_val = 0;

    fgPrintf( FG_ASTRO, FG_INFO, "Initializing solar system\n");

   /* build the full path name to the orbital elements database file */
    g = &general;
    path[0] = '\0';
    strcat(path, g->root_dir);
    strcat(path, "/Scenery/");
    strcat(path, "Planets.dat");

    if ( (data = fopen(path, "r")) == NULL )
    {
	    fgPrintf( FG_ASTRO, FG_ALERT,
		      "Cannot open data file: '%s'\n", path);
    } else {
	/* printf("  reading datafile %s\n", path); */
	fgPrintf( FG_ASTRO, FG_INFO, "  reading datafile %s\n", path);

	/* for all the objects... */
	for (i = 0; i < 9; i ++)
	    {
		/* ...read from the data file ... */
		if (!(fgReadOrbElements (&pltOrbElements[i], data))) {
		    ret_val = 0;
		}
		/* ...and calculate the actual values */
		fgSolarSystemUpdate(&pltOrbElements[i], t);
	    }
	ret_val = 1;
    }
    return ret_val;
}


void fgSolarSystemUpdate(struct OrbElements *planet, struct fgTIME t)
{
   double
         actTime;

   actTime = fgCalcActTime(t);

   /* calculate the actual orbital elements */
   planet->M = DEG_TO_RAD * (planet->MFirst + (planet->MSec * actTime));
   planet->w = DEG_TO_RAD * (planet->wFirst + (planet->wSec * actTime));
   planet->N = DEG_TO_RAD * (planet->NFirst + (planet->NSec * actTime));
   planet->i = DEG_TO_RAD * (planet->iFirst + (planet->iSec * actTime));
   planet->e = planet->eFirst + (planet->eSec * actTime);
   planet->a = planet->aFirst + (planet->aSec * actTime);
}


/* $Log$
/* Revision 1.1  1998/04/22 13:21:29  curt
/* C++ - ifing the code a bit.
/*
 * Revision 1.10  1998/04/18 04:13:57  curt
 * Moved fg_debug.c to it's own library.
 *
 * Revision 1.9  1998/03/14 00:27:12  curt
 * Updated fgGENERAL to a "type" of struct.
 *
 * Revision 1.8  1998/02/23 19:07:55  curt
 * Incorporated Durk's Astro/ tweaks.  Includes unifying the sun position
 * calculation code between sun display, and other FG sections that use this
 * for things like lighting.
 *
 * Revision 1.7  1998/02/12 21:59:33  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.6  1998/02/03 23:20:11  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.5  1998/02/02 20:53:22  curt
 * To version 0.29
 *
 * Revision 1.4  1998/01/27 00:47:47  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
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
