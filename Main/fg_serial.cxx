// fg_serial.cxx -- higher level serial port management routines
//
// Written by Curtis Olson, started November 1998.
//
// Copyright (C) 1998  Curtis L. Olson - curt@flightgear.org
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
// (Log is kept at end of this file)


#include <stdlib.h>   // atoi()

#include <string>
#include <vector>                                                               
#include "Include/fg_stl_config.h"                                              

#ifdef NEEDNAMESPACESTD                                                         
using namespace std;                                                            
#endif                                                                          

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Serial/serial.hxx>
#include <Time/fg_time.hxx>

#include "options.hxx"

#include "fg_serial.hxx"


// support an arbitrary number of serial channels.  Each channel can
// be assigned to an arbitrary port.  Bi-directional communication is
// supported by the underlying layer, but probably will never be
// needed by FGFS?

typedef vector < fgIOCHANNEL > io_container;
typedef io_container::iterator io_iterator;
typedef io_container::const_iterator const_io_iterator;

// define the four channels
io_container port_list;


fgIOCHANNEL::fgIOCHANNEL() :
    kind( FG_SERIAL_DISABLED )
{
}


fgIOCHANNEL::~fgIOCHANNEL() {
}


// configure a port based on the config string
static fgIOCHANNEL config_port( const string& config )
{
    fgIOCHANNEL p;

    string::size_type begin, end;

    string device;
    string format;
    string baud;
    string direction;

    begin = 0;

    FG_LOG( FG_SERIAL, FG_INFO, "Configuring serial port: " << config );

    // device name
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return p;
    }
    
    device = config.substr(begin, end - begin);
    begin = end + 1;
    FG_LOG( FG_SERIAL, FG_INFO, "  device = " << device );

    // format
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return p;
    }
    
    format = config.substr(begin, end - begin);
    begin = end + 1;
    FG_LOG( FG_SERIAL, FG_INFO, "  format = " << format );

    // baud
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return p;
    }
    
    baud = config.substr(begin, end - begin);
    begin = end + 1;
    FG_LOG( FG_SERIAL, FG_INFO, "  baud = " << baud );

    // direction
    direction = config.substr(begin);
    FG_LOG( FG_SERIAL, FG_INFO, "  direction = " << direction );

    if ( p.port.is_enabled() ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "This shouldn't happen, but the port " 
		<< "is already in use, ignoring" );
	return p;
    }

    if ( ! p.port.open_port( device ) ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Error opening device: " << device );
	return p;
    }

    if ( ! p.port.set_baud( atoi( baud.c_str() ) ) ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Error setting baud: " << baud );
	return p;
    }

    if ( format == "nmea" ) {
	if ( direction == "out" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_NMEA_OUT;
	} else if ( direction == "in" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_NMEA_IN;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	}
    } else if ( format == "garman" ) {
	if ( direction == "out" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_GARMAN_OUT;
	} else if ( direction == "in" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_GARMAN_IN;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	}
    } else if ( format == "fgfs" ) {
	if ( direction == "out" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_FGFS_OUT;
	} else if ( direction == "in" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_FGFS_IN;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	}
    } else {
	FG_LOG( FG_SERIAL, FG_ALERT, "Unknown format" );
    }

    return p;
}


// step through the port config streams (from fgOPTIONS) and setup
// serial port channels for each
void fgSerialInit() {
    fgIOCHANNEL port;
    str_container port_options_list = current_options.get_port_options_list();

    const_str_iterator current = port_options_list.begin();
    const_str_iterator last = port_options_list.end();

    for ( ; current != last; ++current ) {
	port = config_port( *current );
	if ( port.kind != fgIOCHANNEL::FG_SERIAL_DISABLED ) {
	    port_list.push_back( port );
	}
    }
}


char calc_nmea_cksum(char *sentence) {
    unsigned char sum = 0;
    int i, len;

    // printf("%s\n", sentence);

    len = strlen(sentence);
    sum = sentence[0];
    for ( i = 1; i < len; i++ ) {
        // printf("%c", sentence[i]);
        sum ^= sentence[i];
    }
    // printf("\n");

    // printf("sum = %02x\n", sum);
    return sum;
}


static void send_nmea_out( fgIOCHANNEL& p ) {
    char rmc[256], gga[256];
    char rmc_sum[10], gga_sum[10];
    char dir;
    int deg;
    double min;
    fgFLIGHT *f;
    fgTIME *t;

    // run once per second
    if ( p.last_time == cur_time_params.cur_time ) {
	return;
    }
    p.last_time = cur_time_params.cur_time;
    
    f = current_aircraft.flight;
    t = &cur_time_params;

    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
	     t->gmt->tm_hour, t->gmt->tm_min, t->gmt->tm_sec );

    char lat[20];
    double latd = FG_Latitude * RAD_TO_DEG;
    if ( latd < 0.0 ) {
	latd *= -1.0;
	dir = 'S';
    } else {
	dir = 'N';
    }
    deg = (int)(latd);
    min = (latd - (double)deg) * 60.0;
    sprintf( lat, "%02d%06.3f,%c", abs(deg), min, dir);

    char lon[20];
    double lond = FG_Longitude * RAD_TO_DEG;
    if ( lond < 0.0 ) {
	lond *= -1.0;
	dir = 'W';
    } else {
	dir = 'E';
    }
    deg = (int)(lond);
    min = (lond - (double)deg) * 60.0;
    sprintf( lon, "%02d%06.3f,%c", abs(deg), min, dir);

    char speed[10];
    sprintf( speed, "%05.1f", FG_V_equiv_kts );

    char heading[10];
    sprintf( heading, "%05.1f", FG_Psi * RAD_TO_DEG );

    char altitude_m[10];
    sprintf( altitude_m, "%02d", (int)(FG_Altitude * FEET_TO_METER) );

    char altitude_ft[10];
    sprintf( altitude_ft, "%02d", (int)FG_Altitude );

    char date[10];
    sprintf( date, "%02d%02d%02d", 
	     t->gmt->tm_mday, t->gmt->tm_mon+1, t->gmt->tm_year );

    // $GPRMC,HHMMSS,A,DDMM.MMM,N,DDDMM.MMM,W,XXX.X,XXX.X,DDMMYY,XXX.X,E*XX
    sprintf( rmc, "GPRMC,%s,A,%s,%s,%s,%s,%s,0.000,E",
	     utc, lat, lon, speed, heading, date );
    sprintf( rmc_sum, "%02X", 0 /*calc_nmea_cksum(rmc)*/ );

    sprintf( gga, "GPGGA,%s,%s,%s,1,,,%s,M,,,,",
	     utc, lat, lon, altitude_m );
    sprintf( gga_sum, "%02X", 0 /*calc_nmea_cksum(gga)*/ );


    FG_LOG( FG_SERIAL, FG_DEBUG, rmc );
    FG_LOG( FG_SERIAL, FG_DEBUG, gga );

    // one full frame every 2 seconds according to the standard
    if ( cur_time_params.cur_time % 2 == 0 ) {
	// rmc on even seconds
	string rmc_sentence = "$";
	rmc_sentence += rmc;
	rmc_sentence += "*";
	rmc_sentence += rmc_sum;
	rmc_sentence += "\r\n";
	p.port.write_port(rmc_sentence);
    } else {
	// gga on odd seconds
	string gga_sentence = "$";
	gga_sentence += gga;
	gga_sentence += "*";
	gga_sentence += gga_sum;
	gga_sentence += "\n";
	// p.port.write_port(gga_sentence);
    }
}

static void read_nmea_in( fgIOCHANNEL& p ) {
}

static void send_garman_out( fgIOCHANNEL& p ) {
    char rmc[256], rmz[256];
    char dir;
    int deg;
    double min;
    fgFLIGHT *f;
    fgTIME *t;

    // run once per second
    if ( p.last_time == cur_time_params.cur_time ) {
	return;
    }
    p.last_time = cur_time_params.cur_time;
    
    f = current_aircraft.flight;
    t = &cur_time_params;

    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
	     t->gmt->tm_hour, t->gmt->tm_min, t->gmt->tm_sec );

    char lat[20];
    double latd = FG_Latitude * RAD_TO_DEG;
    if ( latd < 0.0 ) {
	latd *= -1.0;
	dir = 'S';
    } else {
	dir = 'N';
    }
    deg = (int)(latd);
    min = (latd - (double)deg) * 60.0;
    sprintf( lat, "%02d%06.3f,%c", abs(deg), min, dir);

    char lon[20];
    double lond = FG_Longitude * RAD_TO_DEG;
    if ( lond < 0.0 ) {
	lond *= -1.0;
	dir = 'W';
    } else {
	dir = 'E';
    }
    deg = (int)(lond);
    min = (lond - (double)deg) * 60.0;
    sprintf( lon, "%02d%06.3f,%c", abs(deg), min, dir);

    char speed[10];
    sprintf( speed, "%05.1f", FG_V_equiv_kts );

    char heading[10];
    sprintf( heading, "%05.1f", FG_Psi * RAD_TO_DEG );

    char altitude_m[10];
    sprintf( altitude_m, "%02d", (int)(FG_Altitude * FEET_TO_METER) );

    char altitude_ft[10];
    sprintf( altitude_ft, "%02d", (int)FG_Altitude );

    char date[10];
    sprintf( date, "%02d%02d%02d", 
	     t->gmt->tm_mday, t->gmt->tm_mon+1, t->gmt->tm_year );

    // $GPRMC,HHMMSS,A,DDMM.MMM,N,DDDMM.MMM,W,XXX.X,XXX.X,DDMMYY,XXX.X,E*XX
    sprintf( rmc, "$GPRMC,%s,A,%s,%s,%s,%s,%s,000.0,E*00\r\n",
	     utc, lat, lon, speed, heading, date );

    // sprintf( gga, "$GPGGA,%s,%s,%s,1,04,0.0,%s,M,00.0,M,,*00\r\n",
    //          utc, lat, lon, altitude_m );

    sprintf( rmz, "$PGRMZ,%s,f,3*00\r\n", altitude_ft );

    FG_LOG( FG_SERIAL, FG_DEBUG, rmc );
    FG_LOG( FG_SERIAL, FG_DEBUG, rmz );


    // one full frame every 2 seconds according to the standard
    if ( cur_time_params.cur_time % 2 == 0 ) {
	// rmc on even seconds
	p.port.write_port(rmc);
    } else {
	// rmz on odd seconds
	p.port.write_port(rmz);
    }
}

static void read_garman_in( fgIOCHANNEL& p ) {
}

static void send_fgfs_out( fgIOCHANNEL& p ) {
}

static void read_fgfs_in( fgIOCHANNEL& p ) {
}


// one more level of indirection ...
static void process_port( fgIOCHANNEL& p ) {
    if ( p.kind == fgIOCHANNEL::FG_SERIAL_NMEA_OUT ) {
	send_nmea_out(p);
    } else if ( p.kind == fgIOCHANNEL::FG_SERIAL_NMEA_IN ) {
	read_nmea_in(p);
    } else if ( p.kind == fgIOCHANNEL::FG_SERIAL_GARMAN_OUT ) {
	send_garman_out(p);
    } else if ( p.kind == fgIOCHANNEL::FG_SERIAL_GARMAN_IN ) {
	read_garman_in(p);
    } else if ( p.kind == fgIOCHANNEL::FG_SERIAL_FGFS_OUT ) {
	send_fgfs_out(p);
    } else if ( p.kind == fgIOCHANNEL::FG_SERIAL_FGFS_IN ) {
	read_fgfs_in(p);
    }
}


// process any serial port work
void fgSerialProcess() {
    fgIOCHANNEL port;
    
    const_io_iterator current = port_list.begin();
    const_io_iterator last = port_list.end();

    for ( ; current != last; ++current ) {
	port = *current;
	if ( port.kind != fgIOCHANNEL::FG_SERIAL_DISABLED ) {
	    process_port ( port );
	}
    }
}


// $Log$
// Revision 1.4  1998/11/25 01:33:58  curt
// Support for an arbitrary number of serial ports.
//
// Revision 1.3  1998/11/23 20:51:51  curt
// Tweaking serial stuff.
//
// Revision 1.2  1998/11/19 13:53:25  curt
// Added a "Garman" mode.
//
// Revision 1.1  1998/11/16 13:57:42  curt
// Initial revision.
//
