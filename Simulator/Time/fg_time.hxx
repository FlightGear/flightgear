//
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


// Define a structure containing global time parameters
typedef struct {
    // the date/time in various forms
    // Unix "calendar" time in seconds
    time_t cur_time;

    // Break down of GMT time
    struct tm *gmt;

    // Julian date
    double jd;

    // modified Julian date
    double mjd;

    // side real time at prime meridian
    double gst;

    // local side real time
    double lst;

    // the difference between the precise sidereal time algorithm
    // result and the course result.  
    // course + diff has good accuracy for the short term
    double gst_diff;

    // An offset in seconds from the true time.  Allows us to adjust
    // the effective time of day.
    long int warp;

    // How much to change the value of warp each iteration.  Allows us
    // to make time progress faster than normal.
    long int warp_delta; 

    // Paused (0 = no, 1 = yes)
    int pause;
} fgTIME;

extern fgTIME cur_time_params;


// Update time variables such as gmt, julian date, and sidereal time
void fgTimeInit(fgTIME *t);


// Update the time dependent variables
void fgTimeUpdate(FGInterface *f, fgTIME *t);


#endif // _FG_TIME_HXX


