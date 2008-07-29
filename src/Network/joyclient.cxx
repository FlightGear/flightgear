// joyclient.cxx -- Agwagon joystick client class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>

#include <Aircraft/controls.hxx>
#include <Main/globals.hxx>

#include "joyclient.hxx"


FGJoyClient::FGJoyClient() {
}

FGJoyClient::~FGJoyClient() {
}


// open hailing frequencies
bool FGJoyClient::open() {
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
bool FGJoyClient::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(int[2]);

    if ( get_direction() == SG_IO_OUT ) {
	SG_LOG( SG_IO, SG_ALERT, "joyclient protocol is read only" );
	return false;
    } else if ( get_direction() == SG_IO_IN ) {
	SG_LOG( SG_IO, SG_DEBUG, "Searching for data." );
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		int *msg;
		msg = (int *)buf;
		SG_LOG( SG_IO, SG_DEBUG, "X = " << msg[0] << " Y = "
			<< msg[1] );
		double aileron = ((double)msg[0] / 2048.0) - 1.0;
		double elevator = ((double)msg[1] / 2048.0) - 1.0;
		if ( fabs(aileron) < 0.05 ) {
		    aileron = 0.0;
		}
		if ( fabs(elevator) < 0.05 ) {
		    elevator = 0.0;
		}
		globals->get_controls()->set_aileron( aileron );
		globals->get_controls()->set_elevator( -elevator );
	    }
	} else {
	    while ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		int *msg;
		msg = (int *)buf;
		SG_LOG( SG_IO, SG_DEBUG, "X = " << msg[0] << " Y = "
			<< msg[1] );
		double aileron = ((double)msg[0] / 2048.0) - 1.0;
		double elevator = ((double)msg[1] / 2048.0) - 1.0;
		if ( fabs(aileron) < 0.05 ) {
		    aileron = 0.0;
		}
		if ( fabs(elevator) < 0.05 ) {
		    elevator = 0.0;
		}
		globals->get_controls()->set_aileron( aileron );
		globals->get_controls()->set_elevator( -elevator );
	    }
	}
    }

    return true;
}


// close the channel
bool FGJoyClient::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
