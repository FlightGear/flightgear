// fg_serial.hxx -- Serial I/O routines
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


#ifndef _FG_SERIAL_HXX
#define _FG_SERIAL_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <string>

// #ifdef FG_HAVE_STD_INCLUDES
// #  include <ctime>
// #else
// #  include <time.h>
// #endif

#include <simgear/serial.hxx>

#include "iochannel.hxx"
#include "protocol.hxx"

FG_USING_STD(string);


class FGSerial : public FGIOChannel {

    string device;
    string baud;
    FGSerialPort port;

    char save_buf[ 2 * FG_MAX_MSG_SIZE ];
    int save_len;

public:

    FGSerial();
    ~FGSerial();

    // open the serial port based on specified direction
    bool open( FGProtocol::fgProtocolDir dir );

    // read a block of data of specified size
    int read( char *buf, int length );

    // read a line of data, length is max size of input buffer
    int readline( char *buf, int length );

    // write data to port
    int write( char *buf, int length );

    // write null terminated string to port
    int writestring( char *str );

    // close port
    bool close();

    inline string get_device() const { return device; }
    inline void set_device( const string& d ) { device = d; }
    inline string get_baud() const { return baud; }
    inline void set_baud( const string& b ) { baud = b; }
};


#endif // _FG_SERIAL_HXX


