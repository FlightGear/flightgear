// net_ctrls.hxx -- defines a common net I/O interface to the flight
//                  sim controls
//
// Written by Curtis Olson - curt@flightgear.com, started July 2001.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$


#ifndef _NET_CTRLS_HXX
#define _NET_CTRLS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

const int FG_NET_CTRLS_VERSION = 22;


// Define a structure containing the control parameters

class FGNetCtrls {

public:

    int version;		         // increment when data values change

    enum {
        FG_MAX_ENGINES = 4,
        FG_MAX_WHEELS = 16,
        FG_MAX_TANKS = 6
    };

    // Aero controls
    double aileron;		         // -1 ... 1
    double elevator;		         // -1 ... 1
    double elevator_trim;	         // -1 ... 1
    double rudder;		         // -1 ... 1
    double flaps;		         //  0 ... 1

    // Aero control faults
    bool flaps_power;                    //  true = power available
    bool flap_motor_ok;

    // Engine controls
    int num_engines;		         // number of valid engines
    bool master_bat[FG_MAX_ENGINES];
    bool master_alt[FG_MAX_ENGINES];
    int magnetos[FG_MAX_ENGINES];
    bool starter_power[FG_MAX_ENGINES];  // true = starter power
    double throttle[FG_MAX_ENGINES];     //  0 ... 1
    double mixture[FG_MAX_ENGINES];      //  0 ... 1
    double condition[FG_MAX_ENGINES];    //  0 ... 1
    bool fuel_pump_power[FG_MAX_ENGINES];// true = on
    double prop_advance[FG_MAX_ENGINES]; //  0 ... 1

    // Engine faults
    bool engine_ok[FG_MAX_ENGINES];
    bool mag_left_ok[FG_MAX_ENGINES];
    bool mag_right_ok[FG_MAX_ENGINES];
    bool spark_plugs_ok[FG_MAX_ENGINES];  // false = fouled plugs
    int oil_press_status[FG_MAX_ENGINES]; // 0 = normal, 1 = low, 2 = full fail
    bool fuel_pump_ok[FG_MAX_ENGINES];

    // Fuel management
    int num_tanks;                       // number of valid tanks
    bool fuel_selector[FG_MAX_TANKS];    // false = off, true = on

    // Brake controls
    double brake_left;
    double brake_right;
    double brake_parking;
    
    // Landing Gear
    bool gear_handle; // true=gear handle down; false= gear handle up

    // Switches
    bool master_avionics;

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
    bool icing;                          // icing status could me much
                                         // more complex but I'm
                                         // starting simple here.

    // simulation control
    int speedup;		         // integer speedup multiplier
    int freeze;		                 // 0=normal
				         // 0x01=master
				         // 0x02=position
				         // 0x04=fuel
};


#endif // _NET_CTRLS_HXX


