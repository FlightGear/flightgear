// ExternalNet.hxx -- an net interface to an external flight dynamics model
//
// Written by Curtis Olson, started November 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - curt@flightgear.org
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
#include <simgear/io/lowlevel.hxx> // endian tests

#include <Main/fg_props.hxx>

#include "ExternalNet.hxx"


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


static void global2raw( FGRawCtrls *raw ) {
    int i;

    // fill in values
    SGPropertyNode * node = fgGetNode("/controls", true);
    raw->version = FG_RAW_CTRLS_VERSION;
    raw->aileron = node->getDoubleValue( "aileron" );
    raw->elevator = node->getDoubleValue( "elevator" );
    raw->elevator_trim = node->getDoubleValue( "elevator-trim" );
    raw->rudder = node->getDoubleValue( "rudder" );
    raw->flaps = node->getDoubleValue( "flaps" );
    for ( i = 0; i < FGRawCtrls::FG_MAX_ENGINES; ++i ) {
	raw->throttle[i] = node->getDoubleValue( "throttle", i );
	raw->mixture[i] = node->getDoubleValue( "mixture", i );
	raw->prop_advance[i] = node->getDoubleValue( "propeller-pitch", i );
	raw->magnetos[i] = node->getIntValue( "magnetos", i );
	raw->starter[i] = node->getBoolValue( "starter", i );
    }
    for ( i = 0; i < FGRawCtrls::FG_MAX_WHEELS; ++i ) {
	raw->brake[i] = node->getDoubleValue( "brakes", i );
    }
    raw->hground = fgGetDouble( "/environment/ground-elevation-m" );
    raw->magvar = fgGetDouble("/environment/magnetic-variation-deg");
    raw->speedup = fgGetInt("/sim/speed-up");

    // convert to network byte order
    raw->version = htonl(raw->version);
    htond(raw->aileron);
    htond(raw->elevator);
    htond(raw->elevator_trim);
    htond(raw->rudder);
    htond(raw->flaps);
    for ( i = 0; i < FGRawCtrls::FG_MAX_ENGINES; ++i ) {
	htond(raw->throttle[i]);
	htond(raw->mixture[i]);
	htond(raw->prop_advance[i]);
	htonl(raw->magnetos[i]);
	htonl(raw->starter[i]);
    }
    for ( i = 0; i < FGRawCtrls::FG_MAX_WHEELS; ++i ) {
	htond(raw->brake[i]);
    }
    htond(raw->hground);
    htond(raw->magvar);
    raw->speedup = htonl(raw->speedup);
}


static void net2global( FGNetFDM *net ) {
    int i;

    // Convert to the net buffer from network format
    net->version = ntohl(net->version);

    htond(net->longitude);
    htond(net->latitude);
    htond(net->altitude);
    htond(net->phi);
    htond(net->theta);
    htond(net->psi);

    htond(net->phidot);
    htond(net->thetadot);
    htond(net->psidot);
    htond(net->vcas);
    htond(net->climb_rate);

    htond(net->A_X_pilot);
    htond(net->A_Y_pilot);
    htond(net->A_Z_pilot);

    net->num_engines = htonl(net->num_engines);
    for ( i = 0; i < net->num_engines; ++i ) {
	htonl(net->eng_state[i]);
	htond(net->rpm[i]);
	htond(net->fuel_flow[i]);
	htond(net->EGT[i]);
	htond(net->oil_temp[i]);
	htond(net->oil_px[i]);
    }

    net->num_tanks = htonl(net->num_tanks);
    for ( i = 0; i < net->num_tanks; ++i ) {
	htond(net->fuel_quantity[i]);
    }

    net->num_wheels = htonl(net->num_wheels);
    for ( i = 0; i < net->num_wheels; ++i ) {
	net->wow[i] = htonl(net->wow[i]);
    }

    net->cur_time = ntohl(net->cur_time);
    net->warp = ntohl(net->warp);
    htond(net->visibility);

    if ( net->version == FG_NET_FDM_VERSION ) {
        // cout << "pos = " << net->longitude << " " << net->latitude << endl;
        // cout << "sea level rad = " << cur_fdm_state->get_Sea_level_radius()
	//      << endl;
        cur_fdm_state->_updateGeodeticPosition( net->latitude,
                                                net->longitude,
                                                net->altitude
                                                  * SG_METER_TO_FEET );
        cur_fdm_state->_set_Euler_Angles( net->phi,
                                          net->theta,
                                          net->psi );
        cur_fdm_state->_set_Euler_Rates( net->phidot,
					 net->thetadot,
					 net->psidot );
        cur_fdm_state->_set_V_calibrated_kts( net->vcas );
        cur_fdm_state->_set_Climb_Rate( net->climb_rate );
        cur_fdm_state->_set_Accels_Pilot_Body( net->A_X_pilot,
					       net->A_Y_pilot,
					       net->A_Z_pilot );

	for ( i = 0; i < net->num_engines; ++i ) {
	    SGPropertyNode *node = fgGetNode( "engines/engine", i, true );
	    
	    // node->setBoolValue("running", t->isRunning());
	    // node->setBoolValue("cranking", t->isCranking());
	    
	    // cout << net->eng_state[i] << endl;
	    if ( net->eng_state[i] == 0 ) {
		node->setBoolValue( "cranking", false );
		node->setBoolValue( "running", false );
	    } else if ( net->eng_state[i] == 1 ) {
		node->setBoolValue( "cranking", true );
		node->setBoolValue( "running", false );
	    } else if ( net->eng_state[i] == 2 ) {
		node->setBoolValue( "cranking", false );
		node->setBoolValue( "running", true );
	    }

	    node->setDoubleValue( "rpm", net->rpm[i] );
	    node->setDoubleValue( "fuel-flow-gph", net->fuel_flow[i] );
	    node->setDoubleValue( "egt-degf", net->EGT[i] );
	    node->setDoubleValue( "oil-temperature-degf", net->oil_temp[i] );
	    node->setDoubleValue( "oil-pressure-psi", net->oil_px[i] );		
	}

	for (i = 0; i < net->num_tanks; ++i ) {
	    SGPropertyNode * node
		= fgGetNode("/consumables/fuel/tank", i, true);
	    node->setDoubleValue("level-gal_us", net->fuel_quantity[i] );
	}

	for (i = 0; i < net->num_wheels; ++i ) {
	    SGPropertyNode * node
		= fgGetNode("/gear/gear", i, true);
	    node->setDoubleValue("wow", net->wow[i] );
	}

	/* these are ignored for now  ... */
	/*
	if ( net->cur_time ) {
	    fgSetLong("/sim/time/cur-time-override", net->cur_time);
	}

        globals->set_warp( net->warp );
        last_warp = net->warp;
	*/
    } else {
	SG_LOG( SG_IO, SG_ALERT, "Error: version mismatch in net2global()" );
	SG_LOG( SG_IO, SG_ALERT,
		"\tread " << net->version << " need " << FG_NET_FDM_VERSION );
	SG_LOG( SG_IO, SG_ALERT,
		"\tsomeone needs to upgrade net_fdm.hxx and recompile." );
    }
}


FGExternalNet::FGExternalNet( double dt, string host, int dop, int dip, int cp )
{
    set_delta_t( dt );

    valid = true;

    data_in_port = dip;
    data_out_port = dop;
    cmd_port = cp;
    fdm_host = host;

    /////////////////////////////////////////////////////////
    // Setup client udp connection (sends data to remote fdm)

    if ( ! data_client.open( false ) ) {
	SG_LOG( SG_FLIGHT, SG_ALERT, "Error opening client data channel" );
	valid = false;
    }

    // fire and forget
    data_client.setBlocking( false );

    if ( data_client.connect( fdm_host.c_str(), data_out_port ) == -1 ) {
        printf("error connecting to %s:%d\n", fdm_host.c_str(), data_out_port);
	valid = false;
    }

    /////////////////////////////////////////////////////////
    // Setup server udp connection (for receiving data)

    if ( ! data_server.open( false ) ) {
	SG_LOG( SG_FLIGHT, SG_ALERT, "Error opening client server channel" );
	valid = false;
    }

    // we want to block for incoming data in order to syncronize frame
    // rates.
    data_server.setBlocking( false /* don't block while testing */ );
    // data_server.setBlocking( true /* don't block while testing */ );

    // if we bind to fdm_host = "" then we accept messages from
    // anyone.
    if ( data_server.bind( fdm_host.c_str(), data_in_port ) == -1 ) {
        printf("error binding to port %d\n", data_in_port);
	valid = false;
    }
}


FGExternalNet::~FGExternalNet() {
    data_client.close();
    data_server.close();
}


// Initialize the ExternalNet flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGExternalNet::init() {
    // cout << "FGExternalNet::init()" << endl;

    // Explicitly call the superclass's
    // init method first.
    common_init();

    double lon = fgGetDouble( "/position/longitude-deg" );
    double lat = fgGetDouble( "/position/latitude-deg" );
    double ground = fgGetDouble( "/environment/ground-elevation-m" );
    double heading = fgGetDouble("/orientation/heading-deg");

    char cmd[256];

    sprintf( cmd, "/longitude-deg?value=%.8f", lon );
    new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
//     cout << "before loop()" << endl;
    netChannel::loop(0);
//     cout << "here" << endl;

    sprintf( cmd, "/latitude-deg?value=%.8f", lat );
    new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    netChannel::loop(0);

    sprintf( cmd, "/ground-m?value=%.8f", ground );
    new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    netChannel::loop(0);

    sprintf( cmd, "/heading-deg?value=%.8f", heading );
    new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    netChannel::loop(0);

    sprintf( cmd, "/reset?value=ground" );
    new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    netChannel::loop(0);
}


// Run an iteration of the EOM.  This is a NOP here because the flight
// model values are getting filled in elsewhere (most likely from some
// external source.)
void FGExternalNet::update( int multiloop ) {
    int length;
    int result;

    // Send control positions to remote fdm
    length = sizeof(ctrls);
    global2raw( &ctrls );
    if ( data_client.send( (char *)(& ctrls), length, 0 ) != length ) {
	SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
    }

    // Read next set of FDM data (blocking enabled to maintain 'sync')
    length = sizeof(fdm);
    if ( (result = data_server.recv( (char *)(& fdm), length, 0)) >= 0 ) {
	SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
	net2global( &fdm );
    }
}
