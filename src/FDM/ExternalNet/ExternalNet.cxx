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

#if defined( linux )
#  include <sys/types.h>
#  include <sys/socket.h>
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


static void global2net( FGNetCtrls *net ) {
    int i;

    // fill in values
    SGPropertyNode * node = fgGetNode("/controls", true);
    net->version = FG_NET_CTRLS_VERSION;
    net->aileron = node->getDoubleValue( "aileron" );
    net->elevator = node->getDoubleValue( "elevator" );
    net->elevator_trim = node->getDoubleValue( "elevator-trim" );
    net->rudder = node->getDoubleValue( "rudder" );
    net->flaps = node->getDoubleValue( "flaps" );
    net->flaps_power
            = node->getDoubleValue( "/systems/electrical/outputs/flaps",
                                    1.0 ) >= 1.0;
    net->num_engines = FGNetCtrls::FG_MAX_ENGINES;
    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
	net->throttle[i] = node->getDoubleValue( "throttle", 0.0 );
	net->mixture[i] = node->getDoubleValue( "mixture", 0.0 );
	net->fuel_pump_power[i]
            = node->getDoubleValue( "/systems/electrical/outputs/fuel-pump",
                                    1.0 ) >= 1.0;
	net->prop_advance[i] = node->getDoubleValue( "propeller-pitch", 0.0 );
	net->magnetos[i] = node->getIntValue( "magnetos", 0 );
	if ( i == 0 ) {
	  // cout << "Magnetos -> " << node->getIntValue( "magnetos", 0 );
	}
	net->starter_power[i]
            = node->getDoubleValue( "/systems/electrical/outputs/starter",
                                    1.0 ) >= 1.0;
	if ( i == 0 ) {
	  // cout << " Starter -> " << node->getIntValue( "stater", false )
	  //      << endl;
	}
    }
    net->num_tanks = FGNetCtrls::FG_MAX_TANKS;
    for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
        if ( node->getChild("fuel-selector", i) != 0 ) {
            net->fuel_selector[i]
                = node->getChild("fuel-selector", i)->getDoubleValue();
        } else {
            net->fuel_selector[i] = false;
        }
    }
    net->num_wheels = FGNetCtrls::FG_MAX_WHEELS;
    for ( i = 0; i < FGNetCtrls::FG_MAX_WHEELS; ++i ) {
        if ( node->getChild("brakes", i) != 0 ) {
            if ( node->getChild("parking-brake")->getDoubleValue() > 0.0 ) {
                net->brake[i] = 1.0;
           } else {
                net->brake[i]
                    = node->getChild("brakes", i)->getDoubleValue();
            }
        } else {
            net->brake[i] = 0.0;
        }
    }

    node = fgGetNode("/controls/switches", true);
    net->master_bat = node->getChild("master-bat")->getBoolValue();
    net->master_alt = node->getChild("master-alt")->getBoolValue();
    net->master_avionics = node->getChild("master-avionics")->getBoolValue();

    net->wind_speed_kt = fgGetDouble("/environment/wind-speed-kt");
    net->wind_dir_deg = fgGetDouble("/environment/wind-from-heading-deg");

    // cur_fdm_state->get_ground_elev_ft() is what we want ... this
    // reports the altitude of the aircraft.
    // "/environment/ground-elevation-m" reports the ground elevation
    // of the current view point which could change substantially if
    // the user is switching views.
    net->hground = cur_fdm_state->get_ground_elev_ft() * SG_FEET_TO_METER;
    net->magvar = fgGetDouble("/environment/magnetic-variation-deg");
    net->speedup = fgGetInt("/sim/speed-up");
    net->freeze = 0;
    if ( fgGetBool("/sim/freeze/master") ) {
        net->freeze |= 0x01;
    }
    if ( fgGetBool("/sim/freeze/position") ) {
        net->freeze |= 0x02;
    }
    if ( fgGetBool("/sim/freeze/fuel") ) {
        net->freeze |= 0x04;
    }

    // convert to network byte order
    net->version = htonl(net->version);
    htond(net->aileron);
    htond(net->elevator);
    htond(net->elevator_trim);
    htond(net->rudder);
    htond(net->flaps);
    net->flaps_power = htonl(net->flaps_power);
    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
	htond(net->throttle[i]);
	htond(net->mixture[i]);
        net->fuel_pump_power[i] = htonl(net->fuel_pump_power[i]);
	htond(net->prop_advance[i]);
	net->magnetos[i] = htonl(net->magnetos[i]);
	net->starter_power[i] = htonl(net->starter_power[i]);
    }
    net->num_engines = htonl(net->num_engines);
    for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
        net->fuel_selector[i] = htonl(net->fuel_selector[i]);
    }
    net->num_tanks = htonl(net->num_tanks);
    for ( i = 0; i < FGNetCtrls::FG_MAX_WHEELS; ++i ) {
	htond(net->brake[i]);
    }
    net->num_wheels = htonl(net->num_wheels);
    net->gear_handle = htonl(net->gear_handle);
    net->master_bat = htonl(net->master_bat);
    net->master_alt = htonl(net->master_alt);
    net->master_avionics = htonl(net->master_avionics);
    htond(net->wind_speed_kt);
    htond(net->wind_dir_deg);
    htond(net->hground);
    htond(net->magvar);
    net->speedup = htonl(net->speedup);
    net->freeze = htonl(net->freeze);
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
    htond(net->v_wind_body_north);
    htond(net->v_wind_body_east);
    htond(net->v_wind_body_down);
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
        cur_fdm_state->_set_Velocities_Wind_Body( net->v_wind_body_north,
                                                  net->v_wind_body_east,
                                                  net->v_wind_body_down );

        fgSetDouble( "/sim/alarms/stall-warning", net->stall_warning );
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

#if defined( linux )
    // set SO_REUSEADDR flag
    int socket = data_server.getHandle();
    int one = 1;
    int result;
    result = ::setsockopt( socket, SOL_SOCKET, SO_REUSEADDR,
                           &one, sizeof(one) );
#endif

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

    double lon = fgGetDouble( "/sim/presets/longitude-deg" );
    double lat = fgGetDouble( "/sim/presets/latitude-deg" );
    double alt = fgGetDouble( "/sim/presets/altitude-ft" );
    double ground = fgGetDouble( "/environment/ground-elevation-m" );
    double heading = fgGetDouble("/sim/presets/heading-deg");
    double speed = fgGetDouble( "/sim/presets/airspeed-kt" );

    char cmd[256];

    HTTPClient *http;
    sprintf( cmd, "/longitude-deg?value=%.8f", lon );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone(1000000) ) http->poll(0);
    delete http;

    sprintf( cmd, "/latitude-deg?value=%.8f", lat );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone(1000000) ) http->poll(0);
    delete http;

    sprintf( cmd, "/altitude-ft?value=%.8f", alt );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone(1000000) ) http->poll(0);
    delete http;

    sprintf( cmd, "/ground-m?value=%.8f", ground );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone(1000000) ) http->poll(0);
    delete http;

    sprintf( cmd, "/speed-kts?value=%.8f", speed );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone(1000000) ) http->poll(0);
    delete http;

    sprintf( cmd, "/heading-deg?value=%.8f", heading );
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone(1000000) ) http->poll(0);
    delete http;

    SG_LOG( SG_IO, SG_INFO, "before sending reset command." );

    if( fgGetBool("/sim/presets/onground") ) {
      sprintf( cmd, "/reset?value=ground" );
    } else {
      sprintf( cmd, "/reset?value=air" );
    }
    http = new HTTPClient( fdm_host.c_str(), cmd_port, cmd );
    while ( !http->isDone(1000000) ) http->poll(0);
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
    global2net( &ctrls );
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
