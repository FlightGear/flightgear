// AV400Sim.cxx -- Garmin 400 series protocal class.  This AV400Sim
// protocol generates the set of "simulator" commands a garmin 400
// series gps would expect as input in simulator mode.  The AV400
// protocol generates the set of commands that a garmin 400 series gps
// would emit.
//
// Written by Curtis Olson, started Janauary 2009.
//
// Copyright (C) 2009  Curtis L. Olson - http://www.flightgear.org/~curt
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
#  include "config.h"
#endif

#include <cstdlib>
#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#include <FDM/flightProperties.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "AV400Sim.hxx"

FGAV400Sim::FGAV400Sim() {
  fdm = new FlightProperties;
}

FGAV400Sim::~FGAV400Sim() {
  delete fdm;
}


// generate AV400Sim message
bool FGAV400Sim::gen_message() {
    // cout << "generating garmin message" << endl;

    char msg_a[32], msg_b[32], msg_c[32], msg_d[32], msg_e[32];
    char msg_f[32], msg_h[32], msg_i[32], msg_j[32], msg_k[32], msg_l[32], msg_r[32];
    char msg_type2[256];

    char dir;
    int deg;
    double min;

    // create msg_a
    double latd = fdm->get_Latitude() * SGD_RADIANS_TO_DEGREES;
    if ( latd < 0.0 ) {
	latd = -latd;
	dir = 'S';
    } else {
	dir = 'N';
    }
    deg = (int)latd;
    min = (latd - (double)deg) * 60.0 * 100.0;
    sprintf( msg_a, "a%c %03d %04.0f\r\n", dir, deg, min);

    // create msg_b
    double lond = fdm->get_Longitude() * SGD_RADIANS_TO_DEGREES;
    if ( lond < 0.0 ) {
	lond = -lond;
	dir = 'W';
    } else {
	dir = 'E';
    }
    deg = (int)lond;
    min = (lond - (double)deg) * 60.0 * 100.0;
    sprintf( msg_b, "b%c %03d %04.0f\r\n", dir, deg, min);

    // create msg_c
    double alt = fdm->get_Altitude();
    if ( alt > 99999.0 ) { alt = 99999.0; }
    sprintf( msg_c, "c%05.0f\r\n", alt );

    // create msg_d
    double ve_kts = fgGetDouble( "/velocities/speed-east-fps" ) * SG_FPS_TO_KT;
    if ( ve_kts < 0.0 ) {
	ve_kts = -ve_kts;
	dir = 'W';
    } else {
	dir = 'E';
    }
    if ( ve_kts > 999.0 ) { ve_kts = 999.0; }
    sprintf( msg_d, "d%c%03.0f\r\n", dir, ve_kts );

    // create msg_e
    double vn_kts = fgGetDouble( "/velocities/speed-north-fps" ) * SG_FPS_TO_KT;
    if ( vn_kts < 0.0 ) {
	vn_kts = -vn_kts;
	dir = 'S';
    } else {
	dir = 'N';
    }
    if ( vn_kts > 999.0 ) { vn_kts = 999.0; }
    sprintf( msg_e, "e%c%03.0f\r\n", dir, vn_kts );

    // create msg_f
    double climb_fpm = fgGetDouble( "/velocities/vertical-speed-fps" ) * 60;
    if ( climb_fpm < 0.0 ) {
	climb_fpm = -climb_fpm;
	dir = 'D';
    } else {
	dir = 'U';
    }
    if ( climb_fpm > 9999.0 ) { climb_fpm = 9999.0; }
    sprintf( msg_f, "f%c%04.0f\r\n", dir, climb_fpm );

    // create msg_h
    double obs = fgGetDouble( "/instrumentation/nav[0]/radials/selected-deg" );
    sprintf( msg_h, "h%04d\r\n", (int)(obs*10) );

    // create msg_i
    double fuel = fgGetDouble( "/consumables/fuel/total-fuel-gals" );
    if ( fuel > 999.9 ) { fuel = 999.9; }
    sprintf( msg_i, "i%04.0f\r\n", fuel*10.0 );

    // create msg_j
    double gph = fgGetDouble( "/engines/engine[0]/fuel-flow-gph" );
    gph += fgGetDouble( "/engines/engine[1]/fuel-flow-gph" );
    gph += fgGetDouble( "/engines/engine[2]/fuel-flow-gph" );
    gph += fgGetDouble( "/engines/engine[3]/fuel-flow-gph" );
    if ( gph > 999.9 ) { gph = 999.9; }
    sprintf( msg_j, "j%04.0f\r\n", gph*10.0 );

    // create msg_k
    sprintf( msg_k, "k%04d%02d%02d%02d%02d%02d\r\n",
	     fgGetInt( "/sim/time/utc/year"),
	     fgGetInt( "/sim/time/utc/month"),
	     fgGetInt( "/sim/time/utc/day"),
	     fgGetInt( "/sim/time/utc/hour"),
	     fgGetInt( "/sim/time/utc/minute"),
	     fgGetInt( "/sim/time/utc/second") );

    // create msg_l
    alt = fgGetDouble( "/instrumentation/pressure-alt-ft" );
    if ( alt > 99999.0 ) { alt = 99999.0; }
    sprintf( msg_l, "l%05.0f\r\n", alt );

    // create msg_r
    sprintf( msg_r, "rA\r\n" );

    // sentence type 2
    sprintf( msg_type2, "w01%c\r\n", (char)65 );

    // assemble message
    string sentence;
    sentence += '\002';         // STX
    sentence += msg_a;          // latitude
    sentence += msg_b;          // longitude
    sentence += msg_c;          // gps altitude
    sentence += msg_d;          // ve kts
    sentence += msg_e;          // vn kts
    sentence += msg_f;		// climb fpm
    sentence += msg_h;		// obs heading in deg (*10)
    sentence += msg_i;		// total fuel in gal (*10)
    sentence += msg_j;		// fuel flow gph (*10)
    sentence += msg_k;		// date/time (UTC)
    sentence += msg_l;		// pressure altitude
    sentence += msg_r;		// RAIM available
    sentence += msg_type2;      // type2 message
    sentence += '\003';         // ETX

    // cout << sentence;
    length = sentence.length();
    // cout << endl << "length = " << length << endl;
    strncpy( buf, sentence.c_str(), length );

    return true;
}


// parse AV400Sim message
bool FGAV400Sim::parse_message() {
    SG_LOG( SG_IO, SG_INFO, "parse AV400Sim message" );

    string msg = buf;
    msg = msg.substr( 0, length );
    SG_LOG( SG_IO, SG_INFO, "entire message = " << msg );

    string ident = msg.substr(0, 1);
    if ( ident == "i" ) {
	string side = msg.substr(1,1);
	string num = msg.substr(2,3);
	if ( side == "-" ) {
	    fgSetDouble("/instrumentation/gps/cdi-deflection", 0.0);
	} else {
	    int pos = atoi(num.c_str());
	    if ( side == "L" ) {
		pos *= -1;
	    }
	    fgSetDouble("/instrumentation/gps/cdi-deflection",
			(double)pos / 8.0);
	    fgSetBool("/instrumentation/gps/has-gs", false);
	}
    } else if ( ident == "k" ) {
	string ind = msg.substr(1,1);
	if ( ind == "T" ) {
	    fgSetBool("/instrumentation/gps/to-flag", true);
	    fgSetBool("/instrumentation/gps/from-flag", false);
	} else if ( ind == "F" ) {
	    fgSetBool("/instrumentation/gps/to-flag", false);
	    fgSetBool("/instrumentation/gps/from-flag", true);
	} else {
	    fgSetBool("/instrumentation/gps/to-flag", false);
	    fgSetBool("/instrumentation/gps/from-flag", false);
	}
    } else {
	// SG_LOG( SG_IO, SG_ALERT, "unknown AV400Sim message = " << msg );
    }

    return true;
}


// open hailing frequencies
bool FGAV400Sim::open() {
    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    SGIOChannel *io = get_io_channel();

    if ( ! io->open( get_direction() ) ) {
	SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
	return false;
    }

    set_enabled( true );

    return true;
}


// process work for this port
bool FGAV400Sim::process() {
    SGIOChannel *io = get_io_channel();

    // until we have parsers/generators for the reverse direction,
    // this is hardwired to expect that the physical GPS is slaving
    // from FlightGear.

    // Send FlightGear data to the external device
    gen_message();
    if ( ! io->write( buf, length ) ) {
	SG_LOG( SG_IO, SG_WARN, "Error writing data." );
	return false;
    }

    // read the device messages back
    while ( (length = io->readline( buf, FG_MAX_MSG_SIZE )) > 0 ) {
	// SG_LOG( SG_IO, SG_ALERT, "Success reading data." );
	if ( parse_message() ) {
	    // SG_LOG( SG_IO, SG_ALERT, "Success parsing data." );
	} else {
	    // SG_LOG( SG_IO, SG_ALERT, "Error parsing data." );
	}
    }

    return true;
}


// close the channel
bool FGAV400Sim::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
