// fg_serial.hxx -- Higher level serial port managment routines
//
// Written by Curtis Olson, started November 1998.
//
// Copyright (C) 1998  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_SERIAL_HXX
#define _FG_SERIAL_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include "Include/compiler.h"

#include <string>

#ifdef FG_HAVE_STD_INCLUDES
#  include <ctime>
#else
#  include <time.h>
#endif

#include <Serial/serial.hxx>


class fgIOCHANNEL {

public:

    // Types of serial port protocols
    enum fgPortKind {
	FG_SERIAL_DISABLED = 0,
	FG_SERIAL_NMEA_OUT = 1,
	FG_SERIAL_NMEA_IN = 2,
	FG_SERIAL_GARMIN_OUT = 3,
	FG_SERIAL_GARMIN_IN = 4,
	FG_SERIAL_FGFS_OUT = 5,
	FG_SERIAL_FGFS_IN = 6,

	FG_SERIAL_RUL_OUT = 7         // Raul E Horcasitas <rul@compuserve.com>
    };

    string device;
    string format;
    string baud;
    string direction;

    fgPortKind kind;
    fgSERIAL port;
    time_t last_time;
    bool valid_config;

    fgIOCHANNEL();
    ~fgIOCHANNEL();
};


// support up to four serial channels.  Each channel can be assigned
// to an arbitrary port.  Bi-directional communication is supported by
// the underlying layer.

// define the four channels
// extern fgIOCHANNEL port_a;
// extern fgIOCHANNEL port_b;
// extern fgIOCHANNEL port_c;
// extern fgIOCHANNEL port_d;


// initialize serial ports based on command line options (if any)
void fgSerialInit();


// process any serial port work
void fgSerialProcess();


#endif // _FG_SERIAL_HXX


