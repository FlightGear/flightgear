// net_ctrls.hxx -- defines a common net I/O interface to the flight
//                  sim controls
//
// Written by Curtis Olson - http://www.flightgear.org/~curt
// Started July 2001.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$


#ifndef _NET_CTRLS_HXX
#define _NET_CTRLS_HXX


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

const uint16_t FG_NET_CTRLS_VERSION = 25;


// Define a structure containing the control parameters

class FGNetCtrls {

public:

    enum {
        FG_MAX_ENGINES = 4,
        FG_MAX_WHEELS = 16,
        FG_MAX_TANKS = 6
    };

    uint16_t version;		         // increment when data values change

    // Aero controls
    double aileron;		         // -1 ... 1
    double elevator;		         // -1 ... 1
    double rudder;		         // -1 ... 1
    double aileron_trim;	         // -1 ... 1
    double elevator_trim;	         // -1 ... 1
    double rudder_trim;		         // -1 ... 1
    double flaps;		         //  0 ... 1

    // Aero control faults
    uint8_t flaps_power;                 // true = power available
    uint8_t flap_motor_ok;

    // Engine controls
    uint8_t num_engines;		 // number of valid engines
    uint8_t master_bat[FG_MAX_ENGINES];
    uint8_t master_alt[FG_MAX_ENGINES];
    uint8_t magnetos[FG_MAX_ENGINES];
    uint8_t starter_power[FG_MAX_ENGINES];// true = starter power
    double throttle[FG_MAX_ENGINES];     //  0 ... 1
    double mixture[FG_MAX_ENGINES];      //  0 ... 1
    double condition[FG_MAX_ENGINES];    //  0 ... 1
    uint8_t fuel_pump_power[FG_MAX_ENGINES];// true = on
    double prop_advance[FG_MAX_ENGINES]; //  0 ... 1

    // Engine faults
    uint8_t engine_ok[FG_MAX_ENGINES];
    uint8_t mag_left_ok[FG_MAX_ENGINES];
    uint8_t mag_right_ok[FG_MAX_ENGINES];
    uint8_t spark_plugs_ok[FG_MAX_ENGINES];  // false = fouled plugs
    uint8_t oil_press_status[FG_MAX_ENGINES];// 0 = normal, 1 = low, 2 = full fail
    uint8_t fuel_pump_ok[FG_MAX_ENGINES];

    // Fuel management
    uint8_t num_tanks;                      // number of valid tanks
    uint8_t fuel_selector[FG_MAX_TANKS];    // false = off, true = on
    uint8_t cross_feed;                     // false = off, true = on

    // Brake controls
    double brake_left;
    double brake_right;
    double copilot_brake_left;
    double copilot_brake_right;
    double brake_parking;
    
    // Landing Gear
    uint8_t gear_handle; // true=gear handle down; false= gear handle up

    // Switches
    uint8_t master_avionics;

    // wind and turbulance
    double wind_speed_kt;
    double wind_dir_deg;
    double turbulence_norm;

    // temp and pressure
    double temp_c;
    double press_inhg;

    // other information about environment
    double hground;		         // ground elevation (meters)
    double magvar;		         // local magnetic variation in degs.

    // hazards
    uint8_t icing;                       // icing status could me much
                                         // more complex but I'm
                                         // starting simple here.

    // simulation control
    uint8_t speedup;		         // integer speedup multiplier
    uint8_t freeze;		         // 0=normal
				         // 0x01=master
				         // 0x02=position
				         // 0x04=fuel
};


#endif // _NET_CTRLS_HXX


