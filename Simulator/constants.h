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


#ifndef CONSTANTS_H
#define CONSTANTS_H


#include <math.h>


/* Make sure PI is defined in its various forms */
#ifdef M_PI
#  define  FG_PI  M_PI
#else
#  define  FG_PI  3.14159265358979323846      /* pi */
#endif

#ifdef M_PI_2
#  define  FG_PI_2  M_PI_2
#else
#  define  FG_PI_2  1.57079632679489661923    /* pi/2 */
#endif

#define FG_2PI (FG_PI + FG_PI)                /* 2*pi */


/* Radius of Earth in kilometers at the equator.  Another source had
 * 6378.165 but this is probably close enough */
#define EARTH_RAD 6378.155


/* Degrees to Radians */
#define DEG_TO_RAD       0.017453292          /* deg*pi/180 = rad */

/* Radians to Degrees */
#define RAD_TO_DEG       57.29577951          /* rad*180/pi = deg */

/* Arc seconds to radians */                  /* (arcsec*pi)/(3600*180) = rad */
#define ARCSEC_TO_RAD 4.84813681109535993589e-06 

/* Radians to arc seconds */                  /* (rad*3600*180)/pi = arcsec */
#define RAD_TO_ARCSEC 2035752.03952618601852

/* Feet to Meters */
#define FEET_TO_METER    0.3048

/* Meters to Feet */
#define METER_TO_FEET    3.28083989501312335958  


/* For divide by zero avoidance, this will be close enough to zero */
#define EPSILON 0.000001


#endif CONSTANTS_H


/* $Log$
/* Revision 1.5  1997/07/19 23:04:46  curt
/* Added an initial weather section.
/*
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
