// util.hxx - general-purpose utility functions.
// Copyright (C) 2002  Curtis L. Olson  - curt@me.umn.edu
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


#ifndef __UTIL_HXX
#define __UTIL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif


/**
 * Move a value towards a target.
 *
 * This function was originally written by Alex Perry.
 *
 * @param current The current value.
 * @param target The target value.
 * @param timeratio The percentage of smoothing time that has passed 
 *        (elapsed time/smoothing time)
 * @return The new value.
 */
extern double fgGetLowPass (double current, double target, double timeratio);


#endif // __UTIL_HXX
