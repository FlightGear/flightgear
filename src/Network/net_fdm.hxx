// net_fdm.hxx -- defines a common net I/O interface to the flight
//                dynamics model
//
// Written by Curtis Olson, started September 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - curt@flightgear.com
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


#ifndef _NET_FDM_HXX
#define _NET_FDM_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

const int FG_NET_FDM_VERSION = 0x0104;

// Define a structure containing the top level flight dynamics model
// parameters

class FGNetFDM {

public:

    int version;		// increment when data values change
    int pad;                    // keep doubles 64-bit aligned for some
                                // hardware platforms, such as the Sun
                                // SPARC, which don't like misaligned
                                // data

    // Positions
    double longitude;		// geodetic (radians)
    double latitude;		// geodetic (radians)
    double altitude;		// above sea level (meters)
    double agl;			// above ground level (meters)
    double phi;			// roll (radians)
    double theta;		// pitch (radians)
    double psi;			// yaw or true heading (radians)

    // Velocities
    double vcas;		// calibrated airspeed
    double climb_rate;		// feet per second

    // Time
    time_t cur_time;            // current unix time
    long int warp;              // offset in seconds to unix time
};


#endif // _NET_FDM_HXX
