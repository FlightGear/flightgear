// igc.cxx -- International Glider Commission (IGC) protocol class
//
// Written by Thorsten Brehm, started October 2013.
//
// Copyright (C) 2013 Thorsten Brehm - brehmt (at) gmail com
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//
///////////////////////////////////////////////////////////////////////////////

/* Usage:
 *  "fgfs --igc=file,out,1,OutputFile.igc"
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if defined( HAVE_VERSION_H ) && HAVE_VERSION_H
#  include <Include/version.h>
#else
#  include <Include/no_version.h>
#endif

#include <stdio.h>  // sprintf
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "igc.hxx"

IGCProtocol::IGCProtocol() :
    length(0)
{
}

IGCProtocol::~IGCProtocol()
{
}

// generate IGC header records
bool IGCProtocol::gen_Hrecords()
{
    const char* AircraftType = fgGetString("/sim/aircraft", "unknown");;
    const char* Callsign     = fgGetString("/sim/multiplay/callsign", "");

    SGTime *t = globals->get_time_params();
    int Day   = t->getGmt()->tm_mday;
    int Month = t->getGmt()->tm_mon+1;
    int Year  = t->getGmt()->tm_year % 100;

#ifdef FLIGHTGEAR_VERSION
    const char* Version = FLIGHTGEAR_VERSION;
#else
    const char* Version = "unknown version";
#endif

    length = snprintf(buf, FG_MAX_MSG_SIZE,
                    "HFDTE%02d%02d%02d\r\n"         // date: DDMMYY
                    "HFFXA001\r\n"                  // fix accuracy (1 meter)
                    "HFGTYGliderType:%s\r\n"        // aircraft type
                    "HFGIDGliderID:%s\r\n"          // callsign
                    "HFDTM100GPSDatum:WGS84\r\n"    // GPS datum type
                    "HFRFWFirmwareVersion:FlightGear %s\r\n" // "firmware" version
                    "HFRHWHardwareVersion:FlightGear Flight Simulator\r\n" // "hardware" version
                    "HFFTYFRType:Flight Simulator\r\n", // logger type
                    Day, Month, Year,
                    AircraftType,
                    Callsign,
                    Version);
    SGIOChannel *io = get_io_channel();
    io->write(buf, length);

    return true;
}

// generate igc B record message
bool IGCProtocol::gen_message()
{

    /* IGC B-record spec:
     *  B H H M M S S D D M MM MM N D D D M MM MM E V P P P P P G G G G G CR LF
     *
     *  Description     Size    Element     Remarks
     *  ------------------------------------------------------------------------------------------------------------------------------------------------
     *  Time UTC        6 bytes HHMMSS      Valid characters 0-9. When a valid GNSS fix is received, the UTC time
     *                                      in a B-record line must be obtained directly from the same GNSS data
     *                                      package that was the source of the Lat/long and GNSS altitude that is
     *                                      recorded in the same B-record line. Other sources for the time in a
     *                                      B-record line (such as the Real-Time Clock in the recorder) must only
     *                                      be used to provide time-continuity where GNSS fixes are not available.
     *  Latitude        8 bytes DDMMmmmN/S  Valid characters N, S, 0-9. Obtained directly from the same GPS data
     *                                      package that was the source of the UTC time that is recorded in the
     *                                      same B-record line. If no latitude is obtained from satellite data,
     *                                      pressure altitude fixing must continue, using times from the RTC.
     *                                      In this case, in B record lines must repeat the last latitude that was
     *                                      obtained from satellite data, until GPS fixing is regained.
     *  Longitude       9 bytes DDDMMmmmE/W Valid characters E,W, 0-9. Obtained directly from the same GPS data
     *                                      package that was the source of UTC time that is recorded in the same
     *                                      B-record line. If no longitude is obtained from satellite data,
     *                                      pressure altitude fixing must continue, using times from the RTC.
     *                                      In this case, in B record lines must repeat the last longitude
     *                                      that was obtained from satellite data, until GPS fixing is regained.
     *  Fix validity    1 byte. A or V      Use A for a 3D fix and V for a 2D fix (no GPS altitude) or for no
     *                                      GPS data (pressure altitude data must continue to be recorded using
     *                                      times from the RTC).
     *  Press Alt.      5 bytes PPPPP       Altitude to the ICAO ISA above the 1013.25 hPa sea level datum, valid
     *                                      characters 0-9 and negative sign "-". Negative values to have negative
     *                                      sign instead of leading zero.
     *  GNSS Alt.       5 bytes GGGGG       Altitude above the WGS84 ellipsoid, valid characters 0-9.
     */

     char lonDir = 'E', latDir = 'N';
     int lonDeg, latDeg, lonMin, latMin;

     SGTime *t = globals->get_time_params();

     double deg = fdm.get_Latitude() * SGD_RADIANS_TO_DEGREES;
     if (deg < 0.0)
     {
         deg = -deg;
         latDir = 'S';
     }

     latDeg = (int)(deg);
     latMin = (int)((deg - (double)latDeg) * 60.0 * 1000.0);

     deg = fdm.get_Longitude() * SGD_RADIANS_TO_DEGREES;
     if (deg < 0.0)
     {
         deg = -deg;
         lonDir = 'W';
     }

     lonDeg = (int)(deg);
     lonMin = (int)((deg - (double)lonDeg) * 60.0 * 1000.0);

     int Altitude = fdm.get_Altitude() * SG_FEET_TO_METER;
     if (Altitude < 0)
         Altitude = 0;

     int h = t->getGmt()->tm_hour;
     int m = t->getGmt()->tm_min;
     int s = t->getGmt()->tm_sec;

     // write the B record
     length = snprintf(buf,FG_MAX_MSG_SIZE,
                  "B"
                  "%02d%02d%02d" // UTC time:           HHMMSS
                  "%02d%05d%c"   // Latitude:           DDMMmmmN (or ..S)
                  "%03d%05d%c"   // Longitude:          DDDMMmmmE (or ..W)
                  "A"            // Fix validity:       A for a 3D fix, V for 2D fix
                  "%05d"         // Pressure Altitude:  PPPPP (above 1013.2 hPa)
                  "%05d"         // GNSS Altitude:      AAAAA
                  "\r\n",        // Line feed:          CR LF
                  h, m, s,
                  latDeg, latMin, latDir,
                  lonDeg, lonMin, lonDir,
                  Altitude, // This should be standard pressure altitude instead. Hm, well :).
                  Altitude  // GPS altitude
             );

     return (length > 0);
}

// reading IGC files is not supported
bool IGCProtocol::parse_message()
{
    return false;
}

// write header data
bool IGCProtocol::open()
{
    if ( is_enabled() )
    {
        SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
                << "is already in use, ignoring" );
        return false;
    }

    SGIOChannel *io = get_io_channel();

    if (!io->open( get_direction() ))
    {
        SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
        return false;
    }

    set_enabled( true );

    gen_Hrecords();

    return true;
}

// process work
bool IGCProtocol::process()
{
    SGIOChannel *io = get_io_channel();
    if ( get_direction() == SG_IO_OUT )
    {
        gen_message();
        if (!io->write( buf, length ))
        {
            SG_LOG( SG_IO, SG_WARN, "Error writing data." );
            return false;
        }
    } else
    if ( get_direction() == SG_IO_IN )
    {
        SG_LOG( SG_IO, SG_ALERT, "Error: IGC input is not supported.");
        return false;
    }

    return true;
}

// close the channel
bool IGCProtocol::close()
{
    SGIOChannel *io = get_io_channel();

    set_enabled(false);

    return io->close();
}
