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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

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

#include <Debug/logstream.hxx>
#include <Astro/sky.hxx>
#include <Astro/solarsystem.hxx>
#include <FDM/flight.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Time/light.hxx>

#include "fg_time.hxx"


#define DEGHR(x)        ((x)/15.)
#define RADHR(x)        DEGHR(x*RAD_TO_DEG)


// #define MK_TIME_IS_GMT 0         // default value
// #define TIME_ZONE_OFFSET_WORK 0  // default value


FGTime::FGTime()
{
    if (cur_time_params) {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"Error: only one instance of FGTime allowed" );
	exit(-1);
    }

    cur_time_params = this;
}


FGTime::~FGTime()
{
}


// Initialize the time dependent variables (maybe I'll put this in the
// constructor later)
void FGTime::init() 
{
    FG_LOG( FG_EVENT, FG_INFO, "Initializing Time" );
    gst_diff = -9999.0;
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "time offset = " << current_options.get_time_offset() );
    time_t timeOffset = current_options.get_time_offset();
    time_t gstStart = current_options.get_start_gst();
    time_t lstStart = current_options.get_start_lst();

    // would it be better to put these sanity checks in the options
    // parsing code? (CLO)
    if (timeOffset && gstStart)
	{
	    FG_LOG( FG_EVENT, FG_ALERT, "Error: you specified both a time offset and a gst. Confused now!" );
	    current_options.usage();
	    exit(1);
	}
    if (timeOffset && lstStart)
	{
	    FG_LOG( FG_EVENT, FG_ALERT, "Error: you specified both a time offset and a lst. Confused now!" );
	    current_options.usage();
	    exit(1);
	}
    if (gstStart && lstStart)
	{
	    FG_LOG( FG_EVENT, FG_ALERT, "Error: you specified both a time offset and a lst. Confused now!" );
	    current_options.usage();
	    exit(1);
	}
    cur_time = time(NULL); 
    if (gstStart)
	warp = gstStart - cur_time;
    else if (lstStart) // I need to use the time zone info for this one, but I'll do that later.
	// Until then, Gst and LST are the same 
	{
	    warp = lstStart - cur_time;
	}
    else if (timeOffset)
	{
	    warp = timeOffset;
	}
    else
	{
	    warp = 0;
	}
    warp_delta = 0;
    pause = current_options.get_pause();
}


// given a date in months, mn, days, dy, years, yr, return the
// modified Julian date (number of days elapsed since 1900 jan 0.5),
// mjd.  Adapted from Xephem.

void FGTime::cal_mjd (int mn, double dy, int yr) 
{
    //static double last_mjd, last_dy;
    //double mjd;
    //static int last_mn, last_yr;
    int b, d, m, y;
    long c;
  
    if (mn == last_mn && yr == last_yr && dy == last_dy) {
	mjd = last_mjd;
	//return(mjd);
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
  
    //return(mjd);
}


// given an mjd, calculate greenwich mean sidereal time, gst
void FGTime::utc_gst () 
{
    double day = floor(mjd-0.5)+0.5;
    double hr = (mjd-day)*24.0;
    double T, x;

    T = ((int)(mjd - 0.5) + 0.5 - J2000)/36525.0;
    x = 24110.54841 + (8640184.812866 + (0.093104 - 6.2e-6 * T) * T) * T;
    x /= 3600.0;
    gst = (1.0/SIDRATE)*hr + x;

    FG_LOG( FG_EVENT, FG_DEBUG, "  gst => " << gst );
}


// given Julian Date and Longitude (decimal degrees West) compute
// Local Sidereal Time, in decimal hours.
//
// Provided courtesy of ecdowney@noao.edu (Elwood Downey) 

double FGTime::sidereal_precise (double lng) 
{
    double lstTmp;

    /* printf ("Current Lst on JD %13.5f at %8.4f degrees West: ", 
       mjd + MJD0, lng); */

    // convert to required internal units
    lng *= DEG_TO_RAD;

    // compute LST and print
    utc_gst();
    lstTmp = gst - RADHR (lng);
    lstTmp -= 24.0*floor(lstTmp/24.0);
    // printf ("%7.4f\n", lstTmp);

    // that's all
    return (lstTmp);
}


// return a courser but cheaper estimate of sidereal time
double FGTime::sidereal_course(double lng) 
{
    //struct tm *gmt;
    //double lstTmp;
    time_t start_gmt, now;
    double diff, part, days, hours, lstTmp;
    char tbuf[64];
  
    //gmt = t->gmt;
    //now = t->cur_time;
    now = cur_time;
    start_gmt = get_gmt(gmt->tm_year, 2, 21, 12, 0, 0);
  
    FG_LOG( FG_EVENT, FG_DEBUG, "  COURSE: GMT = " << format_time(gmt, tbuf) );
    FG_LOG( FG_EVENT, FG_DEBUG, "  March 21 noon (GMT) = " << start_gmt );
  
    diff = (now - start_gmt) / (3600.0 * 24.0);
  
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "  Time since 3/21/" << gmt->tm_year << " GMT = " << diff );
  
    part = fmod(diff, 1.0);
    days = diff - part;
    hours = gmt->tm_hour + gmt->tm_min/60.0 + gmt->tm_sec/3600.0;
  
    lstTmp = (days - lng)/15.0 + hours - 12;
  
    while ( lstTmp < 0.0 ) {
	lstTmp += 24.0;
    }
  
    FG_LOG( FG_EVENT, FG_DEBUG,
	    "  days = " << days << "  hours = " << hours << "  lon = " 
	    << lng << "  lst = " << lstTmp );
  
    return(lstTmp);
}


// Update time variables such as gmt, julian date, and sidereal time
void FGTime::update(FGInterface *f) 
{
    double gst_precise, gst_course;

    FG_LOG( FG_EVENT, FG_DEBUG, "Updating time" );

    // get current Unix calendar time (in seconds)
    warp += warp_delta;
    cur_time = time(NULL) + warp;
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "  Current Unix calendar time = " << cur_time 
	    << "  warp = " << warp << "  delta = " << warp_delta );

    if ( warp_delta ) {
	// time is changing so force an update
	local_update_sky_and_lighting_params();
    }

    // get GMT break down for current time
    gmt = gmtime(&cur_time);
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "  Current GMT = " << gmt->tm_mon+1 << "/" 
	    << gmt->tm_mday << "/" << gmt->tm_year << " "
	    << gmt->tm_hour << ":" << gmt->tm_min << ":" 
	    << gmt->tm_sec );

    // calculate modified Julian date
    // t->mjd = cal_mjd ((int)(t->gmt->tm_mon+1), (double)t->gmt->tm_mday, 
    //     (int)(t->gmt->tm_year + 1900));
    cal_mjd ((int)(gmt->tm_mon+1), (double)gmt->tm_mday, 
	     (int)(gmt->tm_year + 1900));

    // add in partial day
    mjd += (gmt->tm_hour / 24.0) + (gmt->tm_min / (24.0 * 60.0)) +
	(gmt->tm_sec / (24.0 * 60.0 * 60.0));

    // convert "back" to Julian date + partial day (as a fraction of one)
    jd = mjd + MJD0;
    FG_LOG( FG_EVENT, FG_DEBUG, "  Current Julian Date = " << jd );

    // printf("  Current Longitude = %.3f\n", FG_Longitude * RAD_TO_DEG);

    // Calculate local side real time
    if ( gst_diff < -100.0 ) {
	// first time through do the expensive calculation & cheap
        // calculation to get the difference.
	FG_LOG( FG_EVENT, FG_INFO, "  First time, doing precise gst" );
	gst_precise = gst = sidereal_precise(0.00);
	gst_course = sidereal_course(0.00);
      
	gst_diff = gst_precise - gst_course;

	lst = sidereal_course(-(f->get_Longitude() * RAD_TO_DEG)) + gst_diff;
    } else {
	// course + difference should drift off very slowly
	gst = sidereal_course( 0.00                              ) + gst_diff;
	lst = sidereal_course( -(f->get_Longitude() * RAD_TO_DEG)) + gst_diff;
    }
    FG_LOG( FG_EVENT, FG_DEBUG,
	    "  Current lon=0.00 Sidereal Time = " << gst );
    FG_LOG( FG_EVENT, FG_DEBUG,
	    "  Current LOCAL Sidereal Time = " << lst << " (" 
	    << sidereal_precise(-(f->get_Longitude() * RAD_TO_DEG)) 
	    << ") (diff = " << gst_diff << ")" );
}


/******************************************************************
 * The following are some functions that were included as FGTime
 * members, although they currently don't make use of any of the
 * class's variables. Maybe this'll change in the future
 *****************************************************************/

// Return time_t for Sat Mar 21 12:00:00 GMT
//
// On many systems it is ambiguous if mktime() assumes the input is in
// GMT, or local timezone.  To address this, a new function called
// timegm() is appearing.  It works exactly like mktime() but
// explicitely interprets the input as GMT.
//
// timegm() is available and documented under FreeBSD.  It is
// available, but completely undocumented on my current Debian 2.1
// distribution.
//
// In the absence of timegm() we have to guess what mktime() might do.
//
// Many older BSD style systems have a mktime() that assumes the input
// time in GMT.  But FreeBSD explicitly states that mktime() assumes
// local time zone
//
// The mktime() on many SYSV style systems (such as Linux) usually
// returns its result assuming you have specified the input time in
// your local timezone.  Therefore, in the absence if timegm() you
// have to go to extra trouble to convert back to GMT.
//
// If you are having problems with incorrectly positioned astronomical
// bodies, this is a really good place to start looking.

time_t FGTime::get_gmt(int year, int month, int day, int hour, int min, int sec)
{
    struct tm mt;

    mt.tm_mon = month;
    mt.tm_mday = day;
    mt.tm_year = year;
    mt.tm_hour = hour;
    mt.tm_min = min;
    mt.tm_sec = sec;
    mt.tm_isdst = -1; // let the system determine the proper time zone

    // For now we assume that if daylight is not defined in
    // /usr/include/time.h that we have a machine with a mktime() that
    // assumes input is in GMT ... this only matters if we are
    // building on a system that does not have timegm()
#if !defined(HAVE_DAYLIGHT)
#  define MK_TIME_IS_GMT 1
#endif

#if defined( HAVE_TIMEGM ) 
    return ( timegm(&mt) );
#elif defined( MK_TIME_IS_GMT )
    return ( mktime(&mt) );
#else // ! defined ( MK_TIME_IS_GMT )

    // timezone seems to work as a proper offset for Linux & Solaris
#   if defined( __linux__ ) || defined( __sun__ ) 
#       define TIMEZONE_OFFSET_WORKS 1
#   endif

    long int start = mktime(&mt);

    FG_LOG( FG_EVENT, FG_DEBUG, "start1 = " << start );
    // the ctime() call can screw up time progression on some versions
    // of Linux
    // fgPrintf( FG_EVENT, FG_DEBUG, "start2 = %s", ctime(&start));
    FG_LOG( FG_EVENT, FG_DEBUG, "(tm_isdst = " << mt.tm_isdst << ")" );

    timezone = fix_up_timezone( timezone );

#  if defined( TIMEZONE_OFFSET_WORKS )
    FG_LOG( FG_EVENT, FG_DEBUG, 
	    "start = " << start << ", timezone = " << timezone );
    return( start - timezone );
#  else // ! defined( TIMEZONE_OFFSET_WORKS )

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
#  endif // ! defined( TIMEZONE_OFFSET_WORKS )
#endif // ! defined ( MK_TIME_IS_GMT )
}

// Fix up timezone if using ftime()
long int FGTime::fix_up_timezone( long int timezone_orig ) 
{
#if !defined( HAVE_GETTIMEOFDAY ) && defined( HAVE_FTIME )
    // ftime() needs a little extra help finding the current timezone
    struct timeb current;
    ftime(&current);
    return( current.timezone * 60 );
#else
    return( timezone_orig );
#endif
}


char* FGTime::format_time( const struct tm* p, char* buf )
{
    sprintf( buf, "%d/%d/%2d %d:%02d:%02d", 
	     p->tm_mon, p->tm_mday, p->tm_year,
	     p->tm_hour, p->tm_min, p->tm_sec);
    return buf;
}


// Force an update of the sky and lighting parameters
void FGTime::local_update_sky_and_lighting_params( void ) {
    // fgSunInit();
    SolarSystem::theSolarSystem->rebuild();
    cur_light_params.Update();
    fgSkyColorsInit();
}


FGTime* FGTime::cur_time_params = 0;
