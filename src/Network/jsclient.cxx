// jsclient.cxx -- simple UDP networked joystick client
//
// Copyright (C) 2003 by Manuel Bessler and Stephen Lowry
//
// based on joyclient.cxx by Curtis Olson
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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

#include <Aircraft/aircraft.hxx>

#include "jsclient.hxx"


FGJsClient::FGJsClient() {
}

FGJsClient::~FGJsClient() {
}


// open hailing frequencies
bool FGJsClient::open() {
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
bool FGJsClient::process() {
    SGIOChannel *io = get_io_channel();
    int length = 4+4+4+4+4+4;

    if ( get_direction() == SG_IO_OUT ) {
	SG_LOG( SG_IO, SG_ALERT, "JsClient protocol is read only" );
	return false;
    } else if ( get_direction() == SG_IO_IN ) {
	SG_LOG( SG_IO, SG_DEBUG, "Searching for data." );
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		long int *msg;
		msg = (long int *)buf;
		SG_LOG( SG_IO, SG_DEBUG, "ax0 = " << msg[0] << " ax1 = "
			<< msg[1] << "ax2 = " << msg[2] << "ax3 = " << msg[3]);
		double axis1 = ((double)msg[0] / 2147483647.0);
		double axis2 = ((double)msg[1] / 2147483647.0);
		if ( fabs(axis1) < 0.05 ) {
		    axis1 = 0.0;
		}
		if ( fabs(axis2) < 0.05 ) {
		    axis2 = 0.0;
		}
		globals->get_controls()->set_throttle(FGControls::ALL_ENGINES, axis1);
//		globals->get_controls()->set_aileron( axis1 );
//		globals->get_controls()->set_elevator( -axis2 );
	    }
	} else {
	    while ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		long int *msg;
		msg = (long int *)buf;
		SG_LOG( SG_IO, SG_DEBUG, "ax0 = " << msg[0] << " ax1 = "
			<< msg[1] << "ax2 = " << msg[2] << "ax3 = " << msg[3]);
		double axis1 = ((double)msg[0] / 2147483647.0);
		double axis2 = ((double)msg[1] / 2147483647.0);
		if ( fabs(axis1) < 0.05 ) {
		    axis1 = 0.0;
		}
		if ( fabs(axis2) < 0.05 ) {
		    axis2 = 0.0;
		}
		globals->get_controls()->set_throttle(FGControls::ALL_ENGINES, axis1);
//		globals->get_controls()->set_aileron( axis1 );
//		globals->get_controls()->set_elevator( -axis2 );
	    }
	}
    }

    return true;
}


// close the channel
bool FGJsClient::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
