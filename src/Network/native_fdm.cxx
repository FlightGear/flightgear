// native_fdm.cxx -- FGFS "Native" flight dynamics protocal class
//
// Written by Curtis Olson, started September 2001.
//
// Copyright (C) 2001  Curtis L. Olson - curt@flightgear.org
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


#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>

#include <FDM/flight.hxx>

#include "native_fdm.hxx"


FGNativeFDM::FGNativeFDM() {
}

FGNativeFDM::~FGNativeFDM() {
}


// open hailing frequencies
bool FGNativeFDM::open() {
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

    cur_fdm_state->_set_Sea_level_radius( SG_EQUATORIAL_RADIUS_FT );
    return true;
}


static void global2raw( const FGInterface *global, FGRawFDM *raw ) {
    raw->version = FG_RAW_FDM_VERSION;
    raw->longitude = cur_fdm_state->get_Longitude();
    raw->latitude = cur_fdm_state->get_Latitude();
    raw->altitude = cur_fdm_state->get_Altitude();
    raw->phi = cur_fdm_state->get_Phi();
    raw->theta = cur_fdm_state->get_Theta();
    raw->psi = cur_fdm_state->get_Psi();
}


static void raw2global( const FGRawFDM *raw, FGInterface *global ) {
    if ( raw->version == FG_RAW_FDM_VERSION ) {
	// cout << "pos = " << raw->longitude << " " << raw->latitude << endl;
	// cout << "sea level rad = " << cur_fdm_state->get_Sea_level_radius() << endl;
	cur_fdm_state->_updatePosition( raw->latitude, raw->longitude,
					raw->altitude * SG_METER_TO_FEET );
	cur_fdm_state->_set_Euler_Angles( raw->phi,
					  raw->theta,
					  raw->psi );
    } else {
	SG_LOG( SG_IO, SG_ALERT, "Error: version mismatch in raw2global()" );
	SG_LOG( SG_IO, SG_ALERT,
		"\tsomeone needs to upgrade raw_fdm.hxx and recompile." );
    }
}


// process work for this port
bool FGNativeFDM::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(buf);

    if ( get_direction() == SG_IO_OUT ) {
	// cout << "size of cur_fdm_state = " << length << endl;
	global2raw( cur_fdm_state, &buf );
	if ( ! io->write( (char *)(& buf), length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		raw2global( &buf, cur_fdm_state );
	    }
	} else {
	    while ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		raw2global( &buf, cur_fdm_state );
	    }
	}
    }

    return true;
}


// close the channel
bool FGNativeFDM::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
