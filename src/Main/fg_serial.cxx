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


#include <Include/compiler.h>

#ifdef FG_HAVE_STD_INCLUDES
#  include <cmath>
#  include <cstdlib>    // atoi()
#else
#  include <math.h>
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
    } else if ( p.format == "rul" ) {
	if ( p.direction == "out" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_RUL_OUT;
	} else if ( p.direction == "in" ) {
	    FG_LOG( FG_SERIAL, FG_ALERT, "RUL format is outgoing only" );
	    return false;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	    return false;
	}
    } else if ( p.format == "pve" ) {
	if ( p.direction == "out" ) {
	    p.kind = fgIOCHANNEL::FG_SERIAL_PVE_OUT;
	} else if ( p.direction == "in" ) {
	    FG_LOG( FG_SERIAL, FG_ALERT, 
		    "ProVision Entertainment format is outgoing only" );
	    return false;
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
    FGTime *t;

    f = current_aircraft.fdm_state;
    t = FGTime::cur_time_params;

    // run once every two seconds
    if ( p->last_time == t->get_cur_time() ) {
	return;
    }
    p->last_time = t->get_cur_time();
    if ( t->get_cur_time() % 2 != 0 ) {
	return;
    }

    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
	     t->getGmt()->tm_hour, t->getGmt()->tm_min, t->getGmt()->tm_sec );

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
    sprintf( date, "%02d%02d%02d", t->getGmt()->tm_mday, 
	     t->getGmt()->tm_mon+1, t->getGmt()->tm_year );

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
    FGTime *t;

    f = current_aircraft.fdm_state;
    t = FGTime::cur_time_params;

    // run once per second
    if ( p->last_time == t->get_cur_time() ) {
	return;
    }
    p->last_time = t->get_cur_time();
    if ( t->get_cur_time() % 2 != 0 ) {
	return;
    }
    
    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
	     t->getGmt()->tm_hour, t->getGmt()->tm_min, t->getGmt()->tm_sec );

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
    sprintf( date, "%02d%02d%02d", t->getGmt()->tm_mday, 
	     t->getGmt()->tm_mon+1, t->getGmt()->tm_year );

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

static void send_rul_out( fgIOCHANNEL *p ) {
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

    FG_LOG( FG_SERIAL, FG_INFO, "p " << roll << " " << pitch );

    string rul_sentence = rul;
    p->port.write_port(rul_sentence);
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

static void send_pve_out( fgIOCHANNEL *p ) {
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
    int roll_deg = (int)(f->get_Phi() * RAD_TO_DEG);
    while ( roll_deg < -180 ) {
	roll_deg += 360;
    }
    while ( roll_deg > 179 ) {
	roll_deg -= 360;
    }

    int pitch_deg = (int)(f->get_Theta() * RAD_TO_DEG);
    while ( pitch_deg < -180 ) {
	pitch_deg += 360;
    }
    while ( pitch_deg > 179 ) {
	pitch_deg -= 360;
    }

    int heave = (int)(f->get_W_body()) + 128;

    // scale roll and pitch to output format (1 - 255)
    // straight && level == (128, 128)

    int roll = (int)( (roll_deg+180.0) * 255.0 / 360.0) + 1;
    int pitch = (int)( (pitch_deg+180.0) * 255.0 / 360.0) + 1;

    sprintf( pve, "p%c%c%c\n", roll, pitch, heave);

    FG_LOG( FG_SERIAL, FG_INFO, "roll=" << roll << " pitch=" << pitch <<
	    " heave=" << heave );

    string pve_sentence = pve;
    p->port.write_port(pve_sentence);
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
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_RUL_OUT ) {
	send_rul_out(p);
    } else if ( p->kind == fgIOCHANNEL::FG_SERIAL_PVE_OUT ) {
	send_pve_out(p);
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
