// fg_io.cxx -- higher level I/O channel management routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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


#include <simgear/compiler.h>

#include <stdlib.h>             // atoi()

#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/io/sg_serial.hxx>
#include <simgear/io/sg_socket.hxx>
#include <simgear/io/sg_socket_udp.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/misc/strutils.hxx>

#include <Network/protocol.hxx>
#include <Network/atc610x.hxx>
#include <Network/atlas.hxx>
#include <Network/garmin.hxx>
#include <Network/httpd.hxx>
#ifdef FG_JPEG_SERVER
#  include <Network/jpg-httpd.hxx>
#endif
#include <Network/joyclient.hxx>
#include <Network/native.hxx>
#include <Network/native_ctrls.hxx>
#include <Network/native_fdm.hxx>
#include <Network/opengc.hxx>
#include <Network/nmea.hxx>
#include <Network/props.hxx>
#include <Network/telnet.hxx>
#include <Network/pve.hxx>
#include <Network/ray.hxx>
#include <Network/rul.hxx>

#include "globals.hxx"

SG_USING_NAMESPACE(std);
SG_USING_STD(string);


// define the global I/O channel list
io_container global_io_list;


// configure a port based on the config string
static FGProtocol *parse_port_config( const string& config )
{
    SG_LOG( SG_IO, SG_INFO, "Parse I/O channel request: " << config );

    vector<string> tokens = simgear::strutils::split( config, "," );
    if (tokens.empty())
    {
	SG_LOG( SG_IO, SG_ALERT,
		"Port configuration error: empty config string" );
	return 0;
    }

    string protocol = tokens[0];
    SG_LOG( SG_IO, SG_INFO, "  protocol = " << protocol );

    FGProtocol *io = 0;

    try
    {
	if ( protocol == "atc610x" ) {
	    return new FGATC610x;
	} else if ( protocol == "atlas" ) {
	    FGAtlas *atlas = new FGAtlas;
	    io = atlas;
	} else if ( protocol == "opengc" ) {
	    // char wait;
	    // printf("Parsed opengc\n"); cin >> wait;
	    FGOpenGC *opengc = new FGOpenGC;
	    io = opengc;
	} else if ( protocol == "garmin" ) {
	    FGGarmin *garmin = new FGGarmin;
	    io = garmin;
	} else if ( protocol == "httpd" ) {
	    // determine port
	    string port = tokens[1];
	    return new FGHttpd( atoi(port.c_str()) );
#ifdef FG_JPEG_SERVER
	} else if ( protocol == "jpg-httpd" ) {
	    // determine port
	    string port = tokens[1];
	    return new FGJpegHttpd( atoi(port.c_str()) );
#endif
	} else if ( protocol == "joyclient" ) {
	    FGJoyClient *joyclient = new FGJoyClient;
	    io = joyclient;
	} else if ( protocol == "native" ) {
	    FGNative *native = new FGNative;
	    io = native;
	} else if ( protocol == "native_ctrls" ) {
	    FGNativeCtrls *native_ctrls = new FGNativeCtrls;
	    io = native_ctrls;
	} else if ( protocol == "native_fdm" ) {
	    FGNativeFDM *native_fdm = new FGNativeFDM;
	    io = native_fdm;
	} else if ( protocol == "nmea" ) {
	    FGNMEA *nmea = new FGNMEA;
	    io = nmea;
	} else if ( protocol == "props" ) {
	    io = new FGProps();
	} else if ( protocol == "telnet" ) {
	    io = new FGTelnet( tokens );
	    return io;
	} else if ( protocol == "pve" ) {
	    FGPVE *pve = new FGPVE;
	    io = pve;
	} else if ( protocol == "ray" ) {
	    FGRAY *ray = new FGRAY;
	    io = ray;
	} else if ( protocol == "rul" ) {
	    FGRUL *rul = new FGRUL;
	    io = rul;
	} else {
	    return NULL;
	}
    }
    catch (FGProtocolConfigError& err)
    {
	SG_LOG( SG_IO, SG_ALERT, "Port configuration error: " << err.what() );
	delete io;
	return 0;
    }

    string medium = tokens[1];
    SG_LOG( SG_IO, SG_INFO, "  medium = " << medium );

    string direction = tokens[2];
    io->set_direction( direction );
    SG_LOG( SG_IO, SG_INFO, "  direction = " << direction );

    string hertz_str = tokens[3];
    double hertz = atof( hertz_str.c_str() );
    io->set_hz( hertz );
    SG_LOG( SG_IO, SG_INFO, "  hertz = " << hertz );

    if ( medium == "serial" ) {
	// device name
	string device = tokens[4];
	SG_LOG( SG_IO, SG_INFO, "  device = " << device );

	// baud
	string baud = tokens[5];
	SG_LOG( SG_IO, SG_INFO, "  baud = " << baud );

	SGSerial *ch = new SGSerial( device, baud );
	io->set_io_channel( ch );
    } else if ( medium == "file" ) {
	// file name
	string file = tokens[4];
	SG_LOG( SG_IO, SG_INFO, "  file name = " << file );

	SGFile *ch = new SGFile( file );
	io->set_io_channel( ch );
    } else if ( medium == "socket" ) {
	string hostname = tokens[4];
	string port = tokens[5];
	string style = tokens[6];

	SG_LOG( SG_IO, SG_INFO, "  hostname = " << hostname );
	SG_LOG( SG_IO, SG_INFO, "  port = " << port );
	SG_LOG( SG_IO, SG_INFO, "  style = " << style );
            
	io->set_io_channel( new SGSocket( hostname, port, style ) );
    }

    return io;
}


// step through the port config streams (from fgOPTIONS) and setup
// serial port channels for each
void fgIOInit() {
    // SG_LOG( SG_IO, SG_INFO, "I/O Channel initialization, " << 
    //         globals->get_channel_options_list()->size() << " requests." );

    FGProtocol *p;
    string_list *channel_options_list = globals->get_channel_options_list();

    // we could almost do this in a single step except pushing a valid
    // port onto the port list copies the structure and destroys the
    // original, which closes the port and frees up the fd ... doh!!!

    // parse the configuration strings and store the results in the
    // appropriate FGIOChannel structures
    for ( int i = 0; i < (int)channel_options_list->size(); ++i ) {
	p = parse_port_config( (*channel_options_list)[i] );
	if ( p != NULL ) {
	    p->open();
	    global_io_list.push_back( p );
	    if ( !p->is_enabled() ) {
		SG_LOG( SG_IO, SG_ALERT, "I/O Channel config failed." );
		exit(-1);
	    }
	} else {
	    SG_LOG( SG_IO, SG_INFO, "I/O Channel parse failed." );
	}
    }
}


// process any serial port work
void fgIOProcess() {
    // cout << "processing I/O channels" << endl;

    static int inited = 0;
    int interval;
    static SGTimeStamp last;
    SGTimeStamp current;

    if ( ! inited ) {
	inited = 1;
	last.stamp();
	interval = 0;
    } else {
        current.stamp();
	interval = current - last;
	last = current;
    }

    for ( unsigned int i = 0; i < global_io_list.size(); ++i ) {
	// cout << "  channel = " << i << endl;
	FGProtocol* p = global_io_list[i];

	if ( p->is_enabled() ) {
	    p->dec_count_down( interval );
	    while ( p->get_count_down() < 0 ) {
		p->process();
		p->dec_count_down(int( -1000000.0 / p->get_hz()));
	    }
	}
    }
}


// shutdown all I/O connections
void fgIOShutdownAll() {
    FGProtocol *p;

    // cout << "processing I/O channels" << endl;

    for ( int i = 0; i < (int)global_io_list.size(); ++i ) {
	// cout << "  channel = " << i << endl;
	p = global_io_list[i];

	if ( p->is_enabled() ) {
	    p->close();
	}
    }
}
