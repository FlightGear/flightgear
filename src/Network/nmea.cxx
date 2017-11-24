// nmea.cxx -- NMEA protocol class
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
#include <cstdio>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#include <FDM/flightProperties.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "nmea.hxx"

FGNMEA::FGNMEA() :
    mLength(0),
    mNmeaMessages(NMEA::SET),
    // by default, expect 2 messages per iteration (input)
    mMaxReceiveLines(2),
    mBiDirectionalSupport(false), // protocol normally only supports input _or_ output
    mLineFeed("\n")
{
}


FGNMEA::~FGNMEA() {
}


// calculate the NMEA check sum
void FGNMEA::add_with_checksum(char *sentence, unsigned int buf_size) {
    unsigned int i;
    unsigned char sum = 0;

    for (i = 1; sentence[i] != 0; i++ ) {
        sum ^= sentence[i];
    }

    if (i + 6 < buf_size)
        snprintf( &sentence[i], 6, "*%02X%s", sum, mLineFeed);

    SG_LOG( SG_IO, SG_DEBUG, sentence );

    mNmeaSentence += sentence;
}


// generate NMEA message
bool FGNMEA::gen_message()
{
    char dir;
    int deg;
    double min;
    char nmea[256];

    SGTime *t = globals->get_time_params();

    char utc[10];
    sprintf( utc, "%02d%02d%02d", 
             t->getGmt()->tm_hour, t->getGmt()->tm_min, t->getGmt()->tm_sec );

    char lat[20];
    {
        double latd = mFdm.get_Latitude() * SGD_RADIANS_TO_DEGREES;
        if ( latd < 0.0 ) {
            latd = -latd;
            dir = 'S';
        } else {
            dir = 'N';
        }
        deg = (int)(latd);
        min = (latd - (double)deg) * 60.0;
        sprintf( lat, "%02d%07.4f,%c", abs(deg), min, dir);
    }

    char lon[20];
    {
        double lond = mFdm.get_Longitude() * SGD_RADIANS_TO_DEGREES;
        if ( lond < 0.0 ) {
            lond = -lond;
            dir = 'W';
        } else {
            dir = 'E';
        }
        deg = (int)(lond);
        min = (lond - (double)deg) * 60.0;
        sprintf( lon, "%03d%07.4f,%c", abs(deg), min, dir);
    }

    double vn = fgGetDouble( "/velocities/speed-north-fps" );
    double ve = fgGetDouble( "/velocities/speed-east-fps" );

    char speed[10];
    {
        double fps = sqrt( vn*vn + ve*ve );
        double mps = fps * SG_FEET_TO_METER;
        double kts = mps * SG_METER_TO_NM * 3600;
        sprintf( speed, "%.1f", kts );
    }

    char heading[10];
    {
        double hdg_true = atan2( ve, vn ) * SGD_RADIANS_TO_DEGREES;
        if ( hdg_true < 0 ) {
          hdg_true += 360.0;
        }
        sprintf( heading, "%.1f", hdg_true );
    }

    double altitude_ft = mFdm.get_Altitude();

    char date[10];
    {
        int year = t->getGmt()->tm_year;
        while ( year >= 100 ) { year -= 100; }
        sprintf( date, "%02d%02d%02d", t->getGmt()->tm_mday,
             t->getGmt()->tm_mon+1, year );
    }

    char magvar[10];
    {
        float magdeg = fgGetDouble( "/environment/magnetic-variation-deg" );
        if ( magdeg < 0.0 ) {
            magdeg = -magdeg;
            dir = 'W';
        } else {
            dir = 'E';
        }
        sprintf( magvar, "%.1f,%c", magdeg, dir );
    }

    // RMC sentence
    if (mNmeaMessages & NMEA::GPRMC)
    {
        // $GPRMC,HHMMSS,A,DDMM.MMMM,N,DDDMM.MMMM,W,XXX.X,XXX.X,DDMMYY,XXX.X,E,A*XX
        sprintf( nmea, "$GPRMC,%s,A,%s,%s,%s,%s,%s,%s,A",
                 utc, lat, lon, speed, heading, date, magvar );
        add_with_checksum(nmea, 256);
    }

    // GGA sentence
    if (mNmeaMessages & NMEA::GPGGA)
    {
        // $GPGGA,HHMMSS,DDMM.MMMM,N,DDDMM.MMMM,W,1,NN,H.H,AAAA.A,M,GG.G,M,,*XX
        sprintf( nmea, "$GPGGA,%s,%s,%s,1,08,0.9,%.1f,M,0.0,M,,",
                 utc, lat, lon, altitude_ft * SG_FEET_TO_METER );
        add_with_checksum(nmea, 256);
    }

    // GSA sentence (totally faked)
    if (mNmeaMessages & NMEA::GPGSA)
    {
        sprintf( nmea, "%s%s",
                "$GPGSA,A,3,01,02,03,,05,,07,,09,,11,12,0.9,0.9,2.0*38", mLineFeed );
        SG_LOG( SG_IO, SG_DEBUG, nmea );

        mNmeaSentence += nmea;
    }

    return true;
}

// parse NMEA message.  messages will look something like the
// following:
//
// $GPRMC,163227,A,3321.173,N,11039.855,W,000.1,270.0,171199,0.000,E*61
// $GPGGA,163227,3321.173,N,11039.855,W,1,,,3333,F,,,,*0F

void FGNMEA::parse_line() {
    SG_LOG( SG_IO, SG_DEBUG, "parse nmea message" );

    if (mLength > FG_MAX_MSG_SIZE-1)
        mLength = FG_MAX_MSG_SIZE-1;

    SG_LOG( SG_IO, SG_DEBUG, "entire message = " << mBuf );

    // test leading character
    if (mBuf[0] != '$')
    {
        SG_LOG( SG_IO, SG_DEBUG, "  invalid NMEA start character = " << mBuf[0]);
        return;
    }

    // get rid of checksum and "*" delimiter
    while ((mLength > 0)&&(mBuf[mLength]!='*'))
    {
        mLength--;
    }
    mBuf[mLength] = 0;

    // split string to tokens
    std::vector<std::string> tokens;
    for (unsigned int pos=1;pos < mLength;pos++)
    {
        const char* pCurrent = &mBuf[pos];
        while ((mBuf[pos]!=',')&&(pos<mLength))
            pos++;
        if (mBuf[pos]==',')
            mBuf[pos] = 0;
        tokens.push_back(pCurrent);
    }

    if (tokens.size() == 0)
        return;

    if (tokens.size()>1)
    {
        for (unsigned int i=0;i<tokens.size();i++)
        {
            SG_LOG( SG_IO, SG_DEBUG, "  NMEA token # " << i << ": " << tokens[i]);
        }
        parse_message(tokens);
    }
}

void FGNMEA::parse_message(const std::vector<std::string>& tokens)
{
    double lon_deg, lon_min, lat_deg, lat_min;
    double lon, lat;
    string::size_type begin = 0, end;

    if (tokens[0] == "GPRMC" ) {
        // $GPRMC,HHMMSS,A,DDMM.MMMM,N,DDDMM.MMMM,W,XXX.X,XXX.X,DDMMYY,XXX.X,E,A*XX
        if ( tokens.size()<9)
            return;

        // #1: time
        const string& utc = tokens[1];
        SG_LOG( SG_IO, SG_DEBUG, "  utc = " << utc );

        // #2: junk
        SG_LOG( SG_IO, SG_DEBUG, "  junk = " << tokens[2] );

        // #3: lat val
        lat_deg = atof( tokens[3].substr(0, 2).c_str() );
        lat_min = atof( tokens[3].substr(2).c_str() );
        lat = lat_deg + ( lat_min / 60.0 );

        // #4: lat dir
        if ( tokens[4] == "S" )
            lat *= -1;

        mFdm.set_Latitude( lat * SGD_DEGREES_TO_RADIANS );

        // #5: lon val
        lon_deg = atof( tokens[5].substr(0, 3).c_str() );
        lon_min = atof( tokens[5].substr(3).c_str() );
        lon = lon_deg + ( lon_min / 60.0 );

        // #6: lon dir
        if ( tokens[6] == "W" )
            lon *= -1;

        mFdm.set_Longitude( lon * SGD_DEGREES_TO_RADIANS );
        SG_LOG( SG_IO, SG_DEBUG, "  lat = " << lat << ", lon = " << lon );

#if 0
        double sl_radius, lat_geoc;
        sgGeodToGeoc( mFdm.get_Latitude(),
              mFdm.get_Altitude(),
              &sl_radius, &lat_geoc );
        mFdm.set_Geocentric_Position( lat_geoc,
               mFdm.get_Longitude(),
                       sl_radius + mFdm.get_Altitude() );
#endif

        // #7: speed
        double speed = atof( tokens[7].c_str() );
        mFdm.set_V_calibrated_kts( speed );
        // mFdm.set_V_ground_speed( speed );
        SG_LOG( SG_IO, SG_DEBUG, "  speed = " << speed );

        // #8: heading
        double heading = atof( tokens[8].c_str() );
        mFdm.set_Euler_Angles( mFdm.get_Phi(),
                         mFdm.get_Theta(),
                         heading * SGD_DEGREES_TO_RADIANS );
        SG_LOG( SG_IO, SG_DEBUG, "  heading = " << heading );
    } else
    if (tokens[0] == "GPGGA" ) {
        if ( tokens.size()<11)
            return;

        // #1: time
        const string& utc = tokens[1];
        SG_LOG( SG_IO, SG_DEBUG, "  utc = " << utc );

        // #2: lat val
        lat_deg = atof( tokens[2].substr(0, 2).c_str() );
        lat_min = atof( tokens[2].substr(2).c_str() );
        lat = lat_deg + ( lat_min / 60.0 );

        // #3: lat dir
        if ( tokens[4] == "S" )
            lat *= -1;

        mFdm.set_Latitude( lat * SGD_DEGREES_TO_RADIANS );

        // #4: lon val
        lon_deg = atof( tokens[4].substr(0, 3).c_str() );
        lon_min = atof( tokens[4].substr(3).c_str() );
        lon = lon_deg + ( lon_min / 60.0 );

        // #5: lon dir
        if ( tokens[5] == "W" )
            lon *= -1;

        mFdm.set_Longitude( lon * SGD_DEGREES_TO_RADIANS );
        SG_LOG( SG_IO, SG_DEBUG, "  lat = " << lat << ", lon = " << lon );

        // #6: junk
        SG_LOG( SG_IO, SG_DEBUG, "  junk = " << tokens[6] );

        // #7: junk
        SG_LOG( SG_IO, SG_DEBUG, "  junk = " << tokens[7] );

        // #8: junk
        SG_LOG( SG_IO, SG_DEBUG, "  junk = " << tokens[8] );

        // #9: altitude
        double altitude = atof( tokens[9].c_str() );

        // #10: altitude unit
        const string& alt_units = tokens[10];

        if ( alt_units != "F" && alt_units != "f" ) {
            altitude *= SG_METER_TO_FEET;
        }

        mFdm.set_Altitude( altitude );

        SG_LOG( SG_IO, SG_DEBUG, " altitude  = " << altitude );
    }
}


// open hailing frequencies
bool FGNMEA::open() {
    if ( is_enabled() ) {
        SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
                << "is already in use, ignoring" );
        return false;
    }

    // bidirectional support does not make sense for NMEA (and Garmin) protocols
    if ((get_direction() == SG_IO_BI)&&
        (!mBiDirectionalSupport))
    {
        SG_LOG( SG_IO, SG_ALERT, "NMEA protocol does not support bidirectional communication. "
                "Use 'in' or 'out' instead of 'bi'.");
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

    if (( get_direction() == SG_IO_OUT )||
        ( get_direction() == SG_IO_BI))
    {
        // process output
        gen_message();
        if ((!mNmeaSentence.empty())&&
            (!io->write( mNmeaSentence.c_str(), mNmeaSentence.length() )))
        {
            SG_LOG( SG_IO, SG_WARN, "Error writing data." );
        }
        mNmeaSentence = "";
    }

    if (( get_direction() == SG_IO_IN )||
        ( get_direction() == SG_IO_BI))
    {
        // process input lines (normally expecting 2 messages per cycle)
        for (unsigned int i=0;i<mMaxReceiveLines;i++)
        {
            if ( (mLength = io->readline( mBuf, FG_MAX_MSG_SIZE )) > 0 ) {
                parse_line();
            } else {
                SG_LOG( SG_IO, SG_WARN, "Error reading data." );
            }
        }
    }

    return true; // return value is unused
}


// close the channel
bool FGNMEA::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    return io->close();
}
