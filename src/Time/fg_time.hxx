// fg_time.hxx -- data structures and routines for managing time related stuff.
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


#ifndef _FG_TIME_HXX
#define _FG_TIME_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>

#include "Include/compiler.h"
#ifdef FG_HAVE_STD_INCLUDES
#  include <ctime>
#else
#  include <time.h>
#endif

#include <FDM/flight.hxx>

#include "timezone.h"
#include "lowleveltime.h"

// Define a structure containing global time parameters
class FGTime {

private:
    // tzContainer stores all the current Timezone control points/
    TimezoneContainer* tzContainer;

    //Store the current local timezone name;
    char *zonename;

    // Unix "calendar" time in seconds
    time_t cur_time;

    // Break down of GMT time
    struct tm *gmt;

    // Julian date
    double jd;

    // modified Julian date
    double mjd;

    double last_mjd, last_dy;
    int last_mn, last_yr;

    // side real time at prime meridian
    double gst;

    // local side real time
    double lst;

    // the difference between the precise sidereal time algorithm
    // result and the course result.  course + diff has good accuracy
    // for the short term
    double gst_diff;

    // An offset in seconds from the true time.  Allows us to adjust
    // the effective time of day.
    long int warp;

    // How much to change the value of warp each iteration.  Allows us
    // to make time progress faster than normal.
    long int warp_delta;

    // Paused?
    bool pause;
                                     
    void local_update_sky_and_lighting_params( void );

public:

    FGTime();
    ~FGTime();

    inline double getMjd() const { return mjd; };
    inline double getLst() const { return lst; };
    inline double getGst() const { return gst; };
    inline time_t get_cur_time() const { return cur_time; };
    inline struct tm* getGmt()const { return gmt; };
    inline bool getPause() const { return pause; };
  
    void adjust_warp(int val) { warp += val; };
    void adjust_warp_delta(int val) { warp_delta += val; };
    void togglePauseMode() { pause = !pause; }; 

    // Initialize the time dependent variables
    void init(FGInterface *f);

    // Update the time dependent variables
    void update(FGInterface *f);

    void cal_mjd (int mn, double dy, int yr);
    void utc_gst(); 
    double sidereal_precise (double lng);
    double sidereal_course(double lng); 
    static FGTime *cur_time_params;

    // Some other stuff which were changed to FGTime members on
    // questionable grounds -:)
    // time_t get_start_gmt(int year);
    time_t get_gmt(int year, int month, int day, 
		   int hour, int minute, int second);
    time_t get_gmt(struct tm* the_time);
  
    char* format_time( const struct tm* p, char* buf );
    long int fix_up_timezone( long int timezone_orig );
};


inline time_t FGTime::get_gmt(struct tm* the_time) // this is just a wrapper
{
  //printf("Using: %24s as input\n", asctime(the_time));
  return get_gmt(the_time->tm_year,
	  the_time->tm_mon,
	  the_time->tm_mday,
	  the_time->tm_hour,
	  the_time->tm_min,
	  the_time->tm_sec);
}


#endif // _FG_TIME_HXX
