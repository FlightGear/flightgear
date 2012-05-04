// AV400.cxx -- Garmin 400 series protocal class
//
// Written by Curtis Olson, started August 2006.
//
// Copyright (C) 2006  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include <cstdlib>
#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "AV400.hxx"

FGAV400::FGAV400() {
}

FGAV400::~FGAV400() {
}


#if 0
// calculate the garmin check sum
static char calc_nmea_cksum(char *sentence) {
    unsigned char sum = 0;
    int i, len;

    // cout << sentence << endl;

    len = strlen(sentence);
    sum = sentence[0];
    for ( i = 1; i < len; i++ ) {
        // cout << sentence[i];
        sum ^= sentence[i];
    }
    // cout << endl;

    // printf("sum = %02x\n", sum);
    return sum;
}
#endif


// generate AV400 message
bool FGAV400::gen_message() {
    // cout << "generating garmin message" << endl;

    char msg_z[32], msg_A[32], msg_B[32], msg_C[32], msg_D[32];
    char msg_Q[32], msg_T[32], msg_type2[256];
    // the following could be implemented, but currently are unused
    // char msg_E[32], msg_G[32], msg_I[32], msg_K[32], msg_L[32], msg_S[32];
    // char msg_l[32];

    char dir;
    int deg;
    double min;

    // create msg_z
    sprintf( msg_z, "z%05.0f\r\n", fdm.get_Altitude() );

    // create msg_A
    sprintf( msg_A, "A");

    double latd = fdm.get_Latitude() * SGD_RADIANS_TO_DEGREES;
    if ( latd < 0.0 ) {
	latd = -latd;
	dir = 'S';
    } else {
	dir = 'N';
    }
    deg = (int)latd;
    min = (latd - (double)deg) * 60.0 * 100.0;
    sprintf( msg_A, "A%c %02d %04.0f\r\n", dir, deg, min);

    // create msg_B
    double lond = fdm.get_Longitude() * SGD_RADIANS_TO_DEGREES;
    if ( lond < 0.0 ) {
	lond = -lond;
	dir = 'W';
    } else {
	dir = 'E';
    }
    deg = (int)lond;
    min = (lond - (double)deg) * 60.0 * 100.0;
    sprintf( msg_B, "B%c %03d %04.0f\r\n", dir, deg, min);

    // create msg_C
    float magdeg = fgGetDouble( "/environment/magnetic-variation-deg" );
    double vn = fgGetDouble( "/velocities/speed-north-fps" );
    double ve = fgGetDouble( "/velocities/speed-east-fps" );
    double gnd_trk_true = atan2( ve, vn ) * SGD_RADIANS_TO_DEGREES;
    double gnd_trk_mag = gnd_trk_true - magdeg;
    if ( gnd_trk_mag < 0.0 ) { gnd_trk_mag += 360.0; }
    if ( gnd_trk_mag >= 360.0 ) { gnd_trk_mag -= 360.0; }
    sprintf( msg_C, "C%03.0f\r\n", gnd_trk_mag);

    // create msg_D
    double speed_kt = sqrt( vn*vn + ve*ve ) * SG_FPS_TO_KT;
    if ( speed_kt > 999.0 ) {
        speed_kt = 999.0;
    }
    sprintf( msg_D, "D%03.0f\r\n", speed_kt);

    // create msg_E (not implemented)
    // create msg_G (not implemented)
    // create msg_I (not implemented)
    // create msg_K (not implemented)
    // create msg_L (not implemented)

    // create msg_Q
    if ( magdeg < 0.0 ) {
        magdeg = -magdeg;
        dir = 'W';
    } else {
        dir = 'E';
    }
    sprintf( msg_Q, "Q%c%03.0f\r\n", dir, magdeg * 10.0 );

    // create msg_S (not implemented)

    // create msg_T
    sprintf( msg_T, "T---------\r\n" );

    // create msg_l (not implemented)

    // sentence type 2
    sprintf( msg_type2, "w01%c\r\n", (char)65 );

    // assemble message
    string sentence;
    sentence += '\002';         // STX
    sentence += msg_z;          // altitude
    sentence += msg_A;          // latitude
    sentence += msg_B;          // longitude
    sentence += msg_C;          // ground track
    sentence += msg_D;          // ground speed (kt)
    // sentence += "E-----\r\n";
    // sentence += "G-----\r\n";
    // sentence += "I----\r\n";
    // sentence += "K-----\r\n";
    // sentence += "L----\r\n";
    sentence += msg_Q;          // magvar
    // sentence += "S-----\r\n";
    sentence += msg_T;          // end of type 1 messages (must be sent)
    sentence += msg_type2;      // type2 message
    // sentence += "l------\r\n";
    sentence += '\003';         // ETX

    // cout << sentence;
    length = sentence.length();
    // cout << endl << "length = " << length << endl;
    strncpy( buf, sentence.c_str(), length );

    return true;
}


// parse AV400 message
bool FGAV400::parse_message() {
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

	    fdm.set_Latitude( lat * SGD_DEGREES_TO_RADIANS );
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

	    fdm.set_Longitude( lon * SGD_DEGREES_TO_RADIANS );
	    SG_LOG( SG_IO, SG_INFO, "  lon = " << lon );

#if 0
	    double sl_radius, lat_geoc;
	    sgGeodToGeoc( fdm.get_Latitude(), 
			  fdm.get_Altitude(), 
			  &sl_radius, &lat_geoc );
	    fdm.set_Geocentric_Position( lat_geoc, 
			   fdm.get_Longitude(), 
	     	           sl_radius + fdm.get_Altitude() );
#endif

	    // speed
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string speed_str = msg.substr(begin, end - begin);
	    begin = end + 1;
	    speed = atof( speed_str.c_str() );
	    fdm.set_V_calibrated_kts( speed );
	    // fdm.set_V_ground_speed( speed );
	    SG_LOG( SG_IO, SG_INFO, "  speed = " << speed );

	    // heading
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string hdg_str = msg.substr(begin, end - begin);
	    begin = end + 1;
	    heading = atof( hdg_str.c_str() );
	    fdm.set_Euler_Angles( fdm.get_Phi(), 
					     fdm.get_Theta(), 
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

	    fdm.set_Altitude( altitude );
    
 	    SG_LOG( SG_IO, SG_INFO, " altitude  = " << altitude );

	}

	// printf("%.8f %.8f\n", lon, lat);

	begin = begin_line;
	end_line = msg.find("\n", begin_line);
    }

    return true;
}


// open hailing frequencies
bool FGAV400::open() {
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
bool FGAV400::process() {
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
bool FGAV400::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
