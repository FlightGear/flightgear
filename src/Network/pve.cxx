// pve.cxx -- "PVE" protocal class (for Provision Entertainment)
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


#include <Debug/logstream.hxx>
#include <FDM/flight.hxx>
#include <Math/fg_geodesy.hxx>
#include <Time/fg_time.hxx>

#include "iochannel.hxx"
#include "pve.hxx"


FGPVE::FGPVE() {
}


FGPVE::~FGPVE() {
}


// generate Garmin message
bool FGPVE::gen_message() {
    // cout << "generating pve message" << endl;
    FGInterface *f = cur_fdm_state;

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

    sprintf( buf, "p%c%c%c%c%c%c\n", 
	     roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2 );
    length = 8;

    // printf( "p [ %u %u ]  [ %u %u ]  [ %u %u ]\n", 
    //         roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2 );

    FG_LOG( FG_IO, FG_INFO, "roll=" << roll << " pitch=" << pitch <<
	    " heave=" << heave );

    return true;
}


// parse RUL message
bool FGPVE::parse_message() {
    FG_LOG( FG_IO, FG_ALERT, "PVE input not supported" );

    return false;
}


// process work for this port
bool FGPVE::process() {
    FGIOChannel *io = get_io_channel();

    if ( get_direction() == out ) {
	gen_message();
	if ( ! io->write( buf, length ) ) {
	    FG_LOG( FG_IO, FG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == in ) {
	FG_LOG( FG_IO, FG_ALERT, "in direction not supported for RUL." );
	return false;
    }

    return true;
}
