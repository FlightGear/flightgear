// net_fdm_mini.hxx -- defines a simple subset I/O interface to the flight
//                     dynamics model variables
//
// Written by Curtis Olson - http://www.flightgear.org/~curt
// Started January 2002.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$


#ifndef _NET_FDM_MINI_HXX
#define _NET_FDM_MINI_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/misc/stdint.hxx>

// NOTE: this file defines an external interface structure.  Due to
// variability between platforms and architectures, we only used fixed
// length types here.  Specifically, integer types can vary in length.
// I am not aware of any platforms that don't use 4 bytes for float
// and 8 bytes for double.


const uint32_t FG_NET_FDM_MINI_VERSION = 2;


// Define a structure containing the top level flight dynamics model
// parameters

class FGNetMiniFDM {

public:

    enum {
        FG_MAX_ENGINES = 4,
        FG_MAX_WHEELS = 3,
        FG_MAX_TANKS = 4
    };

    uint32_t version;		// increment when data values change

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
    uint32_t num_tanks;		// Max number of fuel tanks
    double fuel_quantity[FG_MAX_TANKS];

    // Environment
    uint32_t cur_time;            // current unix time
    int32_t warp;                 // offset in seconds to unix time
};


#endif // _NET_FDM_MINI_HXX
