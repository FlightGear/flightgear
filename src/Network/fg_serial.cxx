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

// #ifdef FG_HAVE_STD_INCLUDES
// #  include <cmath>
// #  include <cstdlib>    // atoi()
// #else
// #  include <math.h>
// #  include <stdlib.h>   // atoi()
// #endif

#include STL_STRING

#include <Debug/logstream.hxx>
#include <Aircraft/aircraft.hxx>
#include <Include/fg_constants.h>
#include <Serial/serial.hxx>
#include <Time/fg_time.hxx>

// #include "options.hxx"

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


#if 0
// "RUL" output format (for some sort of motion platform)
//
// The Baud rate is 2400 , one start bit, eight data bits, 1 stop bit,
// no parity.
//
// For position it requires a 3-byte data packet defined as follows:
//
// First bite: ascII character "P" ( 0x50 or 80 decimal )
// Second byte X pos. (1-255) 1 being 0* and 255 being 359*
// Third byte Y pos.( 1-255) 1 being 0* and 255 359*
//
// So sending 80 127 127 to the two axis motors will position on 180*
// The RS- 232 port is a nine pin connector and the only pins used are
// 3&5.

static void send_rul_out( fgIOCHANNEL *p ) {
    char rul[256];

    FGInterface *f;
    FGTime *t;

    f = current_aircraft.fdm_state;
    t = FGTime::cur_time_params;

    // run as often as possibleonce per second

    // this runs once per second
    // if ( p->last_time == t->get_cur_time() ) {
    //    return;
    // }
    // p->last_time = t->get_cur_time();
    // if ( t->get_cur_time() % 2 != 0 ) {
    //    return;
    // }
    
    // get roll and pitch, convert to degrees
    double roll_deg = f->get_Phi() * RAD_TO_DEG;
    while ( roll_deg < -180.0 ) {
	roll_deg += 360.0;
    }
    while ( roll_deg > 180.0 ) {
	roll_deg -= 360.0;
    }

    double pitch_deg = f->get_Theta() * RAD_TO_DEG;
    while ( pitch_deg < -180.0 ) {
	pitch_deg += 360.0;
    }
    while ( pitch_deg > 180.0 ) {
	pitch_deg -= 360.0;
    }

    // scale roll and pitch to output format (1 - 255)
    // straight && level == (128, 128)

    int roll = (int)( (roll_deg+180.0) * 255.0 / 360.0) + 1;
    int pitch = (int)( (pitch_deg+180.0) * 255.0 / 360.0) + 1;

    sprintf( rul, "p%c%c\n", roll, pitch);

    FG_LOG( FG_IO, FG_INFO, "p " << roll << " " << pitch );

    string rul_sentence = rul;
    p->port.write_port(rul_sentence);
}


// "PVE" (ProVision Entertainment) output format (for some sort of
// motion platform)
//
// Outputs a 5-byte data packet defined as follows:
//
// First bite:  ASCII character "P" ( 0x50 or 80 decimal )
// Second byte:  "roll" value (1-255) 1 being 0* and 255 being 359*
// Third byte:  "pitch" value (1-255) 1 being 0* and 255 being 359*
// Fourth byte:  "heave" value (or vertical acceleration?)
//
// So sending 80 127 127 to the two axis motors will position on 180*
// The RS- 232 port is a nine pin connector and the only pins used are
// 3&5.

static void send_pve_out( fgIOCHANNEL *p ) {
    char pve[256];

    FGInterface *f;
    FGTime *t;


    f = current_aircraft.fdm_state;
    t = FGTime::cur_time_params;

    // run as often as possibleonce per second

    // this runs once per second
    // if ( p->last_time == t->get_cur_time() ) {
    //    return;
    // }
    // p->last_time = t->get_cur_time();
    // if ( t->get_cur_time() % 2 != 0 ) {
    //    return;
    // }
    
    // get roll and pitch, convert to degrees
    double roll_deg = f->get_Phi() * RAD_TO_DEG;
    while ( roll_deg <= -180.0 ) {
	roll_deg += 360.0;
    }
    while ( roll_deg > 180.0 ) {
	roll_deg -= 360.0;
    }

    double pitch_deg = f->get_Theta() * RAD_TO_DEG;
    while ( pitch_deg <= -180.0 ) {
	pitch_deg += 360.0;
    }
    while ( pitch_deg > 180.0 ) {
	pitch_deg -= 360.0;
    }

    short int heave = (int)(f->get_W_body() * 128.0);

    // scale roll and pitch to output format (1 - 255)
    // straight && level == (128, 128)

    short int roll = (int)(roll_deg * 32768 / 180.0);
    short int pitch = (int)(pitch_deg * 32768 / 180.0);

    unsigned char roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2;
    roll_b1 = roll >> 8;
    roll_b2 = roll & 0x00ff;
    pitch_b1 = pitch >> 8;
    pitch_b2 = pitch & 0x00ff;
    heave_b1 = heave >> 8;
    heave_b2 = heave & 0x00ff;

    sprintf( pve, "p%c%c%c%c%c%c\n", 
	     roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2 );

    // printf( "p [ %u %u ]  [ %u %u ]  [ %u %u ]\n", 
    //         roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2 );

    FG_LOG( FG_IO, FG_INFO, "roll=" << roll << " pitch=" << pitch <<
	    " heave=" << heave );

    string pve_sentence = pve;
    p->port.write_port(pve_sentence);
}

#endif


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
