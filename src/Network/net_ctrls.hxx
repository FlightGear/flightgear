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

#include <simgear/misc/stdint.hxx>

// NOTE: this file defines an external interface structure.  Due to
// variability between platforms and architectures, we only used fixed
// length types here.  Specifically, integer types can vary in length.
// I am not aware of any platforms that don't use 4 bytes for float
// and 8 bytes for double.

const uint32_t FG_NET_CTRLS_VERSION = 26;


// Define a structure containing the control parameters

class FGNetCtrls {

public:

    enum {
        FG_MAX_ENGINES = 4,
        FG_MAX_WHEELS = 16,
        FG_MAX_TANKS = 6
    };

    uint32_t version;		         // increment when data values change

    // Aero controls
    double aileron;		         // -1 ... 1
    double elevator;		         // -1 ... 1
    double rudder;		         // -1 ... 1
    double aileron_trim;	         // -1 ... 1
    double elevator_trim;	         // -1 ... 1
    double rudder_trim;		         // -1 ... 1
    double flaps;		         //  0 ... 1

    // Aero control faults
    uint32_t flaps_power;                 // true = power available
    uint32_t flap_motor_ok;

    // Engine controls
    uint32_t num_engines;		 // number of valid engines
    uint32_t master_bat[FG_MAX_ENGINES];
    uint32_t master_alt[FG_MAX_ENGINES];
    uint32_t magnetos[FG_MAX_ENGINES];
    uint32_t starter_power[FG_MAX_ENGINES];// true = starter power
    double throttle[FG_MAX_ENGINES];     //  0 ... 1
    double mixture[FG_MAX_ENGINES];      //  0 ... 1
    double condition[FG_MAX_ENGINES];    //  0 ... 1
    uint32_t fuel_pump_power[FG_MAX_ENGINES];// true = on
    double prop_advance[FG_MAX_ENGINES]; //  0 ... 1

    // Engine faults
    uint32_t engine_ok[FG_MAX_ENGINES];
    uint32_t mag_left_ok[FG_MAX_ENGINES];
    uint32_t mag_right_ok[FG_MAX_ENGINES];
    uint32_t spark_plugs_ok[FG_MAX_ENGINES];  // false = fouled plugs
    uint32_t oil_press_status[FG_MAX_ENGINES];// 0 = normal, 1 = low, 2 = full fail
    uint32_t fuel_pump_ok[FG_MAX_ENGINES];

    // Fuel management
    uint32_t num_tanks;                      // number of valid tanks
    uint32_t fuel_selector[FG_MAX_TANKS];    // false = off, true = on
    uint32_t cross_feed;                     // false = off, true = on

    // Brake controls
    double brake_left;
    double brake_right;
    double copilot_brake_left;
    double copilot_brake_right;
    double brake_parking;
    
    // Landing Gear
    uint32_t gear_handle; // true=gear handle down; false= gear handle up

    // Switches
    uint32_t master_avionics;

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
    uint32_t icing;                       // icing status could me much
                                         // more complex but I'm
                                         // starting simple here.

    // simulation control
    uint32_t speedup;		         // integer speedup multiplier
    uint32_t freeze;		         // 0=normal
				         // 0x01=master
				         // 0x02=position
				         // 0x04=fuel
};


#endif // _NET_CTRLS_HXX


