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


#include <Include/compiler.h>

#ifdef FG_HAVE_STD_INCLUDES
#  include <cstdlib>    // atoi()
#else
#  include <stdlib.h>   // atoi()
#endif

#include STL_STRING
#include STL_IOSTREAM                                           
#include <vector>                                                               

#include <Debug/logstream.hxx>
#include <Aircraft/aircraft.hxx>
#include <Include/fg_constants.h>
#include <Serial/serial.hxx>
#include <Time/fg_time.hxx>

#include "options.hxx"

#include "fg_serial.hxx"

FG_USING_STD(string);
FG_USING_STD(vector);

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
    kind( FG_SERIAL_DISABLED ),
    valid_config( false )
{
}


fgIOCHANNEL::~fgIOCHANNEL() {
}


// configure a port based on the config string
static fgIOCHANNEL parse_port_config( const string& config )
{
    fgIOCHANNEL p;

    string::size_type begin, end;

    begin = 0;

    FG_LOG( FG_SERIAL, FG_INFO, "Parse serial port config: " << config );

    // device name
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return p;
    }
    
    p.device = config.substr(begin, end - begin);
    begin = end + 1;
    FG_LOG( FG_SERIAL, FG_INFO, "  device = " << p.device );

    // format
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return p;
    }
    
    p.format = config.substr(begin, end - begin);
    begin = end + 1;
    FG_LOG( FG_SERIAL, FG_INFO, "  format = " << p.format );

    // baud
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return p;
    }
    
    p.baud = config.substr(begin, end - begin);
    begin = end + 1;
    FG_LOG( FG_SERIAL, FG_INFO, "  baud = " << p.baud );

    // direction
    p.direction = config.substr(begin);
    FG_LOG( FG_SERIAL, FG_INFO, "  direction = " << p.direction );

    p.valid_config = true;

    return p;
}


// configure a port based on the config info
static bool config_port( fgIOCHANNEL &p )
{
    if ( p.port.is_enabled() ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "This shouldn't happen, but the port " 
		<< "is already in use, ignoring" );
	return false;
    }

    if ( ! p.port.open_port( p.device ) ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Error opening device: " << p.device );
	return false;
    }

    // cout << "fd = " << p.port.fd << endl;

    if ( ! p.port.set_baud( atoi( p.baud.c_str() ) ) ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Error setting baud: " << p.baud );
	return false;
    }

    if ( p.format == "nmea" ) {
	if ( p.direction == "out" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_NMEA_OUT;
	} else if ( p.direction == "in" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_NMEA_IN;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	    return false;
	}
    } else if ( p.format == "garmin" ) {
	if ( p.direction == "out" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_GARMIN_OUT;
	} else if ( p.direction == "in" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_GARMIN_IN;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	    return false;
	}
    } else if ( p.format == "fgfs" ) {
	if ( p.direction == "out" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_FGFS_OUT;
	} else if ( p.direction == "in" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_FGFS_IN;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	    return false;
	}
    } else {
	FG_LOG( FG_SERIAL, FG_ALERT, "Unknown format" );
	return false;
    }

    return true;
}


// step through the port config streams (from fgOPTIONS) and setup
// serial port channels for each
void fgSerialInit() {
    fgIOCHANNEL port;
    bool result;
    str_container port_options_list = current_options.get_port_options_list();

    // we could almost do this in a single step except pushing a valid
    // port onto the port list copies the structure and destroys the
    // original, which closes the port and frees up the fd ... doh!!!

    // parse the configuration strings and store the results in stub
    // fgIOCHANNEL structures
    const_str_iterator current_str = port_options_list.begin();
    const_str_iterator last_str = port_options_list.end();
    for ( ; current_str != last_str; ++current_str ) {
	port = parse_port_config( *current_str );
	if ( port.valid_config ) {
	    result = config_port( port );
	    if ( result ) {
		port_list.push_back( port );
	    }
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


static void send_nmea_out( fgIOCHANNEL *p ) {
    char rmc[256], gga[256];
    char rmc_sum[10], gga_sum[10];
    char dir;
    int deg;
    double min;
    FGInterface *f;
    fgTIME *t;

    // run once every two seconds
    if ( p->last_time == cur_time_params.cur_time ) {
	return;
    }
    p->last_time = cur_time_params.cur_time;
    if ( cur_time_params.cur_time % 2 != 0 ) {
	return;
    }

    f = current_aircraft.fdm_state;
    t = &cur_time_params;

    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
	     t->gmt->tm_hour, t->gmt->tm_min, t->gmt->tm_sec );

    char lat[20];
    double latd = f->get_Latitude() * RAD_TO_DEG;
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
    double lond = f->get_Longitude() * RAD_TO_DEG;
    if ( lond < 0.0 ) {
	lond *= -1.0;
	dir = 'W';
    } else {
	dir = 'E';
    }
    deg = (int)(lond);
    min = (lond - (double)deg) * 60.0;
    sprintf( lon, "%03d%06.3f,%c", abs(deg), min, dir);

    char speed[10];
    sprintf( speed, "%05.1f", f->get_V_equiv_kts() );

    char heading[10];
    sprintf( heading, "%05.1f", f->get_Psi() * RAD_TO_DEG );

    char altitude_m[10];
    sprintf( altitude_m, "%02d", (int)(f->get_Altitude() * FEET_TO_METER) );

    char altitude_ft[10];
    sprintf( altitude_ft, "%02d", (int)f->get_Altitude() );

    char date[10];
    sprintf( date, "%02d%02d%02d", 
	     t->gmt->tm_mday, t->gmt->tm_mon+1, t->gmt->tm_year );

    // $GPRMC,HHMMSS,A,DDMM.MMM,N,DDDMM.MMM,W,XXX.X,XXX.X,DDMMYY,XXX.X,E*XX
    sprintf( rmc, "GPRMC,%s,A,%s,%s,%s,%s,%s,0.000,E",
	     utc, lat, lon, speed, heading, date );
    sprintf( rmc_sum, "%02X", calc_nmea_cksum(rmc) );

    sprintf( gga, "GPGGA,%s,%s,%s,1,,,%s,F,,,,",
	     utc, lat, lon, altitude_ft );
    sprintf( gga_sum, "%02X", calc_nmea_cksum(gga) );


    FG_LOG( FG_SERIAL, FG_DEBUG, rmc );
    FG_LOG( FG_SERIAL, FG_DEBUG, gga );

    // RMC sentence
    string rmc_sentence = "$";
    rmc_sentence += rmc;
    rmc_sentence += "*";
    rmc_sentence += rmc_sum;
    rmc_sentence += "\n";
    p->port.write_port(rmc_sentence);
    // cout << rmc_sentence;

    // GGA sentence
    string gga_sentence = "$";
    gga_sentence += gga;
    gga_sentence += "*";
    gga_sentence += gga_sum;
    gga_sentence += "\n";
    p->port.write_port(gga_sentence);
    // cout << gga_sentence;
}

static void read_nmea_in( fgIOCHANNEL *p ) {
}

static void send_garmin_out( fgIOCHANNEL *p ) {
    char rmc[256], rmc_sum[256], rmz[256], rmz_sum[256];
    char dir;
    int deg;
    double min;
    FGInterface *f;
    fgTIME *t;

    // run once per second
    if ( p->last_time == cur_time_params.cur_time ) {
	return;
    }
    p->last_time = cur_time_params.cur_time;
    if ( cur_time_params.cur_time % 2 != 0 ) {
	return;
    }
    
    f = current_aircraft.fdm_state;
    t = &cur_time_params;

    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
	     t->gmt->tm_hour, t->gmt->tm_min, t->gmt->tm_sec );

    char lat[20];
    double latd = f->get_Latitude() * RAD_TO_DEG;
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
    double lond = f->get_Longitude() * RAD_TO_DEG;
    if ( lond < 0.0 ) {
	lond *= -1.0;
	dir = 'W';
    } else {
	dir = 'E';
    }
    deg = (int)(lond);
    min = (lond - (double)deg) * 60.0;
    sprintf( lon, "%03d%06.3f,%c", abs(deg), min, dir);

    char speed[10];
    sprintf( speed, "%05.1f", f->get_V_equiv_kts() );

    char heading[10];
    sprintf( heading, "%05.1f", f->get_Psi() * RAD_TO_DEG );

    char altitude_m[10];
    sprintf( altitude_m, "%02d", (int)(f->get_Altitude() * FEET_TO_METER) );

    char altitude_ft[10];
    sprintf( altitude_ft, "%02d", (int)f->get_Altitude() );

    char date[10];
    sprintf( date, "%02d%02d%02d", 
	     t->gmt->tm_mday, t->gmt->tm_mon+1, t->gmt->tm_year );

    // $GPRMC,HHMMSS,A,DDMM.MMM,N,DDDMM.MMM,W,XXX.X,XXX.X,DDMMYY,XXX.X,E*XX
    sprintf( rmc, "GPRMC,%s,A,%s,%s,%s,%s,%s,000.0,E",
	     utc, lat, lon, speed, heading, date );
    sprintf( rmc_sum, "%02X", calc_nmea_cksum(rmc) );

    // sprintf( gga, "$GPGGA,%s,%s,%s,1,04,0.0,%s,M,00.0,M,,*00\r\n",
    //          utc, lat, lon, altitude_m );

    sprintf( rmz, "PGRMZ,%s,f,3", altitude_ft );
    sprintf( rmz_sum, "%02X", calc_nmea_cksum(rmz) );

    FG_LOG( FG_SERIAL, FG_DEBUG, rmc );
    FG_LOG( FG_SERIAL, FG_DEBUG, rmz );

    // RMC sentence
    string rmc_sentence = "$";
    rmc_sentence += rmc;
    rmc_sentence += "*";
    rmc_sentence += rmc_sum;
    rmc_sentence += "\n";
    p->port.write_port(rmc_sentence);
    // cout << rmc_sentence;

    // RMZ sentence (garmin proprietary)
    string rmz_sentence = "$";
    rmz_sentence += rmz;
    rmz_sentence += "*";
    rmz_sentence += rmz_sum;
    rmz_sentence += "\n";
    p->port.write_port(rmz_sentence);
    // cout << rmz_sentence;
}

static void read_garmin_in( fgIOCHANNEL *p ) {
}

static void send_fgfs_out( fgIOCHANNEL *p ) {
}

static void read_fgfs_in( fgIOCHANNEL *p ) {
}


// one more level of indirection ...
static void process_port( fgIOCHANNEL *p ) {
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
    }
}


// process any serial port work
void fgSerialProcess() {
    fgIOCHANNEL *port;
    
    io_iterator current = port_list.begin();
    io_iterator last = port_list.end();

    for ( ; current != last; ++current ) {
	port = current;
	if ( port->kind != fgIOCHANNEL::FG_SERIAL_DISABLED ) {
	    process_port ( port );
	}
    }
}


// $Log$
// Revision 1.1  1999/04/05 21:32:46  curt
// Initial revision
//
// Revision 1.13  1999/03/02 01:03:16  curt
// Tweaks for building with native SGI compilers.
//
// Revision 1.12  1999/02/26 22:09:50  curt
// Added initial support for native SGI compilers.
//
// Revision 1.11  1999/02/05 21:29:11  curt
// Modifications to incorporate Jon S. Berndts flight model code.
//
// Revision 1.10  1999/01/21 00:55:01  curt
// Fixed some problems with timing of output strings.
// Added checksum support for nmea and garmin output.
//
// Revision 1.9  1999/01/20 13:42:26  curt
// Tweaked FDM interface.
// Testing check sum support for NMEA serial output.
//
// Revision 1.8  1999/01/19 20:57:04  curt
// MacOS portability changes contributed by "Robert Puyol" <puyol@abvent.fr>
//
// Revision 1.7  1998/12/05 15:54:21  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.6  1998/12/03 01:17:18  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.5  1998/11/30 17:43:32  curt
// Lots of tweaking to get serial output to actually work.
//
// Revision 1.4  1998/11/25 01:33:58  curt
// Support for an arbitrary number of serial ports.
//
// Revision 1.3  1998/11/23 20:51:51  curt
// Tweaking serial stuff.
//
// Revision 1.2  1998/11/19 13:53:25  curt
// Added a "Garmin" mode.
//
// Revision 1.1  1998/11/16 13:57:42  curt
// Initial revision.
//
