/*
 * sunsolver.hxx - given a location on earth and a time of day/date,
 *                 find the number of seconds to various sun positions.
 *
 * Written by Curtis Olson, started September 2003.
 *
 * Copyright (C) 2003  Curtis L. Olson  - curt@flightgear.org
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
 */


#ifndef _SUNSOLVER_HXX
#define _SUNSOLVER_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <ctime>
#else
#  include <time.h>
#endif

/**
 * Given the current unix time in seconds, calculate seconds to
 * highest sun angle.
 */
time_t fgTimeSecondsUntilNoon( time_t cur_time,
                               double lon_rad,
                               double lat_rad );


/**
 * Given the current unix time in seconds, calculate seconds to lowest
 * sun angle.
 */
time_t fgTimeSecondsUntilMidnight( time_t cur_time,
                                   double lon_rad,
                                   double lat_rad );

/**
 * Given the current unix time in seconds, calculate seconds to dusk
 */
time_t fgTimeSecondsUntilDusk( time_t cur_time,
                               double lon_rad,
                               double lat_rad );


/**
 * Given the current unix time in seconds, calculate seconds to dawn
 */
time_t fgTimeSecondsUntilDawn( time_t cur_time,
                               double lon_rad,
                               double lat_rad );



#endif /* _SUNSOLVER_HXX */
