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

#include <time.h> // time_t

const int FG_NET_FDM_VERSION = 6;


// Define a structure containing the top level flight dynamics model
// parameters

class FGNetFDM {

public:

    int version;		// increment when data values change
    int pad;                    // keep doubles 64-bit aligned for some
                                // hardware platforms, such as the Sun
                                // SPARC, which don't like misaligned
                                // data

    static const int FG_MAX_ENGINES = 4;
    static const int FG_MAX_WHEELS = 3;
    static const int FG_MAX_TANKS = 2;

    // Positions
    double longitude;		// geodetic (radians)
    double latitude;		// geodetic (radians)
    double altitude;		// above sea level (meters)
    double agl;			// above ground level (meters)
    double phi;			// roll (radians)
    double theta;		// pitch (radians)
    double psi;			// yaw or true heading (radians)

    // Velocities
    double phidot;		// roll rate (radians/sec)
    double thetadot;		// pitch rate (radians/sec)
    double psidot;		// yaw rate (radians/sec)
    double vcas;		// calibrated airspeed
    double climb_rate;		// feet per second

    // Accelerations
    double A_X_pilot;		// X accel in body frame ft/sec^2
    double A_Y_pilot;		// Y accel in body frame ft/sec^2
    double A_Z_pilot;		// Z accel in body frame ft/sec^2

    // Engine status
    int num_engines;		// Number of valid engines
    int eng_state[FG_MAX_ENGINES]; // Engine state (off, cranking, running)
    double rpm[FG_MAX_ENGINES];	// Engine RPM rev/min
    double fuel_flow[FG_MAX_ENGINES]; // Fuel flow gallons/hr
    double EGT[FG_MAX_ENGINES];	// Exhuast gas temp deg F
    double oil_temp[FG_MAX_ENGINES]; // Oil temp deg F
    double oil_px[FG_MAX_ENGINES]; // Oil pressure psi

    // Consumables
    int num_tanks;		// Max number of fuel tanks
    double fuel_quantity[FG_MAX_TANKS];

    // Gear status
    int num_wheels;
    bool wow[FG_MAX_WHEELS];

    // Environment
    time_t cur_time;            // current unix time
    long int warp;              // offset in seconds to unix time
    double visibility;          // visibility in meters (for env. effects)
};


#endif // _NET_FDM_HXX
