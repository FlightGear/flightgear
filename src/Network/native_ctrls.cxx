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


static void global2net( const FGControls *global, FGNetCtrls *net ) {
    int i;

    net->version = FG_NET_CTRLS_VERSION;
    net->aileron = globals->get_controls()->get_aileron();
    net->elevator = globals->get_controls()->get_elevator();
    net->elevator_trim = globals->get_controls()->get_elevator_trim();
    net->rudder = globals->get_controls()->get_rudder();
    net->flaps = globals->get_controls()->get_flaps();
    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {    
	net->throttle[i] = globals->get_controls()->get_throttle(i);
	net->mixture[i] = globals->get_controls()->get_mixture(i);
	net->prop_advance[i] = globals->get_controls()->get_prop_advance(i);
    }
    for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
        net->fuel_selector[i] = globals->get_controls()->get_fuel_selector(i);
    }
    for ( i = 0; i < FGNetCtrls::FG_MAX_WHEELS; ++i ) {
	net->brake[i] =  globals->get_controls()->get_brake(i);
    }

    net->hground = globals->get_scenery()->get_cur_elev();
}


static void net2global( const FGNetCtrls *net, FGControls *global ) {
    int i;

    if ( net->version == FG_NET_CTRLS_VERSION ) {
	globals->get_controls()->set_aileron( net->aileron );
	globals->get_controls()->set_elevator( net->elevator );
	globals->get_controls()->set_elevator_trim( net->elevator_trim );
	globals->get_controls()->set_rudder( net->rudder );
	globals->get_controls()->set_flaps( net->flaps );
	for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {    
	    globals->get_controls()->set_throttle( i, net->throttle[i] );
	    globals->get_controls()->set_mixture( i, net->mixture[i] );
	    globals->get_controls()->set_prop_advance( i, net->prop_advance[i]);
	}
	for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
	    globals->get_controls()->set_fuel_selector( i, net->fuel_selector[i] );
	}
	for ( i = 0; i < FGNetCtrls::FG_MAX_WHEELS; ++i ) {
	    globals->get_controls()->set_brake( i, net->brake[i] );
	}
	globals->get_controls()->set_gear_down( net->gear_handle );
	globals->get_scenery()->set_cur_elev( net->hground );
    } else {
	SG_LOG( SG_IO, SG_ALERT, "Error: version mismatch in net2global()" );
	SG_LOG( SG_IO, SG_ALERT,
		"\tsomeone needs to upgrade net_ctrls.hxx and recompile." );
    }
}


// process work for this port
bool FGNativeCtrls::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(FGNetCtrls);

    if ( get_direction() == SG_IO_OUT ) {
	// cout << "size of cur_fdm_state = " << length << endl;

	global2net( globals->get_controls(), &net_ctrls );

	if ( ! io->write( (char *)(& net_ctrls), length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& net_ctrls), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		net2global( &net_ctrls, globals->get_controls() );
	    }
	} else {
	    while ( io->read( (char *)(& net_ctrls), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		net2global( &net_ctrls, globals->get_controls() );
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
