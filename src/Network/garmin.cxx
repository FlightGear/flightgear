// garmin.cxx -- Garmin protocal class
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


#include <Debug/logstream.hxx>
#include <FDM/flight.hxx>
#include <Math/fg_geodesy.hxx>
#include <Time/fg_time.hxx>

#include "iochannel.hxx"
#include "garmin.hxx"


FGGarmin::FGGarmin() {
}

FGGarmin::~FGGarmin() {
}


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


// generate Garmin message
bool FGGarmin::gen_message() {
    // cout << "generating garmin message" << endl;

    char rmc[256], rmc_sum[256], rmz[256], rmz_sum[256];
    char dir;
    int deg;
    double min;

    FGTime *t = FGTime::cur_time_params;

    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
	     t->getGmt()->tm_hour, t->getGmt()->tm_min, t->getGmt()->tm_sec );

    char lat[20];
    double latd = cur_fdm_state->get_Latitude() * RAD_TO_DEG;
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
    double lond = cur_fdm_state->get_Longitude() * RAD_TO_DEG;
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
    sprintf( speed, "%05.1f", cur_fdm_state->get_V_equiv_kts() );

    char heading[10];
    sprintf( heading, "%05.1f", cur_fdm_state->get_Psi() * RAD_TO_DEG );

    char altitude_m[10];
    sprintf( altitude_m, "%02d", 
	     (int)(cur_fdm_state->get_Altitude() * FEET_TO_METER) );

    char altitude_ft[10];
    sprintf( altitude_ft, "%02d", (int)cur_fdm_state->get_Altitude() );

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

    FG_LOG( FG_IO, FG_DEBUG, rmc );
    FG_LOG( FG_IO, FG_DEBUG, rmz );

    string garmin_sentence;

    // RMC sentence
    garmin_sentence = "$";
    garmin_sentence += rmc;
    garmin_sentence += "*";
    garmin_sentence += rmc_sum;
    garmin_sentence += "\n";

    // RMZ sentence (garmin proprietary)
    garmin_sentence += "$";
    garmin_sentence += rmz;
    garmin_sentence += "*";
    garmin_sentence += rmz_sum;
    garmin_sentence += "\n";

    cout << garmin_sentence;

    length = garmin_sentence.length();
    strncpy( buf, garmin_sentence.c_str(), length );

    return true;
}


// parse Garmin message
bool FGGarmin::parse_message() {
    FG_LOG( FG_IO, FG_INFO, "parse garmin message" );

    string msg = buf;
    msg = msg.substr( 0, length );
    FG_LOG( FG_IO, FG_INFO, "entire message = " << msg );

    string::size_type begin_line, end_line, begin, end;
    begin_line = begin = 0;

    // extract out each line
    end_line = msg.find("\n", begin_line);
    while ( end_line != string::npos ) {
	string line = msg.substr(begin_line, end_line - begin_line);
	begin_line = end_line + 1;
	FG_LOG( FG_IO, FG_INFO, "  input line = " << line );

	// leading character
	string start = msg.substr(begin, 1);
	++begin;
	FG_LOG( FG_IO, FG_INFO, "  start = " << start );

	// sentence
	end = msg.find(",", begin);
	if ( end == string::npos ) {
	    return false;
	}
    
	string sentence = msg.substr(begin, end - begin);
	begin = end + 1;
	FG_LOG( FG_IO, FG_INFO, "  sentence = " << sentence );

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
	    FG_LOG( FG_IO, FG_INFO, "  utc = " << utc );

	    // junk
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string junk = msg.substr(begin, end - begin);
	    begin = end + 1;
	    FG_LOG( FG_IO, FG_INFO, "  junk = " << junk );

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

	    cur_fdm_state->set_Latitude( lat * DEG_TO_RAD );
	    FG_LOG( FG_IO, FG_INFO, "  lat = " << lat );

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

	    cur_fdm_state->set_Longitude( lon * DEG_TO_RAD );
	    FG_LOG( FG_IO, FG_INFO, "  lon = " << lon );

	    double sl_radius, lat_geoc;
	    fgGeodToGeoc( cur_fdm_state->get_Latitude(), 
			  cur_fdm_state->get_Altitude(), 
			  &sl_radius, &lat_geoc );
	    cur_fdm_state->set_Geocentric_Position( lat_geoc, 
			   cur_fdm_state->get_Longitude(), 
	     	           sl_radius + cur_fdm_state->get_Altitude() );

	    // speed
	    end = msg.find(",", begin);
	    if ( end == string::npos ) {
		return false;
	    }
    
	    string speed_str = msg.substr(begin, end - begin);
	    begin = end + 1;
	    speed = atof( speed_str.c_str() );
	    cur_fdm_state->set_V_equiv_kts( speed );
	    cur_fdm_state->set_V_ground_speed( speed );
	    FG_LOG( FG_IO, FG_INFO, "  speed = " << speed );

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
					     heading * DEG_TO_RAD );
	    FG_LOG( FG_IO, FG_INFO, "  heading = " << heading );
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
		altitude *= METER_TO_FEET;
	    }

	    cur_fdm_state->set_Altitude( altitude );
    
 	    FG_LOG( FG_IO, FG_INFO, " altitude  = " << altitude );

	}

	// printf("%.8f %.8f\n", lon, lat);

	begin = begin_line;
	end_line = msg.find("\n", begin_line);
    }

    return true;
}


// open hailing frequencies
bool FGGarmin::open() {
    if ( is_enabled() ) {
	FG_LOG( FG_IO, FG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    FGIOChannel *io = get_io_channel();

    if ( ! io->open( get_direction() ) ) {
	FG_LOG( FG_IO, FG_ALERT, "Error opening channel communication layer." );
	return false;
    }

    set_enabled( true );

    return true;
}


// process work for this port
bool FGGarmin::process() {
    FGIOChannel *io = get_io_channel();

    if ( get_direction() == out ) {
	gen_message();
	if ( ! io->write( buf, length ) ) {
	    FG_LOG( FG_IO, FG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == in ) {
	if ( length = io->readline( buf, FG_MAX_MSG_SIZE ) ) {
	    FG_LOG( FG_IO, FG_ALERT, "Success reading data." );
	    if ( parse_message() ) {
		FG_LOG( FG_IO, FG_ALERT, "Success parsing data." );
	    } else {
		FG_LOG( FG_IO, FG_ALERT, "Error parsing data." );
	    }
	} else {
	    FG_LOG( FG_IO, FG_ALERT, "Error reading data." );
	    return false;
	}
	if ( length = io->readline( buf, FG_MAX_MSG_SIZE ) ) {
	    FG_LOG( FG_IO, FG_ALERT, "Success reading data." );
	    if ( parse_message() ) {
		FG_LOG( FG_IO, FG_ALERT, "Success parsing data." );
	    } else {
		FG_LOG( FG_IO, FG_ALERT, "Error parsing data." );
	    }
	} else {
	    FG_LOG( FG_IO, FG_ALERT, "Error reading data." );
	    return false;
	}
    }

    return true;
}


// close the channel
bool FGGarmin::close() {
    FGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
