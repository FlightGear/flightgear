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
    raw->flaps_power
            = node->getDoubleValue( "/systems/electrical/outputs/flaps",
                                    1.0 ) >= 1.0;
    raw->num_engines = FGRawCtrls::FG_MAX_ENGINES;
    for ( i = 0; i < FGRawCtrls::FG_MAX_ENGINES; ++i ) {
	raw->throttle[i] = node->getDoubleValue( "throttle", 0.0 );
	raw->mixture[i] = node->getDoubleValue( "mixture", 0.0 );
	raw->fuel_pump_power[i]
            = node->getDoubleValue( "/systems/electrical/outputs/fuel-pump",
                                    1.0 ) >= 1.0;
	raw->prop_advance[i] = node->getDoubleValue( "propeller-pitch", 0.0 );
	raw->magnetos[i] = node->getIntValue( "magnetos", 0 );
	if ( i == 0 ) {
	  // cout << "Magnetos -> " << node->getIntValue( "magnetos", 0 );
	}
	raw->starter_power[i]
            = node->getDoubleValue( "/systems/electrical/outputs/starter",
                                    1.0 ) >= 1.0;
	if ( i == 0 ) {
	  // cout << " Starter -> " << node->getIntValue( "stater", false )
	  //      << endl;
	}
    }
    raw->num_tanks = FGRawCtrls::FG_MAX_TANKS;
    for ( i = 0; i < FGRawCtrls::FG_MAX_TANKS; ++i ) {
        if ( node->getChild("fuel-selector", i) != 0 ) {
            raw->fuel_selector[i]
                = node->getChild("fuel-selector", i)->getDoubleValue();
        } else {
            raw->fuel_selector[i] = false;
        }
    }
    raw->num_wheels = FGRawCtrls::FG_MAX_WHEELS;
    for ( i = 0; i < FGRawCtrls::FG_MAX_WHEELS; ++i ) {
        if ( node->getChild("brakes", i) != 0 ) {
            raw->brake[i]
                = node->getChild("brakes", i)->getDoubleValue();
        } else {
            raw->brake[i] = 0.0;
        }
    }

    node = fgGetNode("/controls/switches", true);
    raw->master_bat = node->getChild("master-bat")->getBoolValue();
    raw->master_alt = node->getChild("master-alt")->getBoolValue();
    raw->master_avionics = node->getChild("master-avionics")->getBoolValue();

    // cur_fdm_state->get_ground_elev_ft() is what we want ... this
    // reports the altitude of the aircraft.
    // "/environment/ground-elevation-m" reports the ground elevation
    // of the current view point which could change substantially if
    // the user is switching views.
    raw->hground = cur_fdm_state->get_ground_elev_ft() * SG_FEET_TO_METER;
    raw->magvar = fgGetDouble("/environment/magnetic-variation-deg");
    raw->speedup = fgGetInt("/sim/speed-up");

    // convert to network byte order
    raw->version = htonl(raw->version);
    htond(raw->aileron);
    htond(raw->elevator);
    htond(raw->elevator_trim);
    htond(raw->rudder);
    htond(raw->flaps);
    raw->flaps_power = htonl(raw->flaps_power);
    for ( i = 0; i < FGRawCtrls::FG_MAX_ENGINES; ++i ) {
	htond(raw->throttle[i]);
	htond(raw->mixture[i]);
        raw->fuel_pump_power[i] = htonl(raw->fuel_pump_power[i]);
	htond(raw->prop_advance[i]);
	raw->magnetos[i] = htonl(raw->magnetos[i]);
	raw->starter_power[i] = htonl(raw->starter_power[i]);
    }
    raw->num_engines = htonl(raw->num_engines);
    for ( i = 0; i < FGRawCtrls::FG_MAX_TANKS; ++i ) {
        raw->fuel_selector[i] = htonl(raw->fuel_selector[i]);
    }
    raw->num_tanks = htonl(raw->num_tanks);
    for ( i = 0; i < FGRawCtrls::FG_MAX_WHEELS; ++i ) {
	htond(raw->brake[i]);
    }
    raw->num_wheels = htonl(raw->num_wheels);
    raw->gear_handle = htonl(raw->gear_handle);
    raw->master_bat = htonl(raw->master_bat);
    raw->master_alt = htonl(raw->master_alt);
    raw->master_avionics = htonl(raw->master_avionics);
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
    htond(net->v_north);
    htond(net->v_east);
    htond(net->v_down);
    htond(net->stall_warning);

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
    // I don't need to convert the Wow flags, since they are one byte in size
    htond(net->flap_deflection);

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
        cur_fdm_state->_set_Velocities_Local( net->v_north,
                                              net->v_east,
                                              net->v_down );
        fgSetDouble( "/sim/aero/alarms/stall-warning", net->stall_warning );
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

        fgSetDouble("/surface-positions/flap-pos-norm", net->flap_deflection);
	SGPropertyNode * node = fgGetNode("/controls", true);
        fgSetDouble("/surface-positions/elevator-pos-norm", 
		    node->getDoubleValue( "elevator" ));
        fgSetDouble("/surface-positions/rudder-pos-norm", 
		    node->getDoubleValue( "rudder" ));
        fgSetDouble("/surface-positions/left-aileron-pos-norm", 
		    node->getDoubleValue( "aileron" ));
        fgSetDouble("/surface-positions/right-aileron-pos-norm", 
		    -node->getDoubleValue( "aileron" ));

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
//     set_delta_t( dt );

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

    // disable blocking
    data_server.setBlocking( false );

    // allowed to read from a broadcast addr
    // data_server.setBroadcast( true );

    // if we bind to fdm_host = "" then we accept messages from
    // anyone.
    if ( data_server.bind( "", data_in_port ) == -1 ) {
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

    HTTPClient *http;
    sprintf( cmd, "/longitude-deg?value=%.8f", lon );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone() ) http->poll(0);
    delete http;

    sprintf( cmd, "/latitude-deg?value=%.8f", lat );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone() ) http->poll(0);
    delete http;

    sprintf( cmd, "/ground-m?value=%.8f", ground );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone() ) http->poll(0);
    delete http;

    sprintf( cmd, "/heading-deg?value=%.8f", heading );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone() ) http->poll(0);
    delete http;

    SG_LOG( SG_IO, SG_INFO, "before sending reset command." );

    sprintf( cmd, "/reset?value=ground" );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone() ) http->poll(0);
    delete http;

    SG_LOG( SG_IO, SG_INFO, "Remote FDM init() finished." );
}


// Run an iteration of the EOM.
void FGExternalNet::update( double dt ) {
    int length;
    int result;

    if (is_suspended())
      return;

    // Send control positions to remote fdm
    length = sizeof(ctrls);
    global2raw( &ctrls );
    if ( data_client.send( (char *)(& ctrls), length, 0 ) != length ) {
	SG_LOG( SG_IO, SG_DEBUG, "Error writing data." );
    } else {
	SG_LOG( SG_IO, SG_DEBUG, "wrote control data." );
    }

    // Read next set of FDM data (blocking enabled to maintain 'sync')
    length = sizeof(fdm);
    while ( (result = data_server.recv( (char *)(& fdm), length, 0)) >= 0 ) {
	SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
	net2global( &fdm );
    }
}
