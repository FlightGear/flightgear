// AV400WSim.cxx -- Garmin 400 series protocal class.  This AV400WSim
// protocol generates the set of "simulator" commands a garmin 400 WAAS
// series gps would expect as input in simulator mode.  The AV400W
// protocol parses the set of commands that a garmin 400W series gps
// would emit.
// 
// The Garmin WAAS GPS uses 2 serial channels to communicate with the
// simulator.  These 2 channels are represented by the FGAV400WSimA and
// the FGAV400WSimB classes.  The "A" channel is similar to the previous
// AVSim400 protocol. The "B" channel is considered the "GPS" channel and
// uses a different protocol than the "A" channel. The GPS unit expects
// input on the "B" channel at two different frequencies (1hz and 5hz,
// normally).  The "B" channel also expects responses to certain output
// messages.
//
// Original AV400Sim code Written by Curtis Olson, started Janauary 2009.
// This AV400W code written by Bruce Hellstrom, March 2011.
//
// Copyright (C) 2009  Curtis L. Olson - http://www.flightgear.org/~curt
// Copyright (c) 2011  Bruce Hellstrom - http://www.celebritycc.com
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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#include <FDM/flightProperties.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "AV400WSim.hxx"

FGAV400WSimA::FGAV400WSimA() {
}

FGAV400WSimA::~FGAV400WSimA() {
}


// generate AV400WSimA message
bool FGAV400WSimA::gen_message() {
    // cout << "generating garmin message" << endl;

    char msg_h[32], msg_i[32], msg_j[32], msg_k[32], msg_l[32];
    //char msg_type2[256];

    double alt;

    // create msg_h
    double obs = fgGetDouble( "/instrumentation/nav[0]/radials/selected-deg" );
    sprintf( msg_h, "h%04d\r\n", (int)(obs*10) );

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

    // sentence type 2
    //sprintf( msg_type2, "w01%c\r\n", (char)65 );

    // assemble message
    string sentence;
    sentence += '\002';         // STX
    sentence += msg_h;		// obs heading in deg (*10)
    sentence += msg_i;		// total fuel in gal (*10)
    sentence += msg_j;		// fuel flow gph (*10)
    sentence += msg_k;		// date/time (UTC)
    sentence += msg_l;		// pressure altitude
    //sentence += msg_type2;      // type2 message
    sentence += '\003';         // ETX

    // cout << sentence;
    length = sentence.length();
    // cout << endl << "length = " << length << endl;
    strncpy( buf, sentence.c_str(), length );

    return true;
}


// parse AV400SimA message
bool FGAV400WSimA::parse_message() {
    SG_LOG( SG_IO, SG_INFO, "parse AV400WSimA message" );

    string msg = buf;
    msg = msg.substr( 0, length );
    SG_LOG( SG_IO, SG_INFO, "entire message = " << msg );

    string ident = msg.substr(0, 1);
    if ( ident == "i" ) {
        string side = msg.substr(1,1);
        string num = msg.substr(2,3);
        if ( side == "-" ) {
            fgSetDouble("/instrumentation/av400w/cdi-deflection", 0.0);
        }
        else {
            int pos = atoi(num.c_str());
            if ( side == "L" ) {
                pos *= -1;
            }
            fgSetDouble("/instrumentation/av400w/cdi-deflection",
                        (double)pos / 10.0);
            //printf( "i, %s%s, %f\n", side.c_str(), num.c_str(), (double)(pos / 10.0) );
        }
    }
    else if ( ident == "j" ) {
        string side = msg.substr(1,1);
        string num = msg.substr(2,3);
        if ( side == "-" ) {
            fgSetDouble("/instrumentation/av400w/gs-deflection", 0.0);
        }
        else {
            int pos = atoi(num.c_str());
            if ( side == "B" ) {
                pos *= -1;
            }
            // convert glideslope to -3.5 to 3.5
            fgSetDouble("/instrumentation/av400w/gs-deflection",
                        (double)pos / 28.57);
            //printf( "j, %s%s, %f\n", side.c_str(), num.c_str(), (double)(pos / 28.57) );
        }
    }
    else if ( ident == "k" ) {
        string ind = msg.substr(1,1);
        if ( ind == "T" ) {
            fgSetBool("/instrumentation/av400w/to-flag", true);
            fgSetBool("/instrumentation/av400w/from-flag", false);
            //printf( "set to-flag\n" );
        } else if ( ind == "F" ) {
            fgSetBool("/instrumentation/av400w/to-flag", false);
            fgSetBool("/instrumentation/av400w/from-flag", true);
            //printf( "set from flag\n" );
        } else {
            fgSetBool("/instrumentation/av400w/to-flag", false);
            fgSetBool("/instrumentation/av400w/from-flag", false);
            //printf( "set t/f both false\n" );
        }
        //printf( "k, %s\n", ind.c_str() );
    }
    else if ( ident == "S" ) {
        string ind = msg.substr(1,5);
        //printf( "S - %s\n", ind.c_str() );
    }
    else {
        // SG_LOG( SG_IO, SG_ALERT, "unknown AV400Sim message = " << msg );
    }

    return true;
}


// open hailing frequencies
bool FGAV400WSimA::open() {
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
bool FGAV400WSimA::process() {
    SGIOChannel *io = get_io_channel();

    // until we have parsers/generators for the reverse direction,
    // this is hardwired to expect that the physical GPS is slaving
    // from FlightGear.

    // Send FlightGear data to the external device
    gen_message();
    if ( ! io->write( buf, length ) ) {
        SG_LOG( SG_IO, SG_WARN, "Error writing data." );
        return false;
    }

    // read the device messages back
    while ( (length = io->readline( buf, FG_MAX_MSG_SIZE )) > 0 ) {
        // SG_LOG( SG_IO, SG_ALERT, "Success reading data." );
        if ( parse_message() ) {
            // SG_LOG( SG_IO, SG_ALERT, "Success parsing data." );
        } else {
            // SG_LOG( SG_IO, SG_ALERT, "Error parsing data." );
        }
    }

    return true;
}


// close the channel
bool FGAV400WSimA::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
        return false;
    }

    return true;
}

// Start of FGAV400WSimB class methods
FGAV400WSimB::FGAV400WSimB() :
hz2(0.0),
hz2count(0),
hz2cycles(0),    
flight_phase(0xFF),
req_hostid(true),
req_raimap(false),
req_sbas(false)
{
    hal.clear();
    val.clear();
    hal.append( "\0\0", 2 );
    val.append( "\0\0", 2 );
    outputctr = 0;
    sbas_sel.append( "\0\x01", 2 );
    fdm = new FlightProperties;
}

FGAV400WSimB::~FGAV400WSimB() {
    delete fdm;
}


bool FGAV400WSimB::gen_hostid_message() {
    char chksum = 0;
    string data = "Cj\r\n";
    data += "COPYRIGHT 2008 GARMIN LTD.       \r\n";
    data += "SFTW P/N #    006-B0339-0A\r\n";
    data += "SOFTWARE VER #           3\r\n";
    data += "SOFTWARE REV #           2\r\n";
    data += "SOFTWARE DATE   11/03/2008\r\n";
    data += "SW CRC   8F5E7DD1 AE5D4563\r\n";
    data += "HDWR P/N # 012-00857-01   \r\n";
    data += "SERIAL #   085701214976140\r\n";
    data += "MANUFACTUR DATE 02/26/2007\r\n";
    data += "OPTIONS LIST    iiiiiiiiii";
    
    // calculate the checksum
    for ( string::const_iterator cli = data.begin(); cli != data.end(); cli++ ) {
        chksum ^= *cli;
    }
    
    string sentence( "@@" );
    sentence += data;
    sentence.push_back( chksum );
    sentence += "\x0D\n";
    
    length = sentence.length();
    char *bufptr = buf;
    for ( string::const_iterator cli = sentence.begin(); cli != sentence.end(); cli++ ) {
        *bufptr++ = *cli;
    }

    return true;
}

bool FGAV400WSimB::gen_sbas_message() {
    char chksum = 0;
    string data = "WA";
    data.push_back( '\0' );
    data += sbas_sel;
    
    // calculate the checksum
    for ( string::const_iterator cli = data.begin(); cli != data.end(); cli++ ) {
        chksum ^= *cli;
    }
    
    string sentence( "@@" );
    sentence += data;
    sentence.push_back( chksum );
    sentence += "\x0D\n";
    
    length = sentence.length();
    char *bufptr = buf;
    for ( string::const_iterator cli = sentence.begin(); cli != sentence.end(); cli++ ) {
        *bufptr++ = *cli;
    }

    return true;
}

// Wh - Visible SBAS Satellites (hz2)
bool FGAV400WSimB::gen_Wh_message() {
    char chksum = 0;
    
    // generate the Wh message
    string data = "Wh";
    data.push_back( '\x0F' );
    data.append( "\x3f\x00\x00\x20\x00\x20", 6 );
    data.append( "\x4f\x00\x00\x28\x00\x30", 6 );
    data.append( "\x2d\x00\x00\x48\x01\x05", 6 );
    data.append( "\x1d\x00\x00\x10\x01\x10", 6 );
    data.append( "\x50\x00\x00\x33\x00\x50", 6 );
    data.append( "\x22\x00\x00\x16\x00\x90", 6 );
    data.append( "\x40\x00\x00\x20\x00\x20", 6 );
    data.append( "\x50\x00\x00\x28\x00\x30", 6 );
    data.append( "\x2e\x00\x00\x48\x01\x05", 6 );
    data.append( "\x1e\x00\x00\x10\x01\x10", 6 );
    data.append( "\x51\x00\x00\x33\x00\x50", 6 );
    data.append( "\x23\x00\x00\x16\x00\x90", 6 );
    data.append( "\x1f\x00\x00\x10\x01\x10", 6 );
    data.append( "\x52\x00\x00\x33\x00\x50", 6 );
    data.append( "\x24\x00\x00\x16\x00\x90", 6 );
    data.push_back( '0' );
    
    // calculate the checksum
    for ( string::const_iterator cli = data.begin(); cli != data.end(); cli++ ) {
        chksum ^= *cli;
    }
    
    string sentence( "@@" );
    sentence += data;
    sentence.push_back( chksum );
    sentence += "\x0D\n";

    length = sentence.length();
    char *bufptr = buf;
    for ( string::const_iterator cli = sentence.begin(); cli != sentence.end(); cli++ ) {
        *bufptr++ = *cli;
    }

    return true;
}


// Wx - Channel Status Message (hz2)
bool FGAV400WSimB::gen_Wx_message() {
    char chksum = 0;
    
    // Now process the Wx message
    string data = "Wx";
    data.push_back( (char)( fgGetInt( "/sim/time/utc/month") & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/day") & 0xFF ) );
    data.push_back( (char)( (fgGetInt( "/sim/time/utc/year") >> 8 ) & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/year") & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/hour") & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/minute") & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/second") & 0xFF ) );
    data.append( "\x00\x00\x00\x00", 4 );
    
    for ( int xctr = 0; xctr < 15; xctr++ ) {
        data.append( "\x00\x00\x00\x00\x00\x00\x00\x00", 8 );
    }
    data.push_back( '\0' );
    
    // calculate the checksum
    for ( string::const_iterator cli = data.begin(); cli != data.end(); cli++ ) {
        chksum ^= *cli;
    }
    
    string sentence( "@@" );
    sentence += data;
    sentence.push_back( chksum );
    sentence += "\x0D\n";
    
    // cout << sentence;
    length = sentence.length();
    char *bufptr = buf;
    for ( string::const_iterator cli = sentence.begin(); cli != sentence.end(); cli++ ) {
        *bufptr++ = *cli;
    }

    return true;
}


// Wt - Position and Navigation status
bool FGAV400WSimB::gen_Wt_message() {
    char chksum = 0;
    
    // generate the Wt message
    string data = "Wt";
    data.push_back( (char)( fgGetInt( "/sim/time/utc/month") & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/day") & 0xFF ) );
    data.push_back( (char)( (fgGetInt( "/sim/time/utc/year") >> 8 ) & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/year") & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/hour") & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/minute") & 0xFF ) );
    data.push_back( (char)( fgGetInt( "/sim/time/utc/second") & 0xFF ) );
    data.append( "\x00\x00\x00\x00", 4 );
    
    // get latitude in milliarcseconds
    double latd = fdm->get_Latitude() * SGD_RADIANS_TO_DEGREES;
    latd *= DEG_TO_MILLIARCSECS;
    int latitude = (int)latd;
    data.push_back( (char)( ( latitude >> 24 ) & 0xFF ) );
    data.push_back( (char)( ( latitude >> 16 ) & 0xFF ) );
    data.push_back( (char)( ( latitude >> 8 ) & 0xFF ) );
    data.push_back( (char)( latitude & 0xFF ) );
    
    // get longitude in milliarcseconds
    double lond = fdm->get_Longitude() * SGD_RADIANS_TO_DEGREES;
    lond *= DEG_TO_MILLIARCSECS;
    int longitude = (int)lond;
    data.push_back( (char)( ( longitude >> 24 ) & 0xFF ) );
    data.push_back( (char)( ( longitude >> 16 ) & 0xFF ) );
    data.push_back( (char)( ( longitude >> 8 ) & 0xFF ) );
    data.push_back( (char)( longitude & 0xFF ) );
    
   
    // Altitude settings
    double alt = fdm->get_Altitude();
    if ( alt > 99999.0 ) { alt = 99999.0; }
    
    // send the  WGS-84 ellipsoid height om /-1, (just use regular altitude)
    alt *= SG_FEET_TO_METER;
    int altm = (int)( alt * 100.0f );
    data.push_back( (char)( ( altm >> 24 ) & 0xFF ) );
    data.push_back( (char)( ( altm >> 16 ) & 0xFF ) );
    data.push_back( (char)( ( altm >> 8 ) & 0xFF ) );
    data.push_back( (char)( altm & 0xFF ) );
    
    // put in the geoid height in 0.1 meters
    data.push_back( (char)( ( altm >> 24 ) & 0xFF ) );
    data.push_back( (char)( ( altm >> 16 ) & 0xFF ) );
    data.push_back( (char)( ( altm >> 8 ) & 0xFF ) );
    data.push_back( (char)( altm & 0xFF ) );
    
    // get ground speed
    double gskt = fgGetDouble( "/velocities/groundspeed-kt" );
    gskt *= SG_KT_TO_MPS;
    int gsm = (int)( gskt * 100.0f );
    data.push_back( (char)( ( gsm >> 8 ) & 0xFF ) );
    data.push_back( (char)( gsm & 0xFF ) );
    
    // ground track
    double trkdeg = fgGetDouble("/orientation/heading-deg");
    int hdg = (int)(trkdeg * 10.0f);
    data.push_back( (char)( ( hdg >> 8 ) & 0xFF ) );
    data.push_back( (char)( hdg & 0xFF ) );
    
    // vertical velocity
    double climb_fpm = fgGetDouble( "/velocities/vertical-speed-fps" );
    climb_fpm *= SG_FEET_TO_METER;
    int vvm = (int)( climb_fpm * 50.0f );
    data.push_back( (char)( ( vvm >> 8 ) & 0xFF ) );
    data.push_back( (char)( vvm & 0xFF ) );
    
    // navigation solution status
    data.push_back( '\0' );
    
    // HFOM/VFOM
    data.append( "\0\x09\0\x09", 4 );
    
    // ARINC 748 Mode
    data.push_back( '\x0D' );
    
    // Channel Tracking
    data += "\x7F\xFF";
    
    // calculate the checksum
    for ( string::const_iterator cli = data.begin(); cli != data.end(); cli++ ) {
        chksum ^= *cli;
    }
    
    string sentence( "@@" );
    sentence += data;
    sentence.push_back( chksum );
    sentence += "\x0D\n";

    length = sentence.length();
    char *bufptr = buf;
    for ( string::const_iterator cli = sentence.begin(); cli != sentence.end(); cli++ ) {
        *bufptr++ = *cli;
    }

    return true;
}


// Wm - Data integrity status
bool FGAV400WSimB::gen_Wm_message() {
    char chksum = 0;
    
    // generate the Wt message
    string data = "Wm";

    // flight phase
    data.push_back( flight_phase );
    
    // HAL and VAL
    if ( hal.empty() ) {
        data.append( "\0\0", 2 );
    }
    else {
        data += hal;
    }
    
    if ( val.empty() ) {
        data.append( "\0\0", 2 );
    }
    else {
        data += val;
    }
    
    // Integrity status
    data.append( "\x00\x00\x00", 3 );
    data.append( "\x00\x01\x00\x01\x00\x01\x00\x01", 8 );
    data.append( "\x00\x0F\x00\x0F\x00\x0F", 6 );
    
    // calculate the checksum
    for ( string::const_iterator cli = data.begin(); cli != data.end(); cli++ ) {
        chksum ^= *cli;
    }
    
    string sentence( "@@" );
    sentence += data;
    sentence.push_back( chksum );
    sentence += "\x0D\n";

    length = sentence.length();
    char *bufptr = buf;
    for ( string::const_iterator cli = sentence.begin(); cli != sentence.end(); cli++ ) {
        *bufptr++ = *cli;
    }

    return true;
}
    
// Wv - 3d velocity
bool FGAV400WSimB::gen_Wv_message() {
    char chksum = 0;
    
    // generate the Wt message
    string data = "Wv";

    // data is valid
    data += "1";
    
    // N velocity in .01 m/s
    double vn_mps = fgGetDouble( "/velocities/speed-north-fps" ) * SG_FEET_TO_METER;
    int vnm = (int)( vn_mps * 100 );
    data.push_back( (char)( ( vnm >> 24 ) & 0xFF ) );
    data.push_back( (char)( ( vnm >> 16 ) & 0xFF ) );
    data.push_back( (char)( ( vnm >> 8 ) & 0xFF ) );
    data.push_back( (char)( vnm & 0xFF ) );
    
    // E velocity in .01 m/s
    double ve_mps = fgGetDouble( "/velocities/speed-east-fps" ) * SG_FEET_TO_METER;
    int vne = (int)( ve_mps * 100 );
    data.push_back( (char)( ( vne >> 24 ) & 0xFF ) );
    data.push_back( (char)( ( vne >> 16 ) & 0xFF ) );
    data.push_back( (char)( ( vne >> 8 ) & 0xFF ) );
    data.push_back( (char)( vne & 0xFF ) );
    
    // Up velocity in .01 m/s
    double climb_mps = fgGetDouble( "/velocities/vertical-speed-fps" ) * SG_FEET_TO_METER;
    int vnup = (int)( climb_mps * 100 );
    data.push_back( (char)( ( vnup >> 24 ) & 0xFF ) );
    data.push_back( (char)( ( vnup >> 16 ) & 0xFF ) );
    data.push_back( (char)( ( vnup >> 8 ) & 0xFF ) );
    data.push_back( (char)( vnup & 0xFF ) );

    // calculate the checksum
    for ( string::const_iterator cli = data.begin(); cli != data.end(); cli++ ) {
        chksum ^= *cli;
    }
    
    string sentence( "@@" );
    sentence += data;
    sentence.push_back( chksum );
    sentence += "\x0D\n";
    
    // cout << sentence;
    length = sentence.length();
    char *bufptr = buf;
    for ( string::const_iterator cli = sentence.begin(); cli != sentence.end(); cli++ ) {
        *bufptr++ = *cli;
    }

    return true;
}


bool FGAV400WSimB::verify_checksum( string message, int datachars ) {
    bool bRet = false;
    string dataseg = message.substr(SOM_SIZE, datachars);
    char chksum = 0;
    char cs = message[SOM_SIZE + datachars];
    for ( string::const_iterator cli = dataseg.begin();
          cli != dataseg.end(); cli++ ) {
        chksum ^= *cli;
    }
    
    if ( chksum == cs ) {
        bRet = true;
    }
    else {
        SG_LOG( SG_IO, SG_INFO, "bad input checksum: " << message );
        //string msgid = asciitize_message( message );
        //printf( "FGAV400SimB::verify_checksum bad input checksum:\n%s\n", msgid.c_str() );
    }
        
    return( bRet );
}


string FGAV400WSimB::asciitize_message( string message ) {
    string asciimsg;

    for ( string::const_iterator cli = message.begin();
          cli != message.end(); cli++ ) 
    {
        unsigned char uc = static_cast<unsigned char>(*cli);
        if ( uc >= 32 && uc <= 127 ) {
            asciimsg += *cli;
        }
        else {
            char tempbuf[20];
            sprintf( tempbuf, "\\x%02X", uc );
            asciimsg += tempbuf;
        }
    }
    
    return( asciimsg );
}

string FGAV400WSimB::buffer_to_string() {
    string message;
    char *bufctr = buf;
    
    for ( int xctr = 0; xctr < length; xctr++ ) {
        message.push_back( *bufctr++ );
    }
    return( message );
}
  

// parse AV400Sim message
bool FGAV400WSimB::parse_message() {
    SG_LOG( SG_IO, SG_INFO, "parse AV400WSimB message" );

    string msg = buffer_to_string();
    
    string som = msg.substr(0, 2);
    if ( som != "@@" ) {
        SG_LOG( SG_IO, SG_INFO, "bad start message" );
        return false;
    }

    string ident = msg.substr(2,2);
    
    if ( ident == "AH" ) { // Flight Phase
        if ( verify_checksum( msg, 3 ) ) {
            flight_phase = msg[4];
            //string ascmsg = asciitize_message( msg );
            //printf( "%10d received AH %s\n", outputctr, ascmsg.c_str() );
            switch( flight_phase ) {
                case FGAV400WSimB::PHASE_OCEANIC: // Oceanic
                    if ( hal.empty() ) {
                        hal = "\x39\xE0";
                    }
                    if ( val.empty() ) {
                        val = "\x00\x00";
                    }
                    fgSetBool( "/instrumentation/av400w/has-gs", false );
                    break;
                    
                case PHASE_ENROUTE: // Enroute
                    if ( hal.empty() ) {
                        hal = "\x1C\xF0";
                    }
                    if ( val.empty() ) {
                        val = "\x00\x00";
                    }
                    fgSetBool( "/instrumentation/av400w/has-gs", false );
                    break;
                    
                case PHASE_TERM: // Terminal
                    if ( hal.empty() ) {
                        hal = "\x0E\x78";
                    }
                    if ( val.empty() ) {
                        val = "\x00\x00";
                    }
                    fgSetBool( "/instrumentation/av400w/has-gs", false );
                    break;
                    
                case PHASE_NONPREC: // Non Precision Approach
                    if ( hal.empty() ) {
                        hal = "\x04\x57";
                    }
                    if ( val.empty() ) {
                        val = "\x00\x00";
                    }
                    fgSetBool( "/instrumentation/av400w/has-gs", false );
                    break;
                    
                case PHASE_LNAVVNAV: // LNAV/VNAV
                    if ( hal.empty() ) {
                        hal = "\x04\x57";
                    }
                    if ( val.empty() ) {
                        val = "\x00\x64";
                    }
                    fgSetBool( "/instrumentation/av400w/has-gs", true );
                    break;
                    
                case PHASE_LPVLP: // LPV/LP
                    if ( hal.empty() ) {
                        hal = "\x00\x00";
                    }
                    if ( val.empty() ) {
                        val = "\x00\x00";
                    }
                    fgSetBool( "/instrumentation/av400w/has-gs", true );
                    break;
                    
                default:
                    if ( hal.empty() ) {
                        hal = "\x00\x00";
                    }
                    if ( val.empty() ) {
                        val = "\x00\x00";
                    }
                    fgSetBool( "/instrumentation/av400w/has-gs", false );
                    break;
            }
            //printf( "AH flight status: %c\n", flight_phase + '0' );
        }
    }
    else if ( ident == "AI" ) { // HAL
        if ( verify_checksum( msg, 4 ) ) {
            hal = msg.substr(4,2);
            //printf( "%10d received AI\n", outputctr );
        }
    }
    else if ( ident == "Cj" ) { // Host ID
        if ( verify_checksum( msg, 2 ) ) {
            req_hostid = true;
            //printf( "%10d received Cj\n", outputctr );
        }
    }
    else if ( ident == "WA" ) { // SBAS selection
        if ( verify_checksum( msg, 5 ) ) {
            sbas_sel = msg.substr( 5, 2 );
            req_sbas = true;
            //printf( "%10d received WA\n", outputctr );
        }
    }
    else if ( ident == "Wd" ) { // VAL
        if ( verify_checksum( msg, 4 ) ) {
            val = msg.substr( 4, 2 );
            //printf( "%10d received Wd\n", outputctr );
        }
    }
    else if ( ident == "WY" ) { // ???? Not listed in protocol document
        // Do nothing until we know what it does
    }
    else {
        string unkmsg = msg.substr( 0, 4 );
        printf( "parse_message unknown: %s\n", unkmsg.c_str() );
    }
    
    return true;
}


// open hailing frequencies
bool FGAV400WSimB::open() {
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
bool FGAV400WSimB::process() {
    SGIOChannel *io = get_io_channel();

    // read the device messages back
    // Because the protocol allows for binary data, we can't just read
    // ascii lines. 
    char readbuf[10];
    char *bufptr = buf;
    int templen;
    bool gotCr = false;
    bool gotLf = false;
    bool som1 = false;
    bool som2 = false;
    length = 0;
    
    while ( ( templen = io->read( readbuf, 1 ) ) == 1 ) {
        if ( !som1 && !som2 ) {
            if ( *readbuf == '@' ) {
                som1 = true;
            }
            else {
                continue;
            }
        }
        else if ( !som2 ) {
            if ( *readbuf == '@' ) {
                som2 = true;
            }
            else {
                som1 = false;
                continue;
            }
        }
        else if ( som1 && som2 ) {
            if ( *readbuf == '\n' && !gotCr ) {  // check for a carriage return
                gotCr = true;
            }
            else if ( *readbuf == '\n' && gotCr ) { // see if we got a cr/lf
                gotLf = true;
            }
            else if ( gotCr ) { // we had a cr but the next char was not a lf, so just must be data
                gotCr = false;
            }
        }
        
        *bufptr++ = *readbuf;
        length++;
        
        if ( gotCr && gotLf ) { // message done
            if ( parse_message() ) {
                // SG_LOG( SG_IO, SG_ALERT, "Success parsing data." );
            } else {
                // SG_LOG( SG_IO, SG_ALERT, "Error parsing data." );
            }
            length = 0;
            break;
        }
    }
   
    
    // Check for polled messages
    if ( req_hostid ) {
        gen_hostid_message();
        if ( ! io->write( buf, length ) ) {
            SG_LOG( SG_IO, SG_WARN, "Error writing data." );
            printf( "Error sending HostID\n" );
            return false;
        }
        //printf( "Sent HostID, %d bytes\n", length );
        req_hostid = false;
    }
    else if ( req_sbas ) {
        gen_sbas_message();
        if ( ! io->write( buf, length ) ) {
            SG_LOG( SG_IO, SG_WARN, "Error writing data." );
            printf( "Error sending SBAS\n" );
            return false;
        }
        //printf( "Sent SBAS, %d bytes\n", length );
        req_sbas = false;
    }
    
    // Send the 5Hz messages
    gen_Wt_message();
    if ( ! io->write( buf, length ) ) {
        SG_LOG( SG_IO, SG_WARN, "Error writing data." );
        printf( "Error writing hz message\n" );
        return false;
    }
    //printf( "Sent Wt, %d bytes\n", length );
    
    gen_Wm_message();
    if ( ! io->write( buf, length ) ) {
        SG_LOG( SG_IO, SG_WARN, "Error writing data." );
        printf( "Error writing hz message\n" );
        return false;
    }
    //printf( "Sent Wm, %d bytes\n", length );

    gen_Wv_message();
    if ( ! io->write( buf, length ) ) {
        SG_LOG( SG_IO, SG_WARN, "Error writing data." );
        printf( "Error writing hz message\n" );
        return false;
    }
    //printf( "Sent Wv, %d bytes\n", length );
    
    hz2count++;
    if ( hz2 > 0 && ( hz2count % hz2cycles == 0 ) ) {
        // Send the 1Hz messages
        gen_Wh_message();
        if ( ! io->write( buf, length ) ) {
            SG_LOG( SG_IO, SG_WARN, "Error writing data." );
            printf( "Error writing hz2 message\n" );
            return false;
        }
        //printf( "Sent Wh, %d bytes\n", length );
        
        gen_Wx_message();
        if ( ! io->write( buf, length ) ) {
            SG_LOG( SG_IO, SG_WARN, "Error writing data." );
            printf( "Error writing hz2 message\n" );
            return false;
        }
        //printf( "Sent Wx, %d bytes\n", length );
    }
    
    // read the device messages back again to make sure we don't miss anything
    bufptr = buf;
    templen = 0;
    gotCr = false;
    gotLf = false;
    som1 = false;
    som2 = false;
    length = 0;
    
    while ( ( templen = io->read( readbuf, 1 ) ) == 1 ) {
        if ( !som1 && !som2 ) {
            if ( *readbuf == '@' ) {
                som1 = true;
            }
            else {
                continue;
            }
        }
        else if ( !som2 ) {
            if ( *readbuf == '@' ) {
                som2 = true;
            }
            else {
                som1 = false;
                continue;
            }
        }
        else if ( som1 && som2 ) {
            if ( *readbuf == '\n' && !gotCr ) {  // check for a carriage return
                gotCr = true;
            }
            else if ( *readbuf == '\n' && gotCr ) { // see if we got a cr/lf
                gotLf = true;
            }
            else if ( gotCr ) { // we had a cr but the next char was not a lf, so just must be data
                gotCr = false;
            }
        }
        
        *bufptr++ = *readbuf;
        length++;
        
        if ( gotCr && gotLf ) { // message done
            //string msg = buffer_to_string();
            //string ascmsg = asciitize_message( msg );
            //printf( "Received message\n" );
            //printf( "%s\n", ascmsg.c_str() );
            //printf( "got message\n" );
            if ( parse_message() ) {
                // SG_LOG( SG_IO, SG_ALERT, "Success parsing data." );
            } else {
                // SG_LOG( SG_IO, SG_ALERT, "Error parsing data." );
            }
            length = 0;
            break;
        }
    }
   

    outputctr++;
    if ( outputctr % 10 == 0 ) {
        //printf( "AV400WSimB::process finished\n" );
    }

    return true;
}


// close the channel
bool FGAV400WSimB::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
        return false;
    }

    return true;
}


