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


#include <Include/compiler.h>

// #ifdef FG_HAVE_STD_INCLUDES
// #  include <cmath>
// #  include <cstdlib>    // atoi()
// #else
// #  include <math.h>
// #  include <stdlib.h>   // atoi()
// #endif

#include STL_STRING
// #include STL_IOSTREAM                                           
// #include <vector>                                                               

#include <Debug/logstream.hxx>
// #include <Aircraft/aircraft.hxx>
// #include <Include/fg_constants.h>
#include <Include/fg_types.hxx>
#include <Main/options.hxx>

#include <Network/iochannel.hxx>
#include <Network/fg_file.hxx>
#include <Network/fg_serial.hxx>

#include <Network/protocol.hxx>
#include <Network/garmin.hxx>
#include <Network/nmea.hxx>
#include <Network/pve.hxx>
#include <Network/rul.hxx>

// #include <Time/fg_time.hxx>
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
    if ( protocol == "garmin" ) {
	FGGarmin *garmin = new FGGarmin;
	io = garmin;
    } else if ( protocol == "nmea" ) {
	FGNMEA *nmea = new FGNMEA;
	io = nmea;
    } else if ( protocol == "pve" ) {
	FGPVE *pve = new FGPVE;
	io = pve;
    } else if ( protocol == "rul" ) {
	FGRUL *rul = new FGRUL;
	io = rul;
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
	FGSerial *ch = new FGSerial;
	io->set_io_channel( ch );

	// device name
	end = config.find(",", begin);
	if ( end == string::npos ) {
	    return NULL;
	}
    
	ch->set_device( config.substr(begin, end - begin) );
	begin = end + 1;
	FG_LOG( FG_IO, FG_INFO, "  device = " << ch->get_device() );

	// baud
	ch->set_baud( config.substr(begin) );
	FG_LOG( FG_IO, FG_INFO, "  baud = " << ch->get_baud() );

	io->set_io_channel( ch );
    } else if ( medium == "file" ) {
	FGFile *ch = new FGFile;

	// file name
	ch->set_file_name( config.substr(begin) );
	FG_LOG( FG_IO, FG_INFO, "  file name = " << ch->get_file_name() );

	io->set_io_channel( ch );
    } else if ( medium == "socket" ) {
	// ch = new FGSocket;
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


static void send_fgfs_out( FGIOChannel *p ) {
}

static void read_fgfs_in( FGIOChannel *p ) {
}


// "RUL" output format (for some sort of motion platform)
//
// The Baud rate is 2400 , one start bit, eight data bits, 1 stop bit,
// no parity.
//
// For position it requires a 3-byte data packet defined as follows:
//
// First bite: ascII character "P" ( 0x50 or 80 decimal )
// Second byte X pos. (1-255) 1 being 0* and 255 being 359*
// Third byte Y pos.( 1-255) 1 being 0* and 255 359*
//
// So sending 80 127 127 to the two axis motors will position on 180*
// The RS- 232 port is a nine pin connector and the only pins used are
// 3&5.

static void send_rul_out( FGIOChannel *p ) {
#if 0
    char rul[256];

    FGInterface *f;
    FGTime *t;

    f = current_aircraft.fdm_state;
    t = FGTime::cur_time_params;

    // run as often as possibleonce per second

    // this runs once per second
    // if ( p->last_time == t->get_cur_time() ) {
    //    return;
    // }
    // p->last_time = t->get_cur_time();
    // if ( t->get_cur_time() % 2 != 0 ) {
    //    return;
    // }
    
    // get roll and pitch, convert to degrees
    double roll_deg = f->get_Phi() * RAD_TO_DEG;
    while ( roll_deg < -180.0 ) {
	roll_deg += 360.0;
    }
    while ( roll_deg > 180.0 ) {
	roll_deg -= 360.0;
    }

    double pitch_deg = f->get_Theta() * RAD_TO_DEG;
    while ( pitch_deg < -180.0 ) {
	pitch_deg += 360.0;
    }
    while ( pitch_deg > 180.0 ) {
	pitch_deg -= 360.0;
    }

    // scale roll and pitch to output format (1 - 255)
    // straight && level == (128, 128)

    int roll = (int)( (roll_deg+180.0) * 255.0 / 360.0) + 1;
    int pitch = (int)( (pitch_deg+180.0) * 255.0 / 360.0) + 1;

    sprintf( rul, "p%c%c\n", roll, pitch);

    FG_LOG( FG_IO, FG_INFO, "p " << roll << " " << pitch );

    string rul_sentence = rul;
    p->port.write_port(rul_sentence);
#endif
}


// "PVE" (ProVision Entertainment) output format (for some sort of
// motion platform)
//
// Outputs a 5-byte data packet defined as follows:
//
// First bite:  ASCII character "P" ( 0x50 or 80 decimal )
// Second byte:  "roll" value (1-255) 1 being 0* and 255 being 359*
// Third byte:  "pitch" value (1-255) 1 being 0* and 255 being 359*
// Fourth byte:  "heave" value (or vertical acceleration?)
//
// So sending 80 127 127 to the two axis motors will position on 180*
// The RS- 232 port is a nine pin connector and the only pins used are
// 3&5.

static void send_pve_out( FGIOChannel *p ) {
#if 0
    char pve[256];

    FGInterface *f;
    FGTime *t;


    f = current_aircraft.fdm_state;
    t = FGTime::cur_time_params;

    // run as often as possibleonce per second

    // this runs once per second
    // if ( p->last_time == t->get_cur_time() ) {
    //    return;
    // }
    // p->last_time = t->get_cur_time();
    // if ( t->get_cur_time() % 2 != 0 ) {
    //    return;
    // }
    
    // get roll and pitch, convert to degrees
    double roll_deg = f->get_Phi() * RAD_TO_DEG;
    while ( roll_deg <= -180.0 ) {
	roll_deg += 360.0;
    }
    while ( roll_deg > 180.0 ) {
	roll_deg -= 360.0;
    }

    double pitch_deg = f->get_Theta() * RAD_TO_DEG;
    while ( pitch_deg <= -180.0 ) {
	pitch_deg += 360.0;
    }
    while ( pitch_deg > 180.0 ) {
	pitch_deg -= 360.0;
    }

    short int heave = (int)(f->get_W_body() * 128.0);

    // scale roll and pitch to output format (1 - 255)
    // straight && level == (128, 128)

    short int roll = (int)(roll_deg * 32768 / 180.0);
    short int pitch = (int)(pitch_deg * 32768 / 180.0);

    unsigned char roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2;
    roll_b1 = roll >> 8;
    roll_b2 = roll & 0x00ff;
    pitch_b1 = pitch >> 8;
    pitch_b2 = pitch & 0x00ff;
    heave_b1 = heave >> 8;
    heave_b2 = heave & 0x00ff;

    sprintf( pve, "p%c%c%c%c%c%c\n", 
	     roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2 );

    // printf( "p [ %u %u ]  [ %u %u ]  [ %u %u ]\n", 
    //         roll_b1, roll_b2, pitch_b1, pitch_b2, heave_b1, heave_b2 );

    FG_LOG( FG_IO, FG_INFO, "roll=" << roll << " pitch=" << pitch <<
	    " heave=" << heave );

    string pve_sentence = pve;
    p->port.write_port(pve_sentence);
#endif
}


// one more level of indirection ...
static void process_port( FGIOChannel *p ) {
#if 0
    if ( p->kind == fgIOCHANNEL::FG_SERIAL_NMEA_OUT ) {
	send_nmea_out(p);
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_NMEA_IN ) {
	read_nmea_in(p);
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_GARMIN_OUT ) {
	send_garmin_out(p);
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_GARMIN_IN ) {
	read_garmin_in(p);
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_FGFS_OUT ) {
	send_fgfs_out(p);
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_FGFS_IN ) {
	read_fgfs_in(p);
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_RUL_OUT ) {
	send_rul_out(p);
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_PVE_OUT ) {
	send_pve_out(p);
    }
#endif
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
	    if ( p->get_count_down() < 0 ) {
		p->process();
		p->set_count_down( 1000000.0 / p->get_hz() );
	    }
	}
    }
}
