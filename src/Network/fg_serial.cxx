// fg_serial.cxx -- Serial I/O routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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


#include <Include/compiler.h>

#include STL_STRING

#include <Debug/logstream.hxx>
#include <Aircraft/aircraft.hxx>
#include <Include/fg_constants.h>
#include <Serial/serial.hxx>
#include <Time/fg_time.hxx>

#include "fg_serial.hxx"

FG_USING_STD(string);


FGSerial::FGSerial() {
}


FGSerial::~FGSerial() {
}


// open the serial port based on specified direction
bool FGSerial::open( FGProtocol::fgProtocolDir dir ) {
    if ( ! port.open_port( device ) ) {
	FG_LOG( FG_IO, FG_ALERT, "Error opening device: " << device );
	return false;
    }

    // cout << "fd = " << port.fd << endl;

    if ( ! port.set_baud( atoi( baud.c_str() ) ) ) {
	FG_LOG( FG_IO, FG_ALERT, "Error setting baud: " << baud );
	return false;
    }

    return true;
}


// read data from port
bool FGSerial::read( char *buf, int *length ) {
    // read a chunk
    *length = port.read_port( buf );
    
    // just in case ...
    buf[ *length ] = '\0';

    return true;
}

// write data to port
bool FGSerial::write( char *buf, int length ) {
    int result = port.write_port( buf, length );

    if ( result != length ) {
	FG_LOG( FG_IO, FG_ALERT, "Error writing data: " << device );
	return false;
    }

    return true;
}


// close the port
bool FGSerial::close() {
    if ( ! port.close_port() ) {
	return false;
    }

    return true;
}
