/**************************************************************************
 * fg_time.c -- data structures and routines for managing time related stuff.
 *
 * Written by Curtis Olson, started August 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef USE_FTIME
#  include <sys/timeb.h> /* for ftime() and struct timeb */
#else
#  include <unistd.h>  /* for gettimeofday() */
#  include <sys/time.h>  /* for get/setitimer, gettimeofday, struct timeval */
#endif /* USE_FTIME */

#include <Time/fg_time.h>
#include <Include/fg_constants.h>
#include <Flight/flight.h>


#define DEGHR(x)        ((x)/15.)
#define RADHR(x)        DEGHR(x*RAD_TO_DEG)

#include <Main/fg_debug.h>

struct fgTIME cur_time_params;
struct fgLIGHT cur_light_params;


/* Initialize the time dependent variables */

void fgTimeInit(struct fgTIME *t) {
    fgPrintf( FG_EVENT, FG_INFO, "Initializing Time\n");

    t->gst_diff = -9999.0;
    t->warp = (0) * 3600;
    t->warp_delta = 0;
}


/* given a date in months, mn, days, dy, years, yr, return the
 * modified Julian date (number of days elapsed since 1900 jan 0.5),
 * mjd.  Adapted from Xephem.  */

double cal_mjd (int mn, double dy, int yr) {
    static double last_mjd, last_dy;
    double mjd;
    static int last_mn, last_yr;
    int b, d, m, y;
    long c;

    if (mn == last_mn && yr == last_yr && dy == last_dy) {
	mjd = last_mjd;
	return(mjd);
    }

    m = mn;
    y = (yr < 0) ? yr + 1 : yr;
    if (mn < 3) {
	m += 12;
	y -= 1;
    }

    if (yr < 1582 || (yr == 1582 && (mn < 10 || (mn == 10 && dy < 15)))) {
	b = 0;
    } else {
	int a;
	a = y/100;
	b = 2 - a + a/4;
    }

    if (y < 0) {
	c = (long)((365.25*y) - 0.75) - 694025L;
    } else {
	c = (long)(365.25*y) - 694025L;
    }
    
    d = (int)(30.6001*(m+1));

    mjd = b + c + d + dy - 0.5;

    last_mn = mn;
    last_dy = dy;
    last_yr = yr;
    last_mjd = mjd;

    return(mjd);
}


/* given an mjd, return greenwich mean siderial time, gst */

double utc_gst (double mjd) {
    double gst;
    double day = floor(mjd-0.5)+0.5;
    double hr = (mjd-day)*24.0;
    double T, x;

    T = ((int)(mjd - 0.5) + 0.5 - J2000)/36525.0;
    x = 24110.54841 + (8640184.812866 + (0.093104 - 6.2e-6 * T) * T) * T;
    x /= 3600.0;
    gst = (1.0/SIDRATE)*hr + x;

   fgPrintf( FG_EVENT, FG_DEBUG, "  gst => %.4f\n", gst);

    return(gst);
}


/* given Julian Date and Longitude (decimal degrees West) compute and
 * return Local Sidereal Time, in decimal hours.
 *
 * Provided courtesy of ecdowney@noao.edu (Elwood Downey) 
 */

double sidereal_precise (double mjd, double lng) {
    double gst;
    double lst;

    /* printf ("Current Lst on JD %13.5f at %8.4f degrees West: ", 
       mjd + MJD0, lng); */

    /* convert to required internal units */
    lng *= DEG_TO_RAD;

    /* compute LST and print */
    gst = utc_gst (mjd);
    lst = gst - RADHR (lng);
    lst -= 24.0*floor(lst/24.0);
    /* printf ("%7.4f\n", lst); */

    /* that's all */
    return (lst);
}


/* return a courser but cheaper estimate of sidereal time */
double sidereal_course(struct tm *gmt, time_t now, double lng) {
    time_t start, start_gmt;
    struct tm mt;
    long int offset;
    double diff, part, days, hours, lst;

#ifdef USE_FTIME
    struct timeb current;
#endif /* USE_FTIME */

#ifdef WIN32
    int daylight;
    long int timezone;
#endif /* WIN32 */

    /*
    printf("  COURSE: GMT = %d/%d/%2d %d:%02d:%02d\n", 
           gmt->tm_mon, gmt->tm_mday, gmt->tm_year,
           gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
    */

    mt.tm_mon = 2;
    mt.tm_mday = 21;
    mt.tm_year = gmt->tm_year;
    mt.tm_hour = 12;
    mt.tm_min = 0;
    mt.tm_sec = 0;
    mt.tm_isdst = -1; /* let the system determine the proper time zone */

    start = mktime(&mt);

    /* printf("start1 = %ld\n", start);
       fgPrintf( FG_EVENT, FG_DEBUG, "start2 = %s", ctime(&start));
       fgPrintf( FG_EVENT, FG_DEBUG, "start3 = %ld\n", start); */

#ifndef __CYGWIN32__
    daylight = mt.tm_isdst;
#else
    /* Yargs ... I'm just hardcoding this arbitrarily so it doesn't
     * jump around */
    daylight = 0;
    fgPrintf( FG_EVENT, FG_WARN, 
	      "no daylight savings info ... being hardcoded to %d\n", daylight);
#endif

#ifdef USE_FTIME
    ftime(&current);
    timezone = current.timezone * 60;
#endif /* USE_FTIME */

    if ( daylight > 0 ) {
	daylight = 1;
    } else if ( daylight < 0 ) {
	fgPrintf( FG_EVENT, FG_WARN, 
	   "OOOPS, big time problem in fg_time.c, no daylight savings info.\n");
    }

    offset = -(timezone / 3600 - daylight);

    /* printf("  Raw time zone offset = %ld\n", timezone); */
    /* printf("  Daylight Savings = %d\n", daylight); */

    /* printf("  Local hours from GMT = %ld\n", offset); */

    start_gmt = start - timezone + (daylight * 3600);

    /* printf("  March 21 noon (CST) = %ld\n", start); */
    /* printf("  March 21 noon (GMT) = %ld\n", start_gmt); */

    diff = (now - start_gmt) / (3600.0 * 24.0);

    /* printf("  Time since 3/21/%2d GMT = %.2f\n", gmt->tm_year, diff); */

    part = fmod(diff, 1.0);
    days = diff - part;
    hours = gmt->tm_hour + gmt->tm_min/60.0 + gmt->tm_sec/3600.0;

    lst = (days - lng)/15.0 + hours - 12;

    while ( lst < 0.0 ) {
	lst += 24.0;
    }

    /* printf("  days = %.1f  hours = %.2f  lon = %.2f  lst = %.2f\n", 
	   days, hours, lng, lst); */

    return(lst);
}


/* Update the time dependent variables */

void fgTimeUpdate(fgFLIGHT *f, struct fgTIME *t) {
    double gst_precise, gst_course;

    fgPrintf( FG_EVENT, FG_BULK, "Updating time\n");

    /* get current Unix calendar time (in seconds) */
    t->warp += t->warp_delta;
    t->cur_time = time(NULL) + t->warp;
    fgPrintf( FG_EVENT, FG_BULK, 
	      "  Current Unix calendar time = %ld  warp = %ld  delta = %ld\n", 
	      t->cur_time, t->warp, t->warp_delta);

    /* get GMT break down for current time */
    t->gmt = gmtime(&t->cur_time);
    fgPrintf( FG_EVENT, FG_BULK, 
	      "  Current GMT = %d/%d/%2d %d:%02d:%02d\n", 
	      t->gmt->tm_mon+1, t->gmt->tm_mday, t->gmt->tm_year,
	      t->gmt->tm_hour, t->gmt->tm_min, t->gmt->tm_sec);

    /* calculate modified Julian date */
    t->mjd = cal_mjd ((int)(t->gmt->tm_mon+1), (double)t->gmt->tm_mday, 
	     (int)(t->gmt->tm_year + 1900));

    /* add in partial day */
    t->mjd += (t->gmt->tm_hour / 24.0) + (t->gmt->tm_min / (24.0 * 60.0)) +
	   (t->gmt->tm_sec / (24.0 * 60.0 * 60.0));

    /* convert "back" to Julian date + partial day (as a fraction of one) */
    t->jd = t->mjd + MJD0;
    fgPrintf( FG_EVENT, FG_BULK, "  Current Julian Date = %.5f\n", t->jd);

    /* printf("  Current Longitude = %.3f\n", FG_Longitude * RAD_TO_DEG); */

    /* Calculate local side real time */
    if ( t->gst_diff < -100.0 ) {
	/* first time through do the expensive calculation & cheap
           calculation to get the difference. */
      fgPrintf( FG_EVENT, FG_INFO, "  First time, doing precise gst\n");
      t->gst = gst_precise = sidereal_precise(t->mjd, 0.00);
      gst_course = sidereal_course(t->gmt, t->cur_time, 0.00);
      t->gst_diff = gst_precise - gst_course;

      t->lst = 
	sidereal_course(t->gmt, t->cur_time, -(FG_Longitude * RAD_TO_DEG))
	+ t->gst_diff;
    } else {
	/* course + difference should drift off very slowly */
	t->gst = 
	    sidereal_course(t->gmt, t->cur_time, 0.00) + t->gst_diff;
	t->lst = 
	    sidereal_course(t->gmt, t->cur_time, -(FG_Longitude * RAD_TO_DEG))
	    + t->gst_diff;
    }
    /* printf("  Current lon=0.00 Sidereal Time = %.3f\n", t->gst); */
    /* printf("  Current LOCAL Sidereal Time = %.3f (%.3f) (diff = %.3f)\n", 
           t->lst, sidereal_precise(t->mjd, -(FG_Longitude * RAD_TO_DEG)),
	   t->gst_diff); */
}


/* $Log$
/* Revision 1.35  1998/02/09 15:07:52  curt
/* Minor tweaks.
/*
 * Revision 1.34  1998/02/07 15:29:47  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.33  1998/02/02 20:54:04  curt
 * Incorporated Durk's changes.
 *
 * Revision 1.32  1998/02/01 03:39:56  curt
 * Minor tweaks.
 *
 * Revision 1.31  1998/01/27 00:48:06  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.30  1998/01/21 21:11:35  curt
 * Misc. tweaks.
 *
 * Revision 1.29  1998/01/19 19:27:20  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.28  1998/01/19 18:35:49  curt
 * Minor tweaks and fixes for cygwin32.
 *
 * Revision 1.27  1998/01/13 00:23:13  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.26  1998/01/05 18:44:36  curt
 * Add an option to advance/decrease time from keyboard.
 *
 * Revision 1.25  1997/12/31 17:46:50  curt
 * Tweaked fg_time.c to be able to use ftime() instead of gettimeofday()
 *
 * Revision 1.24  1997/12/30 22:22:42  curt
 * Further integration of event manager.
 *
 * Revision 1.23  1997/12/30 20:47:58  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.22  1997/12/30 01:38:47  curt
 * Switched back to per vertex normals and smooth shading for terrain.
 *
 * Revision 1.21  1997/12/23 04:58:39  curt
 * Tweaked the sky coloring a bit to build in structures to allow finer rgb
 * control.
 *
 * Revision 1.20  1997/12/15 23:55:06  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.19  1997/12/15 20:59:10  curt
 * Misc. tweaks.
 *
 * Revision 1.18  1997/12/12 21:41:31  curt
 * More light/material property tweaking ... still a ways off.
 *
 * Revision 1.17  1997/12/12 19:53:04  curt
 * Working on lightling and material properties.
 *
 * Revision 1.16  1997/12/11 04:43:57  curt
 * Fixed sun vector and lighting problems.  I thing the moon is now lit
 * correctly.
 *
 * Revision 1.15  1997/12/10 22:37:54  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.14  1997/12/10 01:19:52  curt
 * Tweaks for verion 0.15 release.
 *
 * Revision 1.13  1997/12/09 05:11:56  curt
 * Working on tweaking lighting.
 *
 * Revision 1.12  1997/12/09 04:25:37  curt
 * Working on adding a global lighting params structure.
 *
 * Revision 1.11  1997/11/25 19:25:40  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.10  1997/11/15 18:16:42  curt
 * minor tweaks.
 *
 * Revision 1.9  1997/11/14 00:26:50  curt
 * Transform scenery coordinates earlier in pipeline when scenery is being
 * created, not when it is being loaded.  Precalculate normals for each node
 * as average of the normals of each containing polygon so Garoude shading is
 * now supportable.
 *
 * Revision 1.8  1997/10/25 03:30:08  curt
 * Misc. tweaks.
 *
 * Revision 1.7  1997/09/23 00:29:50  curt
 * Tweaks to get things to compile with gcc-win32.
 *
 * Revision 1.6  1997/09/20 03:34:34  curt
 * Still trying to get those durned stars aligned properly.
 *
 * Revision 1.5  1997/09/16 22:14:52  curt
 * Tweaked time of day lighting equations.  Don't draw stars during the day.
 *
 * Revision 1.4  1997/09/16 15:50:31  curt
 * Working on star alignment and time issues.
 *
 * Revision 1.3  1997/09/13 02:00:08  curt
 * Mostly working on stars and generating sidereal time for accurate star
 * placement.
 *
 * Revision 1.2  1997/08/27 03:30:35  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.1  1997/08/13 21:55:59  curt
 * Initial revision.
 *
 */
