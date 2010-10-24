// ExternalNet.hxx -- an net interface to an external flight dynamics model
//
// Written by Curtis Olson, started November 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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
#  include <config.h>
#endif

#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests
#include <simgear/io/sg_netBuffer.hxx>

#include <Main/fg_props.hxx>
#include <Network/native_ctrls.hxx>
#include <Network/native_fdm.hxx>

#include "ExternalNet.hxx"


class HTTPClient : public simgear::NetBufferChannel
{

    bool done;
    SGTimeStamp start;

public:

    HTTPClient ( const char* host, int port, const char* path ) :
        done( false )
    {
	open ();
	connect (host, port);

  char buffer[256];
  ::snprintf (buffer, 256, "GET %s HTTP/1.0\r\n\r\n", path );
	bufferSend(buffer, strlen(buffer) ) ;

        start.stamp();
    }

    virtual void handleBufferRead (simgear::NetBuffer& buffer)
    {
	const char* s = buffer.getData();
	while (*s)
	    fputc(*s++,stdout);

	printf("done\n");
	buffer.remove();
	printf("after buffer.remove()\n");
        done = true;
    }

    bool isDone() const { return done; }
    bool isDone( long usec ) const { 
        if ( start + SGTimeStamp::fromUSec(usec) < SGTimeStamp::now() ) {
            return true;
        } else {
            return done;
        }
    }
};

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

    double lon = fgGetDouble( "/sim/presets/longitude-deg" );
    double lat = fgGetDouble( "/sim/presets/latitude-deg" );
    double alt = fgGetDouble( "/sim/presets/altitude-ft" );
    double ground = get_Runway_altitude_m();
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
    FGProps2NetCtrls( &ctrls, true, true );
    if ( data_client.send( (char *)(& ctrls), length, 0 ) != length ) {
	SG_LOG( SG_IO, SG_DEBUG, "Error writing data." );
    } else {
	SG_LOG( SG_IO, SG_DEBUG, "wrote control data." );
    }

    // Read next set of FDM data (blocking enabled to maintain 'sync')
    length = sizeof(fdm);
    while ( (result = data_server.recv( (char *)(& fdm), length, 0)) >= 0 ) {
	SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
	FGNetFDM2Props( &fdm );
    }
}
