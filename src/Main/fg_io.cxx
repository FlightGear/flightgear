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

#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/io/sg_serial.hxx>
#include <simgear/io/sg_socket.hxx>
#include <simgear/math/sg_types.hxx>

#include <Main/options.hxx>

#include <Network/protocol.hxx>
#include <Network/native.hxx>
#include <Network/garmin.hxx>
#include <Network/nmea.hxx>
#include <Network/props.hxx>
#include <Network/pve.hxx>
#include <Network/ray.hxx>
#include <Network/rul.hxx>
#include <Network/joyclient.hxx>

#include <Time/timestamp.hxx>

FG_USING_STD(string);


// define the global I/O channel list
io_container global_io_list;


// configure a port based on the config string
static FGProtocol *parse_port_config( const string& config )
{
    string::size_type begin, end;

    begin = 0;

    FG_LOG( FG_IO, FG_INFO, "Parse I/O channel request: " << config );

    // determine protocol
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return NULL;		// dummy
    }
    
    string protocol = config.substr(begin, end - begin);
    begin = end + 1;
    FG_LOG( FG_IO, FG_INFO, "  protocol = " << protocol );

    FGProtocol *io;
    if ( protocol == "native" ) {
	FGNative *native = new FGNative;
	io = native;
    } else if ( protocol == "garmin" ) {
	FGGarmin *garmin = new FGGarmin;
	io = garmin;
    } else if ( protocol == "nmea" ) {
	FGNMEA *nmea = new FGNMEA;
	io = nmea;
    } else if ( protocol == "props" ) {
	FGProps *props = new FGProps;
	io = props;
    } else if ( protocol == "pve" ) {
	FGPVE *pve = new FGPVE;
	io = pve;
    } else if ( protocol == "ray" ) {
	FGRAY *ray = new FGRAY;
	io = ray;
    } else if ( protocol == "rul" ) {
	FGRUL *rul = new FGRUL;
	io = rul;
    } else if ( protocol == "joyclient" ) {
	FGJoyClient *joyclient = new FGJoyClient;
	io = joyclient;
    } else {
	return NULL;
    }

    // determine medium
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return NULL;		// dummy
    }
    
    string medium = config.substr(begin, end - begin);
    begin = end + 1;
    FG_LOG( FG_IO, FG_INFO, "  medium = " << medium );

    // determine direction
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return NULL;		// dummy
    }
    
    string direction = config.substr(begin, end - begin);
    begin = end + 1;
    io->set_direction( direction );
    FG_LOG( FG_IO, FG_INFO, "  direction = " << direction );

    // determine hertz
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return NULL;		// dummy
    }
    
    string hertz_str = config.substr(begin, end - begin);
    begin = end + 1;
    double hertz = atof( hertz_str.c_str() );
    io->set_hz( hertz );
    FG_LOG( FG_IO, FG_INFO, "  hertz = " << hertz );

    if ( medium == "serial" ) {
	// device name
	end = config.find(",", begin);
	if ( end == string::npos ) {
	    return NULL;
	}
    
	string device = config.substr(begin, end - begin);
	begin = end + 1;
	FG_LOG( FG_IO, FG_INFO, "  device = " << device );

	// baud
	string baud = config.substr(begin);
	FG_LOG( FG_IO, FG_INFO, "  baud = " << baud );

	SGSerial *ch = new SGSerial( device, baud );
	io->set_io_channel( ch );
    } else if ( medium == "file" ) {
	// file name
	string file = config.substr(begin);
	FG_LOG( FG_IO, FG_INFO, "  file name = " << file );

	SGFile *ch = new SGFile( file );
	io->set_io_channel( ch );
    } else if ( medium == "socket" ) {
	// hostname
	end = config.find(",", begin);
	if ( end == string::npos ) {
	    return NULL;
	}
    
	string hostname = config.substr(begin, end - begin);
	begin = end + 1;
	FG_LOG( FG_IO, FG_INFO, "  hostname = " << hostname );

	// port string
	end = config.find(",", begin);
	if ( end == string::npos ) {
	    return NULL;
	}
    
	string port = config.substr(begin, end - begin);
	begin = end + 1;
	FG_LOG( FG_IO, FG_INFO, "  port string = " << port );

	// socket style
	string style_str = config.substr(begin);
	FG_LOG( FG_IO, FG_INFO, "  style string = " << style_str );
       
	SGSocket *ch = new SGSocket( hostname, port, style_str );
	io->set_io_channel( ch );
    }

    return io;
}


// step through the port config streams (from fgOPTIONS) and setup
// serial port channels for each
void fgIOInit() {
    FGProtocol *p;
    string_list channel_options_list = 
	current_options.get_channel_options_list();

    // we could almost do this in a single step except pushing a valid
    // port onto the port list copies the structure and destroys the
    // original, which closes the port and frees up the fd ... doh!!!

    // parse the configuration strings and store the results in the
    // appropriate FGIOChannel structures
    for ( int i = 0; i < (int)channel_options_list.size(); ++i ) {
	p = parse_port_config( channel_options_list[i] );
	if ( p != NULL ) {
	    p->open();
	    global_io_list.push_back( p );
	    if ( !p->is_enabled() ) {
		FG_LOG( FG_IO, FG_INFO, "I/O Channel config failed." );
	    }
	} else {
	    FG_LOG( FG_IO, FG_INFO, "I/O Channel parse failed." );
	}
    }
}


// process any serial port work
void fgIOProcess() {
    FGProtocol *p;

    // cout << "processing I/O channels" << endl;

    static int inited = 0;
    int interval;
    static FGTimeStamp last;
    FGTimeStamp current;

    if ( ! inited ) {
	inited = 1;
	last.stamp();
	interval = 0;
    } else {
        current.stamp();
	interval = current - last;
	last = current;
    }

    for ( int i = 0; i < (int)global_io_list.size(); ++i ) {
	// cout << "  channel = " << i << endl;
	p = global_io_list[i];

	if ( p->is_enabled() ) {
	    p->dec_count_down( interval );
	    while ( p->get_count_down() < 0 ) {
		p->process();
		p->dec_count_down( -1000000.0 / p->get_hz() );
	    }
	}
    }
}
