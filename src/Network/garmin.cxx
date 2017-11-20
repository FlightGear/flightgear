// garmin.cxx -- Garmin protocol class
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

#include <simgear/debug/logstream.hxx>
#include <FDM/flightProperties.hxx>
#include <simgear/constants.h>
#include "garmin.hxx"

using namespace NMEA;

FGGarmin::FGGarmin() :
    FGNMEA(),
    mGarminMessages(GARMIN::PGRMZ),
    mMetric(true) // use metric altitude reports
    // In fact Garmin devices normally seem report barometric altitude in feet (not meters), but
    // the FG implementation has always used metric reports for years (and we keep it this way,
    // to avoid complaints about changed implementation). The unit is also part of the NMEA message,
    // so smart devices are probably ok with both anyway.
{
    // only enable the GPRMC standard NMEA message
    mNmeaMessages = NMEA::GPRMC;
    // Garmin uses CR-LF line feeds.
    mLineFeed = "\r\n";
}


FGGarmin::~FGGarmin()
{
}


// generate Garmin NMEA messages
bool FGGarmin::gen_message()
{
    (void) FGNMEA::gen_message();

    char nmea[256];

    // RMZ sentence (Garmin proprietary)
    if (mGarminMessages & GARMIN::PGRMZ)
    {
        double altitude_ft = mFdm.get_Altitude();

        // $PGRMZ,AAAA.A,F,T*XX
        if (mMetric)
            sprintf( nmea, "$PGRMZ,%.1f,M,3", altitude_ft * SG_FEET_TO_METER );
        else
            sprintf( nmea, "$PGRMZ,%.1f,F,3", altitude_ft );
        add_with_checksum(nmea, 256);
    }

    return true;
}

// process a Garmin sentence
void FGGarmin::parse_message(const std::vector<std::string>& tokens)
{
    if (tokens[0] == "PGRMZ")
    {
        if (tokens.size()<3)
            return;

        // #1: altitude
        double altitude = atof( tokens[1].c_str() );

        // #2: altitude units
        const string& alt_units = tokens[2];
        if ( alt_units != "F" && alt_units != "f" )
            altitude *= SG_METER_TO_FEET;

        mFdm.set_Altitude( altitude );

        SG_LOG( SG_IO, SG_DEBUG, " altitude  = " << altitude );
    }
    else
    {
        // not a Garmin message. Maybe standard NMEA message.
        FGNMEA::parse_message(tokens);
    }
}
