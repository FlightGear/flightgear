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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests
#include <simgear/io/iochannel.hxx>

#include <FDM/flight.hxx>
#include <Main/globals.hxx>

#include "native_fdm.hxx"

// FreeBSD works better with this included last ... (?)
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <windows.h>
#else
#  include <netinet/in.h>	// htonl() ntohl()
#endif


// The function htond is defined this way due to the way some
// processors and OSes treat floating point values.  Some will raise
// an exception whenever a "bad" floating point value is loaded into a
// floating point register.  Solaris is notorious for this, but then
// so is LynxOS on the PowerPC.  By translating the data in place,
// there is no need to load a FP register with the "corruped" floating
// point value.  By doing the BIG_ENDIAN test, I can optimize the
// routine for big-endian processors so it can be as efficient as
// possible
static void htond (double &x)	
{
    if ( sgIsLittleEndian() ) {
        int    *Double_Overlay;
        int     Holding_Buffer;
    
        Double_Overlay = (int *) &x;
        Holding_Buffer = Double_Overlay [0];
    
        Double_Overlay [0] = htonl (Double_Overlay [1]);
        Double_Overlay [1] = htonl (Holding_Buffer);
    } else {
        return;
    }
}


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


static void global2net( const FGInterface *global, FGNetFDM *net ) {
    net->version = FG_NET_FDM_VERSION;

    // positions
    net->longitude = cur_fdm_state->get_Longitude();
    net->latitude = cur_fdm_state->get_Latitude();
    net->altitude = cur_fdm_state->get_Altitude() * SG_FEET_TO_METER;
    net->phi = cur_fdm_state->get_Phi();
    net->theta = cur_fdm_state->get_Theta();
    net->psi = cur_fdm_state->get_Psi();

    // velocities
    net->vcas = cur_fdm_state->get_V_calibrated_kts();
    net->climb_rate = cur_fdm_state->get_Climb_Rate();

    // time
    net->cur_time = globals->get_time_params()->get_cur_time();
    net->warp = globals->get_warp();

    // Convert the net buffer to network format
    net->version = htonl(net->version);
    htond(net->longitude);
    htond(net->latitude);
    htond(net->altitude);
    htond(net->phi);
    htond(net->theta);
    htond(net->psi);
    htond(net->vcas);
    htond(net->climb_rate);
    net->cur_time = htonl( net->cur_time );
    net->warp = htonl( net->warp );
}


static void net2global( FGNetFDM *net, FGInterface *global ) {

    // Convert to the net buffer from network format
    net->version = htonl(net->version);
    htond(net->longitude);
    htond(net->latitude);
    htond(net->altitude);
    htond(net->phi);
    htond(net->theta);
    htond(net->psi);
    htond(net->vcas);
    htond(net->climb_rate);
    net->cur_time = htonl(net->cur_time);
    net->warp = htonl(net->warp);

    if ( net->version == FG_NET_FDM_VERSION ) {
	// cout << "pos = " << net->longitude << " " << net->latitude << endl;
	// cout << "sea level rad = " << cur_fdm_state->get_Sea_level_radius() << endl;
	cur_fdm_state->_updateGeodeticPosition( net->latitude,
						net->longitude,
						net->altitude
						  * SG_METER_TO_FEET );
	cur_fdm_state->_set_Euler_Angles( net->phi,
					  net->theta,
					  net->psi );
	cur_fdm_state->_set_V_calibrated_kts( net->vcas );
	cur_fdm_state->_set_Climb_Rate( net->climb_rate );

        globals->set_warp( net->warp );
    } else {
	SG_LOG( SG_IO, SG_ALERT, "Error: version mismatch in net2global()" );
	SG_LOG( SG_IO, SG_ALERT,
		"\tsomeone needs to upgrade net_fdm.hxx and recompile." );
    }
}


// process work for this port
bool FGNativeFDM::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(buf);

    if ( get_direction() == SG_IO_OUT ) {
	// cout << "size of cur_fdm_state = " << length << endl;
	global2net( cur_fdm_state, &buf );
	if ( ! io->write( (char *)(& buf), length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		net2global( &buf, cur_fdm_state );
	    }
	} else {
	    while ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		net2global( &buf, cur_fdm_state );
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
