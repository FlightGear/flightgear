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


#include <simgear/compiler.h>

#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/serial/serial.hxx>

#include <Aircraft/aircraft.hxx>

#include "fg_serial.hxx"

FG_USING_STD(string);


FGSerial::FGSerial() :
    save_len(0)
{
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


// Read data from port.  If we don't get enough data, save what we did
// get in the save buffer and return 0.  The save buffer will be
// prepended to subsequent reads until we get as much as is requested.

int FGSerial::read( char *buf, int length ) {
    int result;

    // read a chunk, keep in the save buffer until we have the
    // requested amount read

    char *buf_ptr = save_buf + save_len;
    result = port.read_port( buf_ptr, length - save_len );
    
    if ( result + save_len == length ) {
	strncpy( buf, save_buf, length );
	save_len = 0;

	return length;
    }
    
    return 0;
}


// read data from port
int FGSerial::readline( char *buf, int length ) {
    int result;

    // read a chunk, keep in the save buffer until we have the
    // requested amount read

    char *buf_ptr = save_buf + save_len;
    result = port.read_port( buf_ptr, FG_MAX_MSG_SIZE - save_len );
    save_len += result;

    // look for the end of line in save_buf
    int i;
    for ( i = 0; i < save_len && save_buf[i] != '\n'; ++i );
    if ( save_buf[i] == '\n' ) {
	result = i + 1;
    } else {
	// no end of line yet
	return 0;
    }

    // we found an end of line

    // copy to external buffer
    strncpy( buf, save_buf, result );
    buf[result] = '\0';
    FG_LOG( FG_IO, FG_INFO, "fg_serial line = " << buf );

    // shift save buffer
    for ( i = result; i < save_len; ++i ) {
	save_buf[ i - result ] = save_buf[i];
    }
    save_len -= result;

    return result;
}


// write data to port
int FGSerial::write( char *buf, int length ) {
    int result = port.write_port( buf, length );

    if ( result != length ) {
	FG_LOG( FG_IO, FG_ALERT, "Error writing data: " << device );
    }

    return result;
}


// write null terminated string to port
int FGSerial::writestring( char *str ) {
    int length = strlen( str );
    return write( str, length );
}


// close the port
bool FGSerial::close() {
    if ( ! port.close_port() ) {
	return false;
    }

    return true;
}
