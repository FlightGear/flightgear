//
// fg_time.cxx -- data structures and routines for managing time related stuff.
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "Include/compiler.h"

#ifdef FG_HAVE_STD_INCLUDES
#  include <cmath>
#  include <cstdio>
#  include <cstdlib>
#  include <ctime>
#else
#  include <math.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <time.h>
#endif

#ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h> // for ftime() and struct timeb
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>    // for gettimeofday()
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
#endif

#include <Astro/sky.hxx>
#include <Astro/solarsystem.hxx>
#include <Debug/logstream.hxx>
#include <Flight/flight.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Time/light.hxx>

#include "fg_time.hxx"


#define DEGHR(x)        ((x)/15.)
#define RADHR(x)        DEGHR(x*RAD_TO_DEG)

// #define MK_TIME_IS_GMT 0         // default value
// #define TIME_ZONE_OFFSET_WORK 0  // default value


fgTIME cur_time_params;


// Force an update of the sky and lighting parameters
static void local_update_sky_and_lighting_params( void ) {
    // fgSunInit();
    SolarSystem::theSolarSystem->rebuild();
    cur_light_params.Update();
    fgSkyColorsInit();
}


// Initialize the time dependent variables
void fgTimeInit(fgTIME *t) {
    FG_LOG( FG_EVENT, FG_INFO, "Initializing Time" );

    t->gst_diff = -9999.0;

    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "time offset = " << current_options.get_time_offset() );

    t->warp = current_options.get_time_offset();
    t->warp_delta = 0;

    t->pause = current_options.get_pause();
}


// given a date in months, mn, days, dy, years, yr, return the
// modified Julian date (number of days elapsed since 1900 jan 0.5),
// mjd.  Adapted from Xephem.

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


// given an mjd, return greenwich mean sidereal time, gst

double utc_gst (double mjd) {
    double gst;
    double day = floor(mjd-0.5)+0.5;
    double hr = (mjd-day)*24.0;
    double T, x;

    T = ((int)(mjd - 0.5) + 0.5 - J2000)/36525.0;
    x = 24110.54841 + (8640184.812866 + (0.093104 - 6.2e-6 * T) * T) * T;
    x /= 3600.0;
    gst = (1.0/SIDRATE)*hr + x;

    FG_LOG( FG_EVENT, FG_DEBUG, "  gst => " << gst );

    return(gst);
}


// given Julian Date and Longitude (decimal degrees West) compute and
// return Local Sidereal Time, in decimal hours.
//
// Provided courtesy of ecdowney@noao.edu (Elwood Downey) 
//

double sidereal_precise (double mjd, double lng) {
    double gst;
    double lst;

    /* printf ("Current Lst on JD %13.5f at %8.4f degrees West: ", 
       mjd + MJD0, lng); */

    // convert to required internal units
    lng *= DEG_TO_RAD;

    // compute LST and print
    gst = utc_gst (mjd);
    lst = gst - RADHR (lng);
    lst -= 24.0*floor(lst/24.0);
    // printf ("%7.4f\n", lst);

    // that's all
    return (lst);
}


// Fix up timezone if using ftime()
long int fix_up_timezone( long int timezone_orig ) {
#if !defined( HAVE_GETTIMEOFDAY ) && defined( HAVE_FTIME )
    // ftime() needs a little extra help finding the current timezone
    struct timeb current;
    ftime(&current);
    return( current.timezone * 60 );
#else
    return( timezone_orig );
#endif
}


// Return time_t for Sat Mar 21 12:00:00 GMT
//
// I believe the mktime() has a SYSV vs. BSD behavior difference.
//
// The BSD style mktime() is nice because it returns its result
// assuming you have specified the input time in GMT
//
// The SYSV style mktime() is a pain because it returns its result
// assuming you have specified the input time in your local timezone.
// Therefore you have to go to extra trouble to convert back to GMT.
//
// If you are having problems with incorrectly positioned astronomical
// bodies, this is a really good place to start looking.

time_t get_start_gmt(int year) {
    struct tm mt;

    // For now we assume that if daylight is not defined in
    // /usr/include/time.h that we have a machine with a BSD behaving
    // mktime()
#   if !defined(HAVE_DAYLIGHT)
#       define MK_TIME_IS_GMT 1
#   endif

    // timezone seems to work as a proper offset for Linux & Solaris
#   if defined( __linux__ ) || defined( __sun__ ) 
#       define TIMEZONE_OFFSET_WORKS 1
#   endif

    mt.tm_mon = 2;
    mt.tm_mday = 21;
    mt.tm_year = year;
    mt.tm_hour = 12;
    mt.tm_min = 0;
    mt.tm_sec = 0;
    mt.tm_isdst = -1; // let the system determine the proper time zone

#   if defined( MK_TIME_IS_GMT )
    return ( mktime(&mt) );
#   else // ! defined ( MK_TIME_IS_GMT )

    long int start = mktime(&mt);

    FG_LOG( FG_EVENT, FG_DEBUG, "start1 = " << start );
    // the ctime() call can screw up time progression on some versions
    // of Linux
    // fgPrintf( FG_EVENT, FG_DEBUG, "start2 = %s", ctime(&start));
    FG_LOG( FG_EVENT, FG_DEBUG, "(tm_isdst = " << mt.tm_isdst << ")" );

    timezone = fix_up_timezone( timezone );

#   if defined( TIMEZONE_OFFSET_WORKS )
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "start = " << start << ", timezone = " << timezone );
    return( start - timezone );
#   else // ! defined( TIMEZONE_OFFSET_WORKS )

    daylight = mt.tm_isdst;
    if ( daylight > 0 ) {
	daylight = 1;
    } else if ( daylight < 0 ) {
	FG_LOG( FG_EVENT, FG_WARN, 
		"OOOPS, problem in fg_time.cxx, no daylight savings info." );
    }

    long int offset = -(timezone / 3600 - daylight);

    FG_LOG( FG_EVENT, FG_DEBUG, "  Raw time zone offset = " << timezone );
    FG_LOG( FG_EVENT, FG_DEBUG, "  Daylight Savings = " << daylight );
    FG_LOG( FG_EVENT, FG_DEBUG, "  Local hours from GMT = " << offset );
    
    long int start_gmt = start - timezone + (daylight * 3600);
    
    FG_LOG( FG_EVENT, FG_DEBUG, "  March 21 noon (CST) = " << start );

    return ( start_gmt );
#   endif // ! defined( TIMEZONE_OFFSET_WORKS )
#   endif // ! defined ( MK_TIME_IS_GMT )
}

static char*
format_time( const struct tm* p, char* buf )
{
    sprintf( buf, "%d/%d/%2d %d:%02d:%02d", 
	     p->tm_mon, p->tm_mday, p->tm_year,
	     p->tm_hour, p->tm_min, p->tm_sec);
    return buf;
}

// return a courser but cheaper estimate of sidereal time
double sidereal_course(fgTIME *t, double lng) {
    struct tm *gmt;
    time_t start_gmt, now;
    double diff, part, days, hours, lst;
    char tbuf[64];

    gmt = t->gmt;
    now = t->cur_time;
    start_gmt = get_start_gmt(gmt->tm_year);

    FG_LOG( FG_EVENT, FG_DEBUG, "  COURSE: GMT = " << format_time(gmt, tbuf) );
    FG_LOG( FG_EVENT, FG_DEBUG, "  March 21 noon (GMT) = " << start_gmt );

    diff = (now - start_gmt) / (3600.0 * 24.0);
    
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "  Time since 3/21/" << gmt->tm_year << " GMT = " << diff );

    part = fmod(diff, 1.0);
    days = diff - part;
    hours = gmt->tm_hour + gmt->tm_min/60.0 + gmt->tm_sec/3600.0;

    lst = (days - lng)/15.0 + hours - 12;

    while ( lst < 0.0 ) {
	lst += 24.0;
    }

    FG_LOG( FG_EVENT, FG_DEBUG,
	    "  days = " << days << "  hours = " << hours << "  lon = " 
	    << lng << "  lst = " << lst );

    return(lst);
}


// Update time variables such as gmt, julian date, and sidereal time
void fgTimeUpdate(FGState *f, fgTIME *t) {
    double gst_precise, gst_course;

    FG_LOG( FG_EVENT, FG_DEBUG, "Updating time" );

    // get current Unix calendar time (in seconds)
    t->warp += t->warp_delta;
    t->cur_time = time(NULL) + t->warp;
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "  Current Unix calendar time = " << t->cur_time 
	    << "  warp = " << t->warp << "  delta = " << t->warp_delta );

    if ( t->warp_delta ) {
	// time is changing so force an update
	local_update_sky_and_lighting_params();
    }

    // get GMT break down for current time
    t->gmt = gmtime(&t->cur_time);
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "  Current GMT = " << t->gmt->tm_mon+1 << "/" 
	    << t->gmt->tm_mday << "/" << t->gmt->tm_year << " "
	    << t->gmt->tm_hour << ":" << t->gmt->tm_min << ":" 
	    << t->gmt->tm_sec );

    // calculate modified Julian date
    t->mjd = cal_mjd ((int)(t->gmt->tm_mon+1), (double)t->gmt->tm_mday, 
	     (int)(t->gmt->tm_year + 1900));

    // add in partial day
    t->mjd += (t->gmt->tm_hour / 24.0) + (t->gmt->tm_min / (24.0 * 60.0)) +
	   (t->gmt->tm_sec / (24.0 * 60.0 * 60.0));

    // convert "back" to Julian date + partial day (as a fraction of one)
    t->jd = t->mjd + MJD0;
    FG_LOG( FG_EVENT, FG_DEBUG, "  Current Julian Date = " << t->jd );

    // printf("  Current Longitude = %.3f\n", FG_Longitude * RAD_TO_DEG);

    // Calculate local side real time
    if ( t->gst_diff < -100.0 ) {
	// first time through do the expensive calculation & cheap
        // calculation to get the difference.
      FG_LOG( FG_EVENT, FG_INFO, "  First time, doing precise gst" );
      t->gst = gst_precise = sidereal_precise(t->mjd, 0.00);
      gst_course = sidereal_course(t, 0.00);
      t->gst_diff = gst_precise - gst_course;

      t->lst =
	  sidereal_course(t, -(f->get_Longitude() * RAD_TO_DEG)) + t->gst_diff;
    } else {
	// course + difference should drift off very slowly
	t->gst = sidereal_course(t, 0.00) + t->gst_diff;
	t->lst = sidereal_course(t, -(f->get_Longitude() * RAD_TO_DEG)) + 
	    t->gst_diff;
    }
    FG_LOG( FG_EVENT, FG_DEBUG,
	    "  Current lon=0.00 Sidereal Time = " << t->gst );
    FG_LOG( FG_EVENT, FG_DEBUG,
	    "  Current LOCAL Sidereal Time = " << t->lst << " (" 
	    << sidereal_precise(t->mjd, -(f->get_Longitude() * RAD_TO_DEG)) 
	    << ") (diff = " << t->gst_diff << ")" );
}


// $Log$
// Revision 1.29  1999/01/19 20:57:08  curt
// MacOS portability changes contributed by "Robert Puyol" <puyol@abvent.fr>
//
// Revision 1.28  1999/01/07 20:25:34  curt
// Portability changes and updates from Bernie Bright.
//
// Revision 1.27  1998/12/11 20:26:55  curt
// #include tweaks.
//
// Revision 1.26  1998/12/05 15:54:28  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.25  1998/12/05 14:21:30  curt
// Moved struct fg_timestamp to class fgTIMESTAMP and moved it's definition
// to it's own file, timestamp.hxx.
//
// Revision 1.24  1998/12/04 01:32:49  curt
// Converted "struct fg_timestamp" to "class fgTIMESTAMP" and added some
// convenience inline operators.
//
// Revision 1.23  1998/12/03 01:18:40  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.22  1998/11/16 14:00:28  curt
// FG_LOG() message tweaks.
//
// Revision 1.21  1998/11/06 21:18:26  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.20  1998/11/02 18:25:38  curt
// Check for __CYGWIN__ (b20) as well as __CYGWIN32__ (pre b20 compilers)
// Other misc. tweaks.
//
// Revision 1.19  1998/10/17 01:34:29  curt
// C++ ifying ...
//
// Revision 1.18  1998/10/02 21:36:09  curt
// Fixes to try to break through the win95/98 18.3 fps barrier.
//
// Revision 1.17  1998/09/15 04:27:49  curt
// Changes for new astro code.
//
// Revision 1.16  1998/08/29 13:11:32  curt
// Bernie Bright writes:
//   I've created some new classes to enable pointers-to-functions and
//   pointers-to-class-methods to be treated like objects.  These objects
//   can be registered with fgEVENT_MGR.
//
//   File "Include/fg_callback.hxx" contains the callback class defns.
//
//   Modified fgEVENT and fgEVENT_MGR to use the callback classes.  Also
//   some minor tweaks to STL usage.
//
//   Added file "Include/fg_stl_config.h" to deal with STL portability
//   issues.  I've added an initial config for egcs (and probably gcc-2.8.x).
//   I don't have access to Visual C++ so I've left that for someone else.
//   This file is influenced by the stl_config.h file delivered with egcs.
//
//   Added "Include/auto_ptr.hxx" which contains an implementation of the
//   STL auto_ptr class which is not provided in all STL implementations
//   and is needed to use the callback classes.
//
//   Deleted fgLightUpdate() which was just a wrapper to call
//   fgLIGHT::Update().
//
//   Modified fg_init.cxx to register two method callbacks in place of the
//   old wrapper functions.
//
// Revision 1.15  1998/08/24 20:12:16  curt
// Rewrote sidereal_course with simpler parameters.
//
// Revision 1.14  1998/08/05 00:20:07  curt
// Added a local routine to update lighting params every frame when time is
// accelerated.
//
// Revision 1.13  1998/07/30 23:48:55  curt
// Sgi build tweaks.
// Pause support.
//
// Revision 1.12  1998/07/27 18:42:22  curt
// Added a pause option.
//
// Revision 1.11  1998/07/22 21:45:37  curt
// fg_time.cxx: Removed call to ctime() in a printf() which should be harmless
//   but seems to be triggering a bug.
// light.cxx: Added code to adjust fog color based on sunrise/sunset effects
//   and view orientation.  This is an attempt to match the fog color to the
//   sky color in the center of the screen.  You see discrepancies at the
//   edges, but what else can be done?
// sunpos.cxx: Caculate local direction to sun here.  (what compass direction
//   do we need to face to point directly at sun)
//
// Revision 1.10  1998/07/13 21:02:07  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.9  1998/06/12 00:59:53  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
// Rewrote fg_time.c routine to get LST start seconds to better handle
//   Solaris, and be easier to port, and understand the GMT vs. local
//   timezone issues.
//
// Revision 1.8  1998/06/05 18:18:13  curt
// Incorporated some automake conditionals to try to support mktime() correctly
// on a wider variety of platforms.
// Added the declaration of memmove needed by the stl which apparently
// solaris only defines for cc compilations and not for c++ (__STDC__)
//
// Revision 1.7  1998/05/30 01:57:25  curt
// misc updates.
//
// Revision 1.6  1998/05/02 01:53:17  curt
// Fine tuning mktime() support because of varying behavior on different
// platforms.
//
// Revision 1.5  1998/04/28 21:45:34  curt
// Fixed a horible bug that cause the time to be *WAY* off when compiling
// with the CygWin32 compiler.  This may not yet completely address other
// Win32 compilers, but we'll have to take them on a case by case basis.
//
// Revision 1.4  1998/04/28 01:22:16  curt
// Type-ified fgTIME and fgVIEW.
//
// Revision 1.3  1998/04/25 22:06:33  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.2  1998/04/25 20:24:02  curt
// Cleaned up initialization sequence to eliminate interdependencies
// between sun position, lighting, and view position.  This creates a
// valid single pass initialization path.
//
// Revision 1.1  1998/04/24 00:52:27  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Fog color fixes.
// Separated out lighting calcs into their own file.
//
// Revision 1.41  1998/04/22 13:24:05  curt
// C++ - ifiing the code a bit.
// Starting to reorginize some of the lighting calcs to use a table lookup.
//
// Revision 1.40  1998/04/18 04:14:09  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.39  1998/04/09 18:40:14  curt
// We had unified some of the platform disparate time handling code, and
// there was a bug in timesum() which calculated a new time stamp based on
// the current time stamp + offset.  This hosed the periodic event processing
// logic because you'd never arrive at the time the event was scheduled for.
// Sky updates and lighting changes are handled via this event mechanism so
// they never changed ... it is fixed now.
//
// Revision 1.38  1998/04/08 23:35:40  curt
// Tweaks to Gnu automake/autoconf system.
//
// Revision 1.37  1998/04/03 22:12:55  curt
// Converting to Gnu autoconf system.
// Centralized time handling differences.
//
// Revision 1.36  1998/03/09 22:48:09  curt
// Debug message tweaks.
//
// Revision 1.35  1998/02/09 15:07:52  curt
// Minor tweaks.
//
// Revision 1.34  1998/02/07 15:29:47  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.33  1998/02/02 20:54:04  curt
// Incorporated Durk's changes.
//
// Revision 1.32  1998/02/01 03:39:56  curt
// Minor tweaks.
//
// Revision 1.31  1998/01/27 00:48:06  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.30  1998/01/21 21:11:35  curt
// Misc. tweaks.
//
// Revision 1.29  1998/01/19 19:27:20  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.28  1998/01/19 18:35:49  curt
// Minor tweaks and fixes for cygwin32.
//
// Revision 1.27  1998/01/13 00:23:13  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.26  1998/01/05 18:44:36  curt
// Add an option to advance/decrease time from keyboard.
//
// Revision 1.25  1997/12/31 17:46:50  curt
// Tweaked fg_time.c to be able to use ftime() instead of gettimeofday()
//
// Revision 1.24  1997/12/30 22:22:42  curt
// Further integration of event manager.
//
// Revision 1.23  1997/12/30 20:47:58  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.22  1997/12/30 01:38:47  curt
// Switched back to per vertex normals and smooth shading for terrain.
//
// Revision 1.21  1997/12/23 04:58:39  curt
// Tweaked the sky coloring a bit to build in structures to allow finer rgb
// control.
//
// Revision 1.20  1997/12/15 23:55:06  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.19  1997/12/15 20:59:10  curt
// Misc. tweaks.
//
// Revision 1.18  1997/12/12 21:41:31  curt
// More light/material property tweaking ... still a ways off.
//
// Revision 1.17  1997/12/12 19:53:04  curt
// Working on lightling and material properties.
//
// Revision 1.16  1997/12/11 04:43:57  curt
// Fixed sun vector and lighting problems.  I thing the moon is now lit
// correctly.
//
// Revision 1.15  1997/12/10 22:37:54  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.14  1997/12/10 01:19:52  curt
// Tweaks for verion 0.15 release.
//
// Revision 1.13  1997/12/09 05:11:56  curt
// Working on tweaking lighting.
//
// Revision 1.12  1997/12/09 04:25:37  curt
// Working on adding a global lighting params structure.
//
// Revision 1.11  1997/11/25 19:25:40  curt
// Changes to integrate Durk's moon/sun code updates + clean up.
//
// Revision 1.10  1997/11/15 18:16:42  curt
// minor tweaks.
//
// Revision 1.9  1997/11/14 00:26:50  curt
// Transform scenery coordinates earlier in pipeline when scenery is being
// created, not when it is being loaded.  Precalculate normals for each node
// as average of the normals of each containing polygon so Garoude shading is
// now supportable.
//
// Revision 1.8  1997/10/25 03:30:08  curt
// Misc. tweaks.
//
// Revision 1.7  1997/09/23 00:29:50  curt
// Tweaks to get things to compile with gcc-win32.
//
// Revision 1.6  1997/09/20 03:34:34  curt
// Still trying to get those durned stars aligned properly.
//
// Revision 1.5  1997/09/16 22:14:52  curt
// Tweaked time of day lighting equations.  Don't draw stars during the day.
//
// Revision 1.4  1997/09/16 15:50:31  curt
// Working on star alignment and time issues.
//
// Revision 1.3  1997/09/13 02:00:08  curt
// Mostly working on stars and generating sidereal time for accurate star
// placement.
//
// Revision 1.2  1997/08/27 03:30:35  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.1  1997/08/13 21:55:59  curt
// Initial revision.
//
