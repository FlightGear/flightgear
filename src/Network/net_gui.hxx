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


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


const int FG_NET_GUI_VERSION = 5;


// Define a structure containing the top level flight dynamics model
// parameters

class FGNetGUI {

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
    float altitude;		// above sea level (meters)
    float agl;			// above ground level (meters)
    float phi;			// roll (radians)
    float theta;		// pitch (radians)
    float psi;			// yaw or true heading (radians)

    // Velocities
    float vcas;
    float climb_rate;		// feet per second

    // Consumables
    int num_tanks;		// Max number of fuel tanks
    float fuel_quantity[FG_MAX_TANKS];

    // Environment
    time_t cur_time;            // current unix time
    long int warp;              // offset in seconds to unix time
    float ground_elev;          // ground elev (meters)

    // Approach
    float tuned_freq;           // currently tuned frequency
    float nav_radial;           // target nav radial
    int in_range;               // tuned navaid is in range?
    float dist_nm;              // distance to tuned navaid in nautical miles
    float course_deviation_deg; // degrees off target course
    float gs_deviation_deg;     // degrees off target glide slope
};


#endif // _NET_GUI_HXX
