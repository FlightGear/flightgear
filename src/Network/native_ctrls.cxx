// native_ctrls.cxx -- FGFS "Native" controls I/O class
//
// Written by Curtis Olson, started July 2001.
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

#include <Scenery/scenery.hxx>	// ground elevation

#include "native_ctrls.hxx"


FGNativeCtrls::FGNativeCtrls() {
}

FGNativeCtrls::~FGNativeCtrls() {
}


// open hailing frequencies
bool FGNativeCtrls::open() {
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


static void global2raw( const FGControls *global, FGRawCtrls *raw ) {
    int i;

    raw->version = FG_RAW_CTRLS_VERSION;
    raw->aileron = globals->get_controls()->get_aileron();
    raw->elevator = globals->get_controls()->get_elevator();
    raw->elevator_trim = globals->get_controls()->get_elevator_trim();
    raw->rudder = globals->get_controls()->get_rudder();
    raw->flaps = globals->get_controls()->get_flaps();
    for ( i = 0; i < FG_MAX_ENGINES; ++i ) {    
	raw->throttle[i] = globals->get_controls()->get_throttle(i);
	raw->mixture[i] = globals->get_controls()->get_mixture(i);
	raw->prop_advance[i] = globals->get_controls()->get_prop_advance(i);
    }
    for ( i = 0; i < FG_MAX_WHEELS; ++i ) {
	raw->brake[i] =  globals->get_controls()->get_brake(i);
    }

    raw->hground = scenery.get_cur_elev();
}


static void raw2global( const FGRawCtrls *raw, FGControls *global ) {
    int i;

    if ( raw->version == FG_RAW_CTRLS_VERSION ) {
	globals->get_controls()->set_aileron( raw->aileron );
	globals->get_controls()->set_elevator( raw->elevator );
	globals->get_controls()->set_elevator_trim( raw->elevator_trim );
	globals->get_controls()->set_rudder( raw->rudder );
	globals->get_controls()->set_flaps( raw->flaps );
	for ( i = 0; i < FG_MAX_ENGINES; ++i ) {    
	    globals->get_controls()->set_throttle( i, raw->throttle[i] );
	    globals->get_controls()->set_mixture( i, raw->mixture[i] );
	    globals->get_controls()->set_prop_advance( i, raw->prop_advance[i]);
	}
	for ( i = 0; i < FG_MAX_WHEELS; ++i ) {
	    globals->get_controls()->set_brake( i, raw->brake[i] );
	}
	scenery.set_cur_elev( raw->hground );
    } else {
	SG_LOG( SG_IO, SG_ALERT, "Error: version mismatch in raw2global()" );
	SG_LOG( SG_IO, SG_ALERT,
		"\tsomeone needs to upgrade raw_ctrls.hxx and recompile." );
    }
}


// process work for this port
bool FGNativeCtrls::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(FGRawCtrls);

    if ( get_direction() == SG_IO_OUT ) {
	// cout << "size of cur_fdm_state = " << length << endl;

	global2raw( globals->get_controls(), &raw_ctrls );

	if ( ! io->write( (char *)(& raw_ctrls), length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& raw_ctrls), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		raw2global( &raw_ctrls, globals->get_controls() );
	    }
	} else {
	    while ( io->read( (char *)(& raw_ctrls), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		raw2global( &raw_ctrls, globals->get_controls() );
	    }
	}
    }

    return true;
}


// close the channel
bool FGNativeCtrls::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
