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

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Serial/serial.hxx>
#include <Time/fg_time.hxx>

#include "options.hxx"

#include "fg_serial.hxx"


// support up to four serial channels.  Each channel can be assigned
// to an arbitrary port.  Bi-directional communication is supported by
// the underlying layer.

// define the four channels
fgSERIAL port_a;
fgSERIAL port_b;
fgSERIAL port_c;
fgSERIAL port_d;

// the type of each channel
fgSerialPortKind port_a_kind = FG_SERIAL_DISABLED;
fgSerialPortKind port_b_kind = FG_SERIAL_DISABLED;
fgSerialPortKind port_c_kind = FG_SERIAL_DISABLED;
fgSerialPortKind port_d_kind = FG_SERIAL_DISABLED;


// configure a port based on the config string
static bool config_port(fgSERIAL& s, fgSerialPortKind& kind,
				    const string& config)
{
    string::size_type begin, end;

    string device;
    string format;
    string baud;
    string direction;

    begin = 0;;

    // device name
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return false;
    }
    
    device = config.substr(begin, end - begin);
    begin = end + 1;
    cout << "  device = " << device << endl;

    // format
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return false;
    }
    
    format = config.substr(begin, end - begin);
    begin = end + 1;
    cout << "  format = " << format << endl;

    // baud
    end = config.find(",", begin);
    if ( end == string::npos ) {
	return false;
    }
    
    baud = config.substr(begin, end - begin);
    begin = end + 1;
    cout << "  baud = " << baud << endl;

    // direction
    direction = config.substr(begin);
    cout << "  direction = " << direction << endl;

    if ( s.is_enabled() ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "This shouldn't happen, but the port " 
		<< "is already in use, ignoring" );
	return false;
    }

    if ( ! s.open_port( device ) ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Error opening device: " << device );
    }

    if ( ! s.set_baud( atoi( baud.c_str() ) ) ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Error setting baud: " << baud );
    }

    if ( format == "nmea" ) {
	if ( direction == "out" ) {
	    kind = FG_SERIAL_NMEA_OUT;
	} else if ( direction == "in" ) {
	    kind = FG_SERIAL_NMEA_IN;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	    return false;
	}
    } else if ( format == "garman" ) {
	if ( direction == "out" ) {
	    kind = FG_SERIAL_GARMAN_OUT;
	} else if ( direction == "in" ) {
	    kind = FG_SERIAL_GARMAN_IN;
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT, "Unknown direction" );
	    return false;
	}
    } else if ( format == "fgfs" ) {
	if ( direction == "out" ) {
	    kind = FG_SERIAL_FGFS_OUT;
	} else if ( direction == "in" ) {
	    kind = FG_SERIAL_FGFS_IN;
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


// initialize serial ports based on command line options (if any)
void fgSerialInit() {
    if ( current_options.get_port_a_config() != "" ) {
	config_port(port_a, port_a_kind, current_options.get_port_a_config() );
    }

    if ( current_options.get_port_b_config() != "" ) {
	config_port(port_b, port_b_kind, current_options.get_port_b_config() );
    }

    if ( current_options.get_port_c_config() != "" ) {
	config_port(port_c, port_c_kind, current_options.get_port_c_config() );
    }

    if ( current_options.get_port_d_config() != "" ) {
	config_port(port_d, port_d_kind, current_options.get_port_d_config() );
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


static void send_nmea_out( fgSERIAL& s ) {
    char rmc[256], gga[256];
    char rmc_sum[10], gga_sum[10];
    char dir;
    int deg;
    double min;
    fgFLIGHT *f;
    fgTIME *t;

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
	s.write_port(rmc_sentence);
    } else {
	// gga on odd seconds
	string gga_sentence = "$";
	gga_sentence += gga;
	gga_sentence += "*";
	gga_sentence += gga_sum;
	gga_sentence += "\n";
	// s.write_port(gga_sentence);
    }
}

static void read_nmea_in( fgSERIAL& s ) {
}

static void send_garman_out( fgSERIAL& s ) {
    char rmc[256], rmz[256];
    char dir;
    int deg;
    double min;
    fgFLIGHT *f;
    fgTIME *t;

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
	s.write_port(rmc);
    } else {
	// gga on odd seconds
	s.write_port(rmz);
    }
}

static void read_garman_in( fgSERIAL& s ) {
}

static void send_fgfs_out( fgSERIAL& s ) {
}

static void read_fgfs_in( fgSERIAL& s ) {
}


// one more level of indirection ...
static void process_port( fgSERIAL& s, const fgSerialPortKind kind ) {
    static long last_time;
    if ( kind == FG_SERIAL_NMEA_OUT ) {
	if (cur_time_params.cur_time > last_time ) {
	    send_nmea_out(s);
	} 
	last_time = cur_time_params.cur_time;
    } else if ( kind == FG_SERIAL_NMEA_IN ) {
	read_nmea_in(s);
    } else if ( kind == FG_SERIAL_GARMAN_OUT ) {
	if (cur_time_params.cur_time > last_time ) {
	    send_garman_out(s);
	} 
	last_time = cur_time_params.cur_time;
    } else if ( kind == FG_SERIAL_GARMAN_IN ) {
	read_garman_in(s);
    } else if ( kind == FG_SERIAL_FGFS_OUT ) {
	send_fgfs_out(s);
    } else if ( kind == FG_SERIAL_FGFS_IN ) {
	read_fgfs_in(s);
    }
}


// process any serial port work
void fgSerialProcess() {
    if ( port_a_kind != FG_SERIAL_DISABLED ) {
	process_port(port_a, port_a_kind);
    }

    if ( port_b_kind != FG_SERIAL_DISABLED ) {
	process_port(port_b, port_b_kind);
    }

    if ( port_c_kind != FG_SERIAL_DISABLED ) {
	process_port(port_c, port_c_kind);
    }

    if ( port_d_kind != FG_SERIAL_DISABLED ) {
	process_port(port_d, port_d_kind);
    }
}


// $Log$
// Revision 1.3  1998/11/23 20:51:51  curt
// Tweaking serial stuff.
//
// Revision 1.2  1998/11/19 13:53:25  curt
// Added a "Garman" mode.
//
// Revision 1.1  1998/11/16 13:57:42  curt
// Initial revision.
//
