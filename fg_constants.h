/**************************************************************************
 * constants.h -- various constant definitions
 *
 * Written by Curtis Olson, started July 1997.
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


#ifndef _FG_CONSTANTS_H
#define _FG_CONSTANTS_H


#include <math.h>


/* This should be defined via autoconf in configure.in */
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


/* Radius of Earth in kilometers at the equator.  Another source had
 * 6378.165 but this is probably close enough */
#define EARTH_RAD 6378.155


/* Earth parameters for WGS 84, taken from LaRCsim/ls_constants.h */

/* Value of earth radius from [8] */
#define EQUATORIAL_RADIUS_FT 20925650.    /* ft */
#define EQUATORIAL_RADIUS_KM 6378138.12   /* meter */
/* Radius squared */
#define RESQ_FT 437882827922500.          /* ft */
#define RESQ_KM 40680645877797.1344       /* meter */

/* Value of earth flattening parameter from ref [8] 
 *
 *      Note: FP = f
 *            E  = 1-f
 *            EPS = sqrt(1-(1-f)^2)
 */
              
#define FP    0.003352813178
#define E     0.996647186
#define EPS   0.081819221
#define INVG  0.031080997


/* Time Related Parameters */

#define MJD0  2415020.0
#define J2000 (2451545.0 - MJD0)
#define SIDRATE         .9972695677


/* Conversions */

/* Degrees to Radians */
#define DEG_TO_RAD       0.017453292          /* deg*pi/180 = rad */

/* Radians to Degrees */
#define RAD_TO_DEG       57.29577951          /* rad*180/pi = deg */

/* Arc seconds to radians */                  /* (arcsec*pi)/(3600*180) = rad */
#define ARCSEC_TO_RAD 4.84813681109535993589e-06 

/* Radians to arc seconds */                  /* (rad*3600*180)/pi = arcsec */
#define RAD_TO_ARCSEC 206264.806247096355156

/* Feet to Meters */
#define FEET_TO_METER    0.3048

/* Meters to Feet */
#define METER_TO_FEET    3.28083989501312335958  


/* For divide by zero avoidance, this will be close enough to zero */
#define FG_EPSILON 0.0000001


/* Timing constants for Flight Model updates */
#define DEFAULT_TIMER_HZ 20
#define DEFAULT_MULTILOOP 6
#define DEFAULT_MODEL_HZ (DEFAULT_TIMER_HZ * DEFAULT_MULTILOOP)


/* Field of view limits */
#define FG_FOV_MIN 0.1
#define FG_FOV_MAX 179.9


#endif /* _FG_CONSTANTS_H */


/* $Log$
/* Revision 1.5  1998/05/17 16:56:47  curt
/* Re-organized PI related constants.
/*
 * Revision 1.4  1998/05/16 13:03:10  curt
 * Defined field of view max/min limits.
 *
 * Revision 1.3  1998/04/08 23:35:32  curt
 * Tweaks to Gnu automake/autoconf system.
 *
 * Revision 1.2  1998/03/23 21:18:37  curt
 * Made FG_EPSILON smaller.
 *
 * Revision 1.1  1998/01/27 00:46:50  curt
 * prepended "fg_" on the front of these to avoid potential conflicts with
 * system include files.
 *
 * Revision 1.3  1998/01/22 02:59:35  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.2  1998/01/07 03:31:26  curt
 * Miscellaneous tweaks.
 *
 * Revision 1.1  1997/12/15 21:02:15  curt
 * Moved to .../FlightGear/Src/Include/
 *
 * Revision 1.10  1997/09/13 01:59:45  curt
 * Mostly working on stars and generating sidereal time for accurate star
 * placement.
 *
 * Revision 1.9  1997/08/22 21:34:32  curt
 * Doing a bit of reorganizing and house cleaning.
 *
 * Revision 1.8  1997/07/31 22:52:22  curt
 * Working on redoing internal coordinate systems & scenery transformations.
 *
 * Revision 1.7  1997/07/23 21:52:10  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.6  1997/07/21 14:45:01  curt
 * Minor tweaks.
 *
 * Revision 1.5  1997/07/19 23:04:46  curt
 * Added an initial weather section.
 *
 * Revision 1.4  1997/07/19 22:37:03  curt
 * Added various PI definitions.
 *
 * Revision 1.3  1997/07/14 16:26:03  curt
 * Testing/playing -- placed objects randomly across the entire terrain.
 *
 * Revision 1.2  1997/07/08 18:20:11  curt
 * Working on establishing a hard ground.
 *
 * Revision 1.1  1997/07/07 21:02:36  curt
 * Initial revision.
 * */
