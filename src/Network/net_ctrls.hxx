// net_ctrls.hxx -- defines a common net I/O interface to the flight
//                  sim controls
//
// Written by Curtis Olson, started July 2001.
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


#ifndef _NET_CTRLS_HXX
#define _NET_CTRLS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

const int FG_NET_CTRLS_VERSION = 18;


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
    int magnetos[FG_MAX_ENGINES];
    bool starter_power[FG_MAX_ENGINES];  // true = starter power
    double throttle[FG_MAX_ENGINES];     //  0 ... 1
    double mixture[FG_MAX_ENGINES];      //  0 ... 1
    bool fuel_pump_power[FG_MAX_ENGINES];// true = on
    double prop_advance[FG_MAX_ENGINES]; //  0 ... 1

    // Engine faults
    bool engine_ok[FG_MAX_ENGINES];
    bool mag_left_ok[FG_MAX_ENGINES];
    bool mag_right_ok[FG_MAX_ENGINES];
    bool spark_plugs_ok[FG_MAX_ENGINES]; // false = fouled plugs
    int oil_press_status[FG_MAX_ENGINES]; // 0 = normal, 1 = low, 2 = full fail
    bool fuel_pump_ok[FG_MAX_ENGINES];

    // Fuel management
    int num_tanks;                       // number of valid tanks
    bool fuel_selector[FG_MAX_TANKS];    // false = off, true = on

    // Brake controls
    int num_wheels;		         // number of valid wheels
    double brake[FG_MAX_WHEELS];         //  0 ... 1
    
    // Landing Gear
    bool gear_handle; // true=gear handle down; false= gear handle up

    // Switches
    bool master_bat;
    bool master_alt;
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

    // simulation control
    int speedup;		         // integer speedup multiplier
    int freeze;		                 // 0=normal
				         // 0x01=master
				         // 0x02=position
				         // 0x04=fuel
};


#endif // _NET_CTRLS_HXX


