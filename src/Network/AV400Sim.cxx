// AV400Sim.cxx -- Garmin 400 series protocal class.  This AV400Sim
// protocol generates the set of "simulator" commands a garmin 400
// series gps would expect as input in simulator mode.  The AV400
// protocol generates the set of commands that a garmin 400 series gps
// would emit.
//
// Written by Curtis Olson, started Janauary 2009.
//
// Copyright (C) 2009  Curtis L. Olson - http://www.flightgear.org/~curt
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
#  include "config.h"
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#include <FDM/flight.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "AV400Sim.hxx"

FGAV400Sim::FGAV400Sim() {
}

FGAV400Sim::~FGAV400Sim() {
}


// generate AV400Sim message
bool FGAV400Sim::gen_message() {
    // cout << "generating garmin message" << endl;

    char msg_a[32], msg_b[32], msg_c[32], msg_d[32], msg_e[32];
    char msg_f[32], msg_i[32], msg_j[32], msg_k[32], msg_l[32], msg_r[32];
    char msg_type2[256];

    char dir;
    int deg;
    double min;

    // create msg_a
    double latd = cur_fdm_state->get_Latitude() * SGD_RADIANS_TO_DEGREES;
    if ( latd < 0.0 ) {
	latd = -latd;
	dir = 'S';
    } else {
	dir = 'N';
    }
    deg = (int)latd;
    min = (latd - (double)deg) * 60.0 * 100.0;
    sprintf( msg_a, "a%c %03d %04.0f\r\n", dir, deg, min);

    // create msg_b
    double lond = cur_fdm_state->get_Longitude() * SGD_RADIANS_TO_DEGREES;
    if ( lond < 0.0 ) {
	lond = -lond;
	dir = 'W';
    } else {
	dir = 'E';
    }
    deg = (int)lond;
    min = (lond - (double)deg) * 60.0 * 100.0;
    sprintf( msg_b, "b%c %03d %04.0f\r\n", dir, deg, min);

    // create msg_c
    double alt = cur_fdm_state->get_Altitude();
    if ( alt > 99999.0 ) { alt = 99999.0; }
    sprintf( msg_c, "c%05.0f\r\n", alt );

    // create msg_d
    double ve_kts = fgGetDouble( "/velocities/speed-east-fps" ) * SG_FPS_TO_KT;
    if ( ve_kts < 0.0 ) {
	ve_kts = -ve_kts;
	dir = 'W';
    } else {
	dir = 'E';
    }
    if ( ve_kts > 999.0 ) { ve_kts = 999.0; }
    sprintf( msg_d, "d%c%03.0f\r\n", dir, ve_kts );

    // create msg_e
    double vn_kts = fgGetDouble( "/velocities/speed-north-fps" ) * SG_FPS_TO_KT;
    if ( vn_kts < 0.0 ) {
	vn_kts = -vn_kts;
	dir = 'S';
    } else {
	dir = 'N';
    }
    if ( vn_kts > 999.0 ) { vn_kts = 999.0; }
    sprintf( msg_e, "e%c%03.0f\r\n", dir, vn_kts );

    // create msg_f
    double climb_fpm = fgGetDouble( "/velocities/vertical-speed-fps" ) * 60;
    if ( climb_fpm < 0.0 ) {
	climb_fpm = -climb_fpm;
	dir = 'D';
    } else {
	dir = 'U';
    }
    if ( climb_fpm > 9999.0 ) { climb_fpm = 9999.0; }
    sprintf( msg_f, "f%c%04.0f\r\n", dir, climb_fpm );

    // create msg_i
    double fuel = fgGetDouble( "/consumables/fuel/total-fuel-gals" );
    if ( fuel > 999.9 ) { fuel = 999.9; }
    sprintf( msg_i, "i%04.0f\r\n", fuel*10.0 );

    // create msg_j
    double gph = fgGetDouble( "/engines/engine[0]/fuel-flow-gph" );
    gph += fgGetDouble( "/engines/engine[1]/fuel-flow-gph" );
    gph += fgGetDouble( "/engines/engine[2]/fuel-flow-gph" );
    gph += fgGetDouble( "/engines/engine[3]/fuel-flow-gph" );
    if ( gph > 999.9 ) { gph = 999.9; }
    sprintf( msg_j, "j%04.0f\r\n", gph*10.0 );

    // create msg_k
    sprintf( msg_k, "k%04d%02d%02d%02d%02d%02d\r\n",
	     fgGetInt( "/sim/time/utc/year"),
	     fgGetInt( "/sim/time/utc/month"),
	     fgGetInt( "/sim/time/utc/day"),
	     fgGetInt( "/sim/time/utc/hour"),
	     fgGetInt( "/sim/time/utc/minute"),
	     fgGetInt( "/sim/time/utc/second") );

    // create msg_l
    alt = fgGetDouble( "/instrumentation/pressure-alt-ft" );
    if ( alt > 99999.0 ) { alt = 99999.0; }
    sprintf( msg_l, "l%05.0f\r\n", alt );

    // create msg_r
    sprintf( msg_r, "rA\r\n" );

    // sentence type 2
    sprintf( msg_type2, "w01%c\r\n", (char)65 );

    // assemble message
    string sentence;
    sentence += '\002';         // STX
    sentence += msg_a;          // latitude
    sentence += msg_b;          // longitude
    sentence += msg_c;          // gps altitude
    sentence += msg_d;          // ve kts
    sentence += msg_e;          // vn kts
    sentence += msg_f;		// climb fpm
    sentence += msg_i;		// total fuel in gal (*10)
    sentence += msg_j;		// fuel flow gph (*10)
    sentence += msg_k;		// date/time (UTC)
    sentence += msg_l;		// pressure altitude
    sentence += msg_r;		// RAIM available
    sentence += msg_type2;      // type2 message
    sentence += '\003';         // ETX

    // cout << sentence;
    length = sentence.length();
    // cout << endl << "length = " << length << endl;
    strncpy( buf, sentence.c_str(), length );

    return true;
}


// parse AV400Sim message
bool FGAV400Sim::parse_message() {
    SG_LOG( SG_IO, SG_INFO, "parse garmin message" );

    string msg = buf;
    msg = msg.substr( 0, length );
    SG_LOG( SG_IO, SG_INFO, "entire message = " << msg );

    string::size_type begin_line, end_line, begin, end;
    begin_line = begin = 0;

    // extract out each line
    end_line = msg.find("\n", begin_line);
    while ( end_line != string::npos ) {
	string line = msg.substr(begin_line, end_line - begin_line);
	begin_line = end_line + 1;
	SG_LOG( SG_IO, SG_INFO, "  input line = " << line );

	// leading character
	string start = msg.substr(begin, 1);
	++begin;
	SG_LOG( SG_IO, SG_INFO, "  start = " << start );

	// sentence
	end = msg.find(",", begin);
	if ( end == string::npos ) {
	    return false;
	}
    
	string sentence = msg.substr(begin, end - begin);
	begin = end + 1;
	SG_LOG( SG_IO, SG_INFO, "  sentence = " << sentence );

	double lon_deg, lon_min, lat_deg, lat_min;
	double lon, lat, speed, heading, altitude;

	if ( sentence == "GPRMC" ) {
	    // time
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string utc = msg.substr(begin, end - begin);
	    begin = end + 1;
	    SG_LOG( SG_IO, SG_INFO, "  utc = " << utc );

	    // junk
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string junk = msg.substr(begin, end - begin);
	    begin = end + 1;
	    SG_LOG( SG_IO, SG_INFO, "  junk = " << junk );

	    // lat val
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string lat_str = msg.substr(begin, end - begin);
	    begin = end + 1;

	    lat_deg = atof( lat_str.substr(0, 2).c_str() );
	    lat_min = atof( lat_str.substr(2).c_str() );

	    // lat dir
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string lat_dir = msg.substr(begin, end - begin);
	    begin = end + 1;

	    lat = lat_deg + ( lat_min / 60.0 );
	    if ( lat_dir == "S" ) {
		lat *= -1;
	    }

	    cur_fdm_state->set_Latitude( lat * SGD_DEGREES_TO_RADIANS );
	    SG_LOG( SG_IO, SG_INFO, "  lat = " << lat );

	    // lon val
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string lon_str = msg.substr(begin, end - begin);
	    begin = end + 1;

	    lon_deg = atof( lon_str.substr(0, 3).c_str() );
	    lon_min = atof( lon_str.substr(3).c_str() );

	    // lon dir
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string lon_dir = msg.substr(begin, end - begin);
	    begin = end + 1;

	    lon = lon_deg + ( lon_min / 60.0 );
	    if ( lon_dir == "W" ) {
		lon *= -1;
	    }

	    cur_fdm_state->set_Longitude( lon * SGD_DEGREES_TO_RADIANS );
	    SG_LOG( SG_IO, SG_INFO, "  lon = " << lon );

#if 0
	    double sl_radius, lat_geoc;
	    sgGeodToGeoc( cur_fdm_state->get_Latitude(), 
			  cur_fdm_state->get_Altitude(), 
			  &sl_radius, &lat_geoc );
	    cur_fdm_state->set_Geocentric_Position( lat_geoc, 
			   cur_fdm_state->get_Longitude(), 
	     	           sl_radius + cur_fdm_state->get_Altitude() );
#endif

	    // speed
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string speed_str = msg.substr(begin, end - begin);
	    begin = end + 1;
	    speed = atof( speed_str.c_str() );
	    cur_fdm_state->set_V_calibrated_kts( speed );
	    // cur_fdm_state->set_V_ground_speed( speed );
	    SG_LOG( SG_IO, SG_INFO, "  speed = " << speed );

	    // heading
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string hdg_str = msg.substr(begin, end - begin);
	    begin = end + 1;
	    heading = atof( hdg_str.c_str() );
	    cur_fdm_state->set_Euler_Angles( cur_fdm_state->get_Phi(), 
					     cur_fdm_state->get_Theta(), 
					     heading * SGD_DEGREES_TO_RADIANS );
	    SG_LOG( SG_IO, SG_INFO, "  heading = " << heading );
	} else if ( sentence == "PGRMZ" ) {
	    // altitude
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string alt_str = msg.substr(begin, end - begin);
	    altitude = atof( alt_str.c_str() );
	    begin = end + 1;

	    // altitude units
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string alt_units = msg.substr(begin, end - begin);
	    begin = end + 1;

	    if ( alt_units != "F" && alt_units != "f" ) {
		altitude *= SG_METER_TO_FEET;
	    }

	    cur_fdm_state->set_Altitude( altitude );
    
 	    SG_LOG( SG_IO, SG_INFO, " altitude  = " << altitude );

	}

	// printf("%.8f %.8f\n", lon, lat);

	begin = begin_line;
	end_line = msg.find("\n", begin_line);
    }

    return true;
}


// open hailing frequencies
bool FGAV400Sim::open() {
    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    SGIOChannel *io = get_io_channel();

    if ( ! io->open( get_direction() ) ) {
	SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
	return false;
    }

    set_enabled( true );

    return true;
}


// process work for this port
bool FGAV400Sim::process() {
    SGIOChannel *io = get_io_channel();

    if ( get_direction() == SG_IO_OUT ) {
	gen_message();
	if ( ! io->write( buf, length ) ) {
	    SG_LOG( SG_IO, SG_WARN, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( (length = io->readline( buf, FG_MAX_MSG_SIZE )) > 0 ) {
	    SG_LOG( SG_IO, SG_ALERT, "Success reading data." );
	    if ( parse_message() ) {
		SG_LOG( SG_IO, SG_ALERT, "Success parsing data." );
	    } else {
		SG_LOG( SG_IO, SG_ALERT, "Error parsing data." );
	    }
	} else {
	    SG_LOG( SG_IO, SG_ALERT, "Error reading data." );
	    return false;
	}
	if ( (length = io->readline( buf, FG_MAX_MSG_SIZE )) > 0 ) {
	    SG_LOG( SG_IO, SG_ALERT, "Success reading data." );
	    if ( parse_message() ) {
		SG_LOG( SG_IO, SG_ALERT, "Success parsing data." );
	    } else {
		SG_LOG( SG_IO, SG_ALERT, "Error parsing data." );
	    }
	} else {
	    SG_LOG( SG_IO, SG_ALERT, "Error reading data." );
	    return false;
	}
    }

    return true;
}


// close the channel
bool FGAV400Sim::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
