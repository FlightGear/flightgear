// raw_ctrls.hxx -- defines a common raw I/O interface to the flight
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


#ifndef _RAW_CTRLS_HXX
#define _RAW_CTRLS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

const int FG_RAW_CTRLS_VERSION = 8;


// Define a structure containing the control parameters

class FGRawCtrls {

public:

    int version;		         // increment when data values change

    enum {
        FG_MAX_ENGINES = 4,
        FG_MAX_WHEELS = 3,
        FG_MAX_TANKS = 4
    };

    // Aero controls
    double aileron;		         // -1 ... 1
    double elevator;		         // -1 ... 1
    double elevator_trim;	         // -1 ... 1
    double rudder;		         // -1 ... 1
    double flaps;		         //  0 ... 1

    // Engine controls
    int num_engines;		         // number of valid engines
    int magnetos[FG_MAX_ENGINES];
    bool starter[FG_MAX_ENGINES];        // true = starter engauged
    double throttle[FG_MAX_ENGINES];     //  0 ... 1
    double mixture[FG_MAX_ENGINES];      //  0 ... 1
    double prop_advance[FG_MAX_ENGINES]; //  0 ... 1

    // Fuel management
    int num_tanks;                       // number of valid tanks
    bool fuel_selector[FG_MAX_TANKS];    // false = off, true = on

    // Brake controls
    int num_wheels;		         // number of valid wheels
    double brake[FG_MAX_WHEELS];         //  0 ... 1
    
    // Landing Gear
    bool gear_handle; // true=gear handle down; false= gear handle up

    // Other values of use to a remote FDM
    double hground;		         // ground elevation (meters)
    double magvar;		         // local magnetic variation in degrees.
    int speedup;		         // integer speedup multiplier
};


#endif // _RAW_CTRLS_HXX


