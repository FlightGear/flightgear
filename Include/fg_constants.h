// fg_constants.h -- various constant definitions
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _FG_CONSTANTS_H
#define _FG_CONSTANTS_H


/*
#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "Include/compiler.h"

#ifdef FG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  ifdef FG_MATH_EXCEPTION_CLASH
#    define exception C_exception
#  endif
#  include <math.h>
#endif

// This should be defined via autoconf in configure.in
#ifndef VERSION
#define VERSION "\"not defined\""
#endif


// Make sure PI is defined in its various forms

// PI, only PI, and nothing but PI
#ifdef M_PI
#  define  FG_PI    M_PI
#else
#  define  FG_PI    3.14159265358979323846
#endif

// 2 * PI
#define FG_2PI      6.28318530717958647692

// PI / 2
#ifdef M_PI_2
#  define  FG_PI_2  M_PI_2
#else
#  define  FG_PI_2  1.57079632679489661923
#endif

// PI / 4
#define FG_PI_4     0.78539816339744830961

#ifndef M_E
#  define M_E     2.7182818284590452354
#endif

// ONE_SECOND is pi/180/60/60, or about 100 feet at earths' equator
#define ONE_SECOND 4.848136811E-6


// Radius of Earth in kilometers at the equator.  Another source had
// 6378.165 but this is probably close enough
#define EARTH_RAD 6378.155


// Earth parameters for WGS 84, taken from LaRCsim/ls_constants.h

// Value of earth radius from [8]
#define EQUATORIAL_RADIUS_FT 20925650.    // ft
#define EQUATORIAL_RADIUS_M   6378138.12  // meter
// Radius squared
#define RESQ_FT 437882827922500.          // ft
#define RESQ_M   40680645877797.1344      // meter

// Value of earth flattening parameter from ref [8] 
//
//      Note: FP = f
//            E  = 1-f
//            EPS = sqrt(1-(1-f)^2)
//
              
#define FP    0.003352813178
#define E     0.996647186
#define EPS   0.081819221
#define INVG  0.031080997

// Time Related Parameters

#define MJD0  2415020.0
#define J2000 (2451545.0 - MJD0)
#define SIDRATE         .9972695677


// Conversions

// Degrees to Radians
#define DEG_TO_RAD       0.017453292          // deg*pi/180 = rad

// Radians to Degrees
#define RAD_TO_DEG       57.29577951          // rad*180/pi = deg

// Arc seconds to radians                     // (arcsec*pi)/(3600*180) = rad
#define ARCSEC_TO_RAD    4.84813681109535993589e-06 

// Radians to arc seconds                     // (rad*3600*180)/pi = arcsec
#define RAD_TO_ARCSEC    206264.806247096355156

// Feet to Meters
#define FEET_TO_METER    0.3048

// Meters to Feet
#define METER_TO_FEET    3.28083989501312335958  

// Meters to Nautical Miles, 1 nm = 6076.11549 feet
#define METER_TO_NM      0.00053995680

// Nautical Miles to Meters
#define NM_TO_METER      1852.0000

// Radians to Nautical Miles, 1 nm = 1/60 of a degree
#define NM_TO_RAD        0.00029088820866572159

// Nautical Miles to Radians
#define RAD_TO_NM        3437.7467707849392526

// For divide by zero avoidance, this will be close enough to zero
#define FG_EPSILON 0.0000001


// Timing constants for Flight Model updates
#define DEFAULT_TIMER_HZ 20
#define DEFAULT_MULTILOOP 6
#define DEFAULT_MODEL_HZ (DEFAULT_TIMER_HZ * DEFAULT_MULTILOOP)


// Field of view limits
#define FG_FOV_MIN 0.1
#define FG_FOV_MAX 179.9


// Maximum nodes per tile
#define MAX_NODES 2000


#endif // _FG_CONSTANTS_H


