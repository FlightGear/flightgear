// net_gui.hxx -- defines a simple subset I/O interface to the flight
//                     dynamics model variables
//
// Written by Curtis Olson - curt@flightgear.com, started January 2002.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$


#ifndef _NET_GUI_HXX
#define _NET_GUI_HXX


// NOTE: this file defines an external interface structure.  Due to
// variability between platforms and architectures, we only used fixed
// length types here.  Specifically, integer types can vary in length.
// I am not aware of any platforms that don't use 4 bytes for float
// and 8 bytes for double.

#ifdef HAVE_STDINT_H
# include <stdint.h>
#elif defined( _MSC_VER ) || defined(__MINGW32__)
typedef signed char      int8_t;
typedef signed short     int16_t;
typedef signed int       int32_t;
typedef signed __int64   int64_t;
typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;
typedef unsigned __int64 uint64_t;
#else
# error "Port me! Platforms that don't have <stdint.h> need to define int8_t, et. al."
#endif

const uint32_t FG_NET_GUI_VERSION = 7;


// Define a structure containing the top level flight dynamics model
// parameters

class FGNetGUI {

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
    float altitude;		// above sea level (meters)
    float agl;			// above ground level (meters)
    float phi;			// roll (radians)
    float theta;		// pitch (radians)
    float psi;			// yaw or true heading (radians)

    // Velocities
    float vcas;
    float climb_rate;		// feet per second

    // Consumables
    uint32_t num_tanks;		// Max number of fuel tanks
    float fuel_quantity[FG_MAX_TANKS];

    // Environment
    uint32_t cur_time;          // current unix time
                                // FIXME: make this uint64_t before 2038
    uint32_t warp;              // offset in seconds to unix time
    float ground_elev;          // ground elev (meters)

    // Approach
    float tuned_freq;           // currently tuned frequency
    float nav_radial;           // target nav radial
    uint32_t in_range;           // tuned navaid is in range?
    float dist_nm;              // distance to tuned navaid in nautical miles
    float course_deviation_deg; // degrees off target course
    float gs_deviation_deg;     // degrees off target glide slope
};


#endif // _NET_GUI_HXX
