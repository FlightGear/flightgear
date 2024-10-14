// util.hxx - general-purpose utility functions.
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifndef __UTIL_HXX
#define __UTIL_HXX 1

#include <string>
#include <simgear/misc/sg_path.hxx>

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
double fgGetLowPass (double current, double target, double timeratio);


/**
 * Set the read and write lists of allowed paths patterns for SGPath::validate()
 */
void fgInitAllowedPaths();

namespace flightgear
{
    /**
     * @brief getAircraftAuthorsText - get the aircraft authors as a single
     * string value. This will combine the new (structured) authors data if
     * present, otherwise just return the old plain string
     * @return
     */
    std::string getAircraftAuthorsText();

    /**
     * @brief low-pass filter for bearings, etc in degrees. Inputs can have any sign
     * and output is always in the range [-180.0 .. 180.0]. Ranges of inputs do not
     * have to be consistent, they will be normalised before filtering.
     * 
     * @param current - current value in degrees
     * @param target  - target (new) value in degrees.
     * @param timeratio - 
     */
    double lowPassPeriodicDegreesSigned(double current, double target, double timeratio);

    /**
     * @brief low-pass filter for bearings, etc in degrees. Inputs can have any sign
     * and output is always in the range [0.0 .. 360.0]. Ranges of inputs do not
     * have to be consistent, they will be normalised before filtering.
     * 
     * @param current - current value in degrees
     * @param target  - target (new) value in degrees.
     * @param timeratio - 
     */
    double lowPassPeriodicDegreesPositive(double current, double target, double timeratio);

    /**
    * @brief exponential filter
    * 
    * @param current - current value
    * @param target  - target (new) value
    * @param timeratio - 
    */
    double filterExponential(double current, double target, double timeratio);
}

#endif // __UTIL_HXX
