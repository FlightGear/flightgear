/*
 * sunsolver.hxx - given a location on earth and a time of day/date,
 *                 find the number of seconds to various sun positions.
 *
 * Written by Curtis Olson, started September 2003.
 *
 * Copyright (C) 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 */


#ifndef _SUNSOLVER_HXX
#define _SUNSOLVER_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>

#include <ctime>

/**
 * Given the current unix time in seconds, calculate seconds to the
 * specified sun angle (relative to straight up.)  Also specify if we
 * want the angle while the sun is ascending or descending.  For
 * instance noon is when the sun angle is 0 (or the closest it can
 * get.)  Dusk is when the sun angle is 90 and descending.  Dawn is
 * when the sun angle is 90 and ascending.
 */
time_t fgTimeSecondsUntilSunAngle( time_t cur_time,
                                   double lon_rad,
                                   double lat_rad,
                                   double target_angle_deg,
                                   bool ascending );

/**
 * given a particular time expressed in side real time at prime
 * meridian (GST), compute position on the earth (lat, lon) such that
 * sun is directly overhead.  (lat, lon are reported in radians
 */
void fgSunPositionGST(double gst, double *lon, double *lat);


#endif /* _SUNSOLVER_HXX */
