// net_fdm_mini.hxx -- defines a simple subset I/O interface to the flight
//                     dynamics model variables
//
// Written by Curtis Olson - curt@flightgear.com, started January 2002.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$


#ifndef _NET_FDM_MINI_HXX
#define _NET_FDM_MINI_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


const int FG_NET_FDM_MINI_VERSION = 1;


// Define a structure containing the top level flight dynamics model
// parameters

class FGNetMiniFDM {

public:

    enum {
        FG_MAX_ENGINES = 4,
        FG_MAX_WHEELS = 3,
        FG_MAX_TANKS = 4
    };

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
    double vcas;
    double climb_rate;		// feet per second

    // Consumables
    int num_tanks;		// Max number of fuel tanks
    double fuel_quantity[FG_MAX_TANKS];

    // Environment
    time_t cur_time;            // current unix time
    long int warp;              // offset in seconds to unix time
};


#endif // _NET_FDM_MINI_HXX
