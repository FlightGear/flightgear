// nmea.cxx -- NMEA protocal class
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include <FDM/flightProperties.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "nmea.hxx"

FGNMEA::FGNMEA() {
  fdm = new FlightProperties();
}

FGNMEA::~FGNMEA() {
  delete fdm;
}


// calculate the nmea check sum
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


// generate NMEA message
bool FGNMEA::gen_message() {
    // cout << "generating nmea message" << endl;

    char rmc[256], gga[256], gsa[256];
    char rmc_sum[10], gga_sum[10];
    char dir;
    int deg;
    double min;

    SGTime *t = globals->get_time_params();

    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
	     t->getGmt()->tm_hour, t->getGmt()->tm_min, t->getGmt()->tm_sec );

    char gga_lat[20], rmc_lat[20];
    double latd = fdm->get_Latitude() * SGD_RADIANS_TO_DEGREES;
    if ( latd < 0.0 ) {
	latd = -latd;
	dir = 'S';
    } else {
	dir = 'N';
    }
    deg = (int)(latd);
    min = (latd - (double)deg) * 60.0;
    sprintf( gga_lat, "%02d%07.4f,%c", abs(deg), min, dir);
    sprintf( rmc_lat, "%02d%07.4f,%c", abs(deg), min, dir);

    char gga_lon[20], rmc_lon[20];
    double lond = fdm->get_Longitude() * SGD_RADIANS_TO_DEGREES;
    if ( lond < 0.0 ) {
	lond = -lond;
	dir = 'W';
    } else {
	dir = 'E';
    }
    deg = (int)(lond);
    min = (lond - (double)deg) * 60.0;
    sprintf( gga_lon, "%03d%07.4f,%c", abs(deg), min, dir);
    sprintf( rmc_lon, "%03d%07.4f,%c", abs(deg), min, dir);

    double vn = fgGetDouble( "/velocities/speed-north-fps" );
    double ve = fgGetDouble( "/velocities/speed-east-fps" );
    double fps = sqrt( vn*vn + ve*ve );
    double mps = fps * SG_FEET_TO_METER;
    double kts = mps * SG_METER_TO_NM * 3600;
    char speed[10];
    sprintf( speed, "%.1f", kts );

    double hdg_true = atan2( ve, vn ) * SGD_RADIANS_TO_DEGREES;
    if ( hdg_true < 0 ) {
      hdg_true += 360.0;
    }
    char heading[10];
    sprintf( heading, "%.1f", hdg_true );

    char altitude_m[10];
    sprintf( altitude_m, "%.1f", 
	     fdm->get_Altitude() * SG_FEET_TO_METER );

    char date[10];
    int year = t->getGmt()->tm_year;
    while ( year >= 100 ) { year -= 100; }
    sprintf( date, "%02d%02d%02d", t->getGmt()->tm_mday, 
	     t->getGmt()->tm_mon+1, year );

    char magvar[10];
    float magdeg = fgGetDouble( "/environment/magnetic-variation-deg" );
    if ( magdeg < 0.0 ) {
	magdeg = -magdeg;
	dir = 'W';
    } else {
	dir = 'E';
    }
    sprintf( magvar, "%.1f,%c", magdeg, dir );
 
    // $GPRMC,HHMMSS,A,DDMM.MMMM,N,DDDMM.MMMM,W,XXX.X,XXX.X,DDMMYY,XXX.X,E,A*XX
    sprintf( rmc, "GPRMC,%s,A,%s,%s,%s,%s,%s,%s,A",
	     utc, rmc_lat, rmc_lon, speed, heading, date, magvar );
    sprintf( rmc_sum, "%02X", calc_nmea_cksum(rmc) );

    // $GPGGA,HHMMSS,DDMM.MMMM,N,DDDMM.MMMM,W,1,NN,H.H,AAAA.A,M,GG.G,M,,*XX
    sprintf( gga, "GPGGA,%s,%s,%s,1,08,0.9,%s,M,0.0,M,,",
	     utc, gga_lat, gga_lon, altitude_m );
    sprintf( gga_sum, "%02X", calc_nmea_cksum(gga) );
    sprintf( gsa, "%s",
             "$GPGSA,A,3,01,02,03,,05,,07,,09,,11,12,0.9,0.9,2.0*38" );

    SG_LOG( SG_IO, SG_DEBUG, rmc );
    SG_LOG( SG_IO, SG_DEBUG, gga );
    SG_LOG( SG_IO, SG_DEBUG, gsa );

    string nmea_sentence;

    // RMC sentence
    nmea_sentence = "$";
    nmea_sentence += rmc;
    nmea_sentence += "*";
    nmea_sentence += rmc_sum;
    nmea_sentence += "\n";

    // GGA sentence
    nmea_sentence += "$";
    nmea_sentence += gga;
    nmea_sentence += "*";
    nmea_sentence += gga_sum;
    nmea_sentence += "\n";

    // GSA sentence (totally faked)
    nmea_sentence += gsa;
    nmea_sentence += "\n";

    // cout << nmea_sentence;

    length = nmea_sentence.length();
    strncpy( buf, nmea_sentence.c_str(), length );

    return true;
}


// parse NMEA message.  messages will look something like the
// following:
//
// $GPRMC,163227,A,3321.173,N,11039.855,W,000.1,270.0,171199,0.000,E*61
// $GPGGA,163227,3321.173,N,11039.855,W,1,,,3333,F,,,,*0F

bool FGNMEA::parse_message() {
    SG_LOG( SG_IO, SG_INFO, "parse nmea message" );

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

	    fdm->set_Latitude( lat * SGD_DEGREES_TO_RADIANS );
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

	    fdm->set_Longitude( lon * SGD_DEGREES_TO_RADIANS );
	    SG_LOG( SG_IO, SG_INFO, "  lon = " << lon );

#if 0
	    double sl_radius, lat_geoc;
	    sgGeodToGeoc( fdm->get_Latitude(), 
			  fdm->get_Altitude(), 
			  &sl_radius, &lat_geoc );
	    fdm->set_Geocentric_Position( lat_geoc, 
			   fdm->get_Longitude(), 
	     	           sl_radius + fdm->get_Altitude() );
#endif

	    // speed
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string speed_str = msg.substr(begin, end - begin);
	    begin = end + 1;
	    speed = atof( speed_str.c_str() );
	    fdm->set_V_calibrated_kts( speed );
	    // fdm->set_V_ground_speed( speed );
	    SG_LOG( SG_IO, SG_INFO, "  speed = " << speed );

	    // heading
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string hdg_str = msg.substr(begin, end - begin);
	    begin = end + 1;
	    heading = atof( hdg_str.c_str() );
	    fdm->set_Euler_Angles( fdm->get_Phi(), 
					     fdm->get_Theta(), 
					     heading * SGD_DEGREES_TO_RADIANS );
	    SG_LOG( SG_IO, SG_INFO, "  heading = " << heading );
	} else if ( sentence == "GPGGA" ) {
	    // time
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string utc = msg.substr(begin, end - begin);
	    begin = end + 1;
	    SG_LOG( SG_IO, SG_INFO, "  utc = " << utc );

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

	    // fdm->set_Latitude( lat * SGD_DEGREES_TO_RADIANS );
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

	    // fdm->set_Longitude( lon * SGD_DEGREES_TO_RADIANS );
	    SG_LOG( SG_IO, SG_INFO, "  lon = " << lon );

	    // junk
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string junk = msg.substr(begin, end - begin);
	    begin = end + 1;
	    SG_LOG( SG_IO, SG_INFO, "  junk = " << junk );

	    // junk
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    junk = msg.substr(begin, end - begin);
	    begin = end + 1;
	    SG_LOG( SG_IO, SG_INFO, "  junk = " << junk );

	    // junk
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    junk = msg.substr(begin, end - begin);
	    begin = end + 1;
	    SG_LOG( SG_IO, SG_INFO, "  junk = " << junk );

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

	    if ( alt_units != "F" ) {
		altitude *= SG_METER_TO_FEET;
	    }

	    fdm->set_Altitude( altitude );
    
 	    SG_LOG( SG_IO, SG_INFO, " altitude  = " << altitude );

	}

	// printf("%.8f %.8f\n", lon, lat);

	begin = begin_line;
	end_line = msg.find("\n", begin_line);
    }

    return true;
}


// open hailing frequencies
bool FGNMEA::open() {
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
bool FGNMEA::process() {
    SGIOChannel *io = get_io_channel();

    if ( get_direction() == SG_IO_OUT ) {
	gen_message();
	if ( ! io->write( buf, length ) ) {
	    SG_LOG( SG_IO, SG_WARN, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( (length = io->readline( buf, FG_MAX_MSG_SIZE )) > 0 ) {
	    parse_message();
	} else {
	    SG_LOG( SG_IO, SG_WARN, "Error reading data." );
	    return false;
	}
	if ( (length = io->readline( buf, FG_MAX_MSG_SIZE )) > 0 ) {
	    parse_message();
	} else {
	    SG_LOG( SG_IO, SG_WARN, "Error reading data." );
	    return false;
	}
    }

    return true;
}


// close the channel
bool FGNMEA::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
