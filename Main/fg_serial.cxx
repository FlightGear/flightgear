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


static void send_nmea_out( fgSERIAL& s ) {
    char nmea[256];
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

    char date[10];
    sprintf( date, "%02d%02d%02d", 
	     t->gmt->tm_mday, t->gmt->tm_mon+1, t->gmt->tm_year );

    // $GPRMC,HHMMSS,A,DDMM.MMM,N,DDDMM.MMM,W,XXX.X,XXX.X,DDMMYY,XXX.X,E*XX
    sprintf( nmea, "$GPRMC,%s,A,%s,%s,%s,%s,%s,000.0,E*00\n",
	     utc, lat, lon, speed, heading, date );

    FG_LOG( FG_SERIAL, FG_DEBUG, nmea << endl );
    s.write_port(nmea);
}

static void read_nmea_in( fgSERIAL& s ) {
}

static void send_fgfs_out( fgSERIAL& s ) {
}

static void read_fgfs_in( fgSERIAL& s ) {
}


// one more level of indirection ...
static void process_port( fgSERIAL& s, const fgSerialPortKind kind ) {
    if ( kind == FG_SERIAL_NMEA_OUT ) {
	send_nmea_out(s);
    } else if ( kind == FG_SERIAL_NMEA_IN ) {
	read_nmea_in(s);
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
// Revision 1.1  1998/11/16 13:57:42  curt
// Initial revision.
//
