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
// (Log is kept at end of this file)


#ifndef _FG_SERIAL_HXX
#define _FG_SERIAL_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <Serial/serial.hxx>


// Types of serial port protocols
enum fgSerialPortKind {
    FG_SERIAL_DISABLED = 0,
    FG_SERIAL_NMEA_OUT = 1,
    FG_SERIAL_NMEA_IN = 2,
    FG_SERIAL_GARMAN_OUT = 3,
    FG_SERIAL_GARMAN_IN = 4,
    FG_SERIAL_FGFS_OUT = 5,
    FG_SERIAL_FGFS_IN = 6
};


// support up to four serial channels.  Each channel can be assigned
// to an arbitrary port.  Bi-directional communication is supported by
// the underlying layer.

// define the four channels
extern fgSERIAL port_a;
extern fgSERIAL port_b;
extern fgSERIAL port_c;
extern fgSERIAL port_d;


// initialize serial ports based on command line options (if any)
void fgSerialInit();


// process any serial port work
void fgSerialProcess();


#endif // _FG_SERIAL_HXX


// $Log$
// Revision 1.2  1998/11/19 13:53:27  curt
// Added a "Garman" mode.
//
// Revision 1.1  1998/11/16 13:57:43  curt
// Initial revision.
//
