// net_fdm.hxx -- defines a common net I/O interface to the flight
//                dynamics model
//
// Written by Curtis Olson - curt@flightgear.com, started September 2001.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$


#ifndef _NET_FDM_HXX
#define _NET_FDM_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <time.h> // time_t

const int FG_NET_FDM_VERSION = 19;


// Define a structure containing the top level flight dynamics model
// parameters

class FGNetFDM {

public:

    int version;		// increment when data values change
    int pad;                    // keep doubles 64-bit aligned for some
                                // hardware platforms, such as the Sun
                                // SPARC, which don't like misaligned
                                // data

    enum {
        FG_MAX_ENGINES = 4,
        FG_MAX_WHEELS = 3,
        FG_MAX_TANKS = 4
    };

    // Positions
    double longitude;		// geodetic (radians)
    double latitude;		// geodetic (radians)
    double altitude;		// above sea level (meters)
    float agl;			// above ground level (meters)
    float phi;			// roll (radians)
    float theta;		// pitch (radians)
    float psi;			// yaw or true heading (radians)
    float alpha;                // angle of attack (radians)
    float beta;                 // side slip angle (radians)

    // Velocities
    float phidot;		// roll rate (radians/sec)
    float thetadot;		// pitch rate (radians/sec)
    float psidot;		// yaw rate (radians/sec)
    float vcas;		        // calibrated airspeed
    float climb_rate;		// feet per second
    float v_north;              // north velocity in local/body frame, fps
    float v_east;               // east velocity in local/body frame, fps
    float v_down;               // down/vertical velocity in local/body frame, fps
    float v_wind_body_north;    // north velocity in local/body frame
                                // relative to local airmass, fps
    float v_wind_body_east;     // east velocity in local/body frame
                                // relative to local airmass, fps
    float v_wind_body_down;     // down/vertical velocity in local/body
                                // frame relative to local airmass, fps

    // Accelerations
    float A_X_pilot;		// X accel in body frame ft/sec^2
    float A_Y_pilot;		// Y accel in body frame ft/sec^2
    float A_Z_pilot;		// Z accel in body frame ft/sec^2

    // Stall
    float stall_warning;        // 0.0 - 1.0 indicating the amount of stall
    float slip_deg;		// slip ball deflection

    // Pressure
    
    // Engine status
    int num_engines;		     // Number of valid engines
    int eng_state[FG_MAX_ENGINES];   // Engine state (off, cranking, running)
    float rpm[FG_MAX_ENGINES];	     // Engine RPM rev/min
    float fuel_flow[FG_MAX_ENGINES]; // Fuel flow gallons/hr
    float egt[FG_MAX_ENGINES];	     // Exhuast gas temp deg F
    float mp_osi[FG_MAX_ENGINES];    // Manifold pressure
    float oil_temp[FG_MAX_ENGINES];  // Oil temp deg F
    float oil_px[FG_MAX_ENGINES];    // Oil pressure psi

    // Consumables
    int num_tanks;		// Max number of fuel tanks
    float fuel_quantity[FG_MAX_TANKS];

    // Gear status
    int num_wheels;
    bool wow[FG_MAX_WHEELS];
    float gear_pos[FG_MAX_WHEELS];
    float gear_steer[FG_MAX_WHEELS];
    float gear_compression[FG_MAX_WHEELS];

    // Environment
    time_t cur_time;            // current unix time
    long int warp;              // offset in seconds to unix time
    float visibility;           // visibility in meters (for env. effects)

    // Control surface positions (normalized values)
    float elevator;
    float elevator_trim_tab;
    float left_flap;
    float right_flap;
    float left_aileron;
    float right_aileron;
    float rudder;
    float nose_wheel;
    float speedbrake;
    float spoilers;
};


#endif // _NET_FDM_HXX
