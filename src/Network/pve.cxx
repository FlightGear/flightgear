// pve.cxx -- "PVE" protocal class (for Provision Entertainment)
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>		// sprintf()

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>

#include <FDM/flightProperties.hxx>

#include "pve.hxx"


FGPVE::FGPVE() {
}


FGPVE::~FGPVE() {
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

bool FGPVE::gen_message() {
    // cout << "generating pve message" << endl;
    FlightProperties f;

    // get roll and pitch, convert to degrees
    double roll_deg = f.get_Phi() * SGD_RADIANS_TO_DEGREES;
    while ( roll_deg <= -180.0 ) {
	roll_deg += 360.0;
    }
    while ( roll_deg > 180.0 ) {
	roll_deg -= 360.0;
    }

    double pitch_deg = f.get_Theta() * SGD_RADIANS_TO_DEGREES;
    while ( pitch_deg <= -180.0 ) {
	pitch_deg += 360.0;
    }
    while ( pitch_deg > 180.0 ) {
	pitch_deg -= 360.0;
    }

    short int heave = (int)(f.get_wBody() * 128.0);

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

    sprintf( buf, "p%c%c%c%c%c%c\n", 
	     roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2 );
    length = 8;

    // printf( "p [ %u %u ]  [ %u %u ]  [ %u %u ]\n", 
    //         roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2 );

    SG_LOG( SG_IO, SG_INFO, "roll=" << roll << " pitch=" << pitch <<
	    " heave=" << heave );

    return true;
}


// parse RUL message
bool FGPVE::parse_message() {
    SG_LOG( SG_IO, SG_ALERT, "PVE input not supported" );

    return false;
}


// process work for this port
bool FGPVE::process() {
    SGIOChannel *io = get_io_channel();

    if ( get_direction() == SG_IO_OUT ) {
	gen_message();
	if ( ! io->write( buf, length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	SG_LOG( SG_IO, SG_ALERT, "in direction not supported for RUL." );
	return false;
    }

    return true;
}
