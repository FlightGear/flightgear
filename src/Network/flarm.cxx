// flarm.cxx -- Flarm protocol class
//
// Written by Thorsten Brehm, started November 2017.
//
// Copyright (C) 2017 Thorsten Brehm - brehmt (at) gmail com
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
#include <simgear/constants.h>
#include <simgear/io/iochannel.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/sg_inlines.h>

#include <FDM/flightProperties.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Include/version.h>

#include "flarm.hxx"

using namespace NMEA;

// #define FLARM_DEBUGGING

#ifdef FLARM_DEBUGGING
#warning Flarm debugging is enabled!
#endif

/*
 * The Flarm protocol emulation reports multi-player and AI aircraft within the
 * configured range using NMEA-style messages. The module supports bidirectional
 * communication, i.e. its capable of sending messages, and also of receiving
 * (and replying to) configuration commands. The emulation should be good enough
 * to convince standard NAV/moving map clients (tablet APPs like skydemon, skymap,
 * xcsoar etc) that they're connected to a Flarm device. The clients receive the
 * GPS position and traffic information from the flight simulator.
 *
 * By default, the emulation reports all moving aircraft as targets of the lowest
 * alert level ("traffic info only"). Parked aircraft are not reported.
 *
 * Higher alert levels (low-level/import/urgent alert) are only triggered
 * when the FlightGear TCAS instrument is installed in the aircraft model (yes,
 * Flarm is not TCAS, but reusing the TCAS threat-level for the emulation
 * should be good enough :-) ). The TCAS instrument classifies all AI and MP
 * aircraft into 4 threat levels (from invisible to high alert) - similar to
 * the actual alert levels normally provided by a Flarm device.
 *
 * Supported NMEA messages:
 *      $GPRMC: own position information
 *
 * Supported Garmin proprietary messages:
 *      $PGRMZ: own barometric altitude information in feet
 *
 * Supported Flarm proprietary messages:
 *      $PFLAU: status and intruder data
 *      $PFLAA: data on targets within range
 *      $PFLAV: version information
 *      $PFLAE: device status information
 *      $PFLAC: configuration request
 *      $PFLAS: debug information
 *
 * Module properties:
 *   All Flarm configuration properties are mirrored to /sim/flarm/config.
 *   Useful properties:
 *   /sim/flarm/config/RANGE    Range in meters when considering traffic targets
 *   /sim/flarm/config/NMEAOUT  NMEA message mode (0=off, 1=all, 2=Garmin/NMEA messages only, 3=Flarm messages only)
 *   /sim/flarm/config/ACFT     Aircraft type (1=glider, 3=helicopter, 8=motor aircraft, 9=jet)
 *
 */

FGFlarm::FGFlarm() :
    FGGarmin(),
    mFlarmMessages(FLARM::SET),
    mFlarmConfig(fgGetNode("/sim/flarm/config", true)),
    mLastUpdate(0)
{
    // Flarm (and Garmin) devices normally report barometric altitude in feet
    mMetric = false;
    // disable all Garmin messages, except PGRMZ
    mGarminMessages = GARMIN::PGRMZ;
    // Allow processing more message lines per cycle than with FG's standard NMEA protocol,
    // otherwise we won't reply quickly when a remote sends multiple configuration requests.
    mMaxReceiveLines = 20;
    // allow bidirectional communication (we're sending data and accepting requests)
    mBiDirectionalSupport = true;

#ifdef FLARM_DEBUGGING
    // show I/O debug messages
    sglog().set_log_classes(SG_IO);
    sglog().set_log_priority(SG_DEBUG);
#endif

    // some default configuration data, to please XCSoar and other apps
    const unsigned int zero=0;
    setDefaultConfigValue("ACFT",       9);
    setDefaultConfigValue("ADDWP",      "");
    setDefaultConfigValue("BAUD",       5);
    setDefaultConfigValue("CFLAGS",     zero);
    setDefaultConfigValue("COMPID",     "");
    setDefaultConfigValue("COMPCLASS",  "");
    setDefaultConfigValue("COPIL",      "");
    setDefaultConfigValue("GLIDERID",   "");
    setDefaultConfigValue("GLIDERTYPE", "");
    setDefaultConfigValue("ID",         "0");
    setDefaultConfigValue("LOGINT",     2);
    setDefaultConfigValue("NEWTASK",    "");
    setDefaultConfigValue("NMEAOUT",    1); // all messages enabled
    setDefaultConfigValue("NOTRACK",    zero); // disabled
    setDefaultConfigValue("PILOT",      "Curt"); // :-)))
    setDefaultConfigValue("PRIV",       zero);
    setDefaultConfigValue("RANGE",      25500);
    setDefaultConfigValue("THRE",       2);
    setDefaultConfigValue("UI",         zero);
}


FGFlarm::~FGFlarm() {
}


void FGFlarm::setDefaultConfigValue(const char* ConfigKey, const char* Value)
{
    if (!mFlarmConfig->hasValue(ConfigKey))
        mFlarmConfig->setStringValue(ConfigKey, Value);
}


void FGFlarm::setDefaultConfigValue(const char* ConfigKey, unsigned int Value)
{
    if (!mFlarmConfig->hasValue(ConfigKey))
        mFlarmConfig->setIntValue(ConfigKey, Value);
}


// generate Flarm NMEA messages
bool FGFlarm::gen_message()
{
    // generate generic messages first
    FGGarmin::gen_message();

    // traffic updates once per second only, independent of the normal protocol frequency
    if ((get_count()-mLastUpdate)/get_hz() < 1.0)
    {
        return true;
    }
    mLastUpdate = get_count();

    char nmea[256];
    int TargetCount=0;
    double ClosestDistanceM2 = 99e9;
    double ClosestLond=0.0, ClosestLatd=0.0;
    int ClosestRelVerticalM=0;
    int ClosestThreatLevel=-1;

    // obtain own position
    double latd = mFdm.get_Latitude() * SGD_RADIANS_TO_DEGREES;
    double lond = mFdm.get_Longitude() * SGD_RADIANS_TO_DEGREES;

    // PFLAA (Flarm proprietary)
    if (mFlarmMessages & FLARM::PFLAA)
    {
        double altitude_ft = mFdm.get_Altitude();

        // check all AI/MP aircraft
        SGPropertyNode* pAi = fgGetNode("/ai/models", true);
        simgear::PropertyList aircraftList = pAi->getChildren("aircraft");
        for (simgear::PropertyList::iterator i = aircraftList.begin(); i != aircraftList.end(); ++i)
        {
            SGPropertyNode* pModel = *i;
            if ((pModel)&&(pModel->nChildren()))
            {
                double GroundSpeedKt = pModel->getDoubleValue("velocities/true-airspeed-kt", 0.0);
                int threatLevel = pModel->getIntValue("tcas/threat-level", -99);
                // threatLevel is undefined (-99) when no TCAS is installed
                if (threatLevel == -99)
                {
                    // set threat level to 0 (traffic info) when a/c is moving. Otherwise -1 (invisible).
                    threatLevel = (GroundSpeedKt>1) ? 0 : -1;
                }

                // report traffic, unless considered "invisible"
                if (threatLevel >= 0)
                {
                    // position data of current intruder
                    double targetLatd = pModel->getDoubleValue("position/latitude-deg", 0.0);
                    double targetLond = pModel->getDoubleValue("position/longitude-deg", 0.0);

                    // calculate the relative North and relative East distances in meters, as
                    // required by the Flarm protocol
                    double RelNorthAngleDeg = targetLatd - latd;
                    double RelNorth = ((2*SG_PI*SG_POLAR_RADIUS_M) / 360.0) * RelNorthAngleDeg;

                    double RelEastAngleDeg  = targetLond - lond;
                    double RelEast  = ((2*SG_PI*SG_EQUATORIAL_RADIUS_M) / 360.0) *
                                        abs(cos(latd*SGD_DEGREES_TO_RADIANS)) * RelEastAngleDeg;

#ifdef FLARM_DEBUGGING
                    {
                        double distanceM = sqrt(RelNorth*RelNorth+RelEast*RelEast);
                        pModel->setDoubleValue("flarm/distance", distanceM);
                        pModel->setIntValue("flarm/alive", pModel->getIntValue("flarm/alive",0)+1);
                    }
#endif

                    // do not consider targets beyond 100km (1e5 meters)
                    double FlarmRangeM = mFlarmConfig->getIntValue("RANGE", 25500);
                    double DistanceM2 = RelNorth*RelNorth+RelEast*RelEast;
                    if (DistanceM2 < FlarmRangeM*FlarmRangeM)
                    {
                        TargetCount++;
#if 0//def FLARM_DEBUGGING
                        {
                            double distanceM = sqrt(RelNorth*RelNorth+RelEast*RelEast);
                            printf("%3u: id %3u, %s, distance: %.1fkm, North: %.1f, East: %.1f, speed: %.1f kt\n",
                                    pModel->getIndex(),
                                    pModel->getIntValue("id"),
                                    pModel->getStringValue("callsign", "<no name>"),
                                    distanceM/1000.0, RelNorth/1e3, RelEast/1e3, GroundSpeedKt);
                        }
#endif

                        int RelVerticalM = (pModel->getDoubleValue("position/altitude-ft", 0.0)-altitude_ft)* SG_FEET_TO_METER;
                        int Track = pModel->getDoubleValue("orientation/true-heading-deg", 0.0);
                        int ClimbRateMs = pModel->getDoubleValue("velocities/vertical-speed-fps", 0.0) * (SG_FPS_TO_KT * SG_KT_TO_MPS);
                        int AcftType = 9; // report as jet aircraft for now
                        // generate some fake 6-digit hex code
                        unsigned int ID = pModel->getIntValue("id") & 0x00FFFFFF;
                        //$PFLAA,AlarmLevel,RelNorth,RelEast,RelVertical,IDType,ID,Track,TurnRate,GroundSpeed,ClimbRate,AcftType
                        snprintf( nmea, 256, "$PFLAA,%u,%i,%i,%i,2,%06X,%u,,%i,%i,%u",
                                 threatLevel, (int)RelNorth, (int)RelEast, RelVerticalM, ID,
                                 Track, (int) (GroundSpeedKt*SG_KT_TO_MPS), ClimbRateMs, AcftType);
                        add_with_checksum(nmea, 256);

                        if (DistanceM2 < ClosestDistanceM2)
                        {
                            ClosestDistanceM2 = DistanceM2;
                            ClosestThreatLevel = threatLevel;
                            ClosestRelVerticalM = RelVerticalM;
                            ClosestLatd = targetLatd;
                            ClosestLond = targetLond;
                        }
                    }
                }
            }
        }
    }

    // PFLAU (Flarm proprietary)
    if (mFlarmMessages & FLARM::PFLAU)
    {
        if (ClosestThreatLevel < 0)
        {
            // no threats, but maybe some targets
            snprintf( nmea, 256, "$PFLAU,%u,1,1,1,0,,0,,", TargetCount);
        }
        else
        {
            // calculate the bearing and range of the closest target
            double az2, bearing, distanceM;
            geo_inverse_wgs_84(latd, lond, ClosestLatd, ClosestLond, &bearing, &az2, &distanceM);

            // calculate relative bearing
            double heading = mFdm.get_Psi_deg();
            bearing -= heading;
            SG_NORMALIZE_RANGE(bearing, -180.0, 180.0);

            // set alert mode, depending on TCAS threat level
            int AlertMode = 0; // no alert
            if (ClosestThreatLevel >= 2) // TCAS RA alert
                AlertMode = 2; // alarm!
            else
            if (ClosestThreatLevel == 1) // TCAS proximity alert
                AlertMode = 1; // warning
            snprintf( nmea, 256, "$PFLAU,%u,1,1,1,%u,%.0f,%u,%u,%u",
                    TargetCount, ClosestThreatLevel, bearing, AlertMode, ClosestRelVerticalM, (unsigned int) distanceM);
        }
        add_with_checksum(nmea, 256);
    }

    return true;
}

// process a Flarm sentence
void FGFlarm::parse_message(const std::vector<std::string>& tokens)
{
    string::size_type begin = 0, end;
    char nmea[256];

    if ( tokens[0] == "PFLAE" )
    {
        if (tokens.size()<2)
            return;

        // #1: request
        const string& request = tokens[1];
        SG_LOG( SG_IO, SG_DEBUG, "  PFLAE request = " << request );
        if (request == "R")
        {
            // report "no errors"
            snprintf( nmea, 256, "$PFLAE,A,0,0");
            add_with_checksum(nmea, 256);
            SG_LOG( SG_IO, SG_DEBUG, "  PFLAE reply= " << nmea );
        }
    }
    else
    if ( tokens[0] == "PFLAV" )
    {
        if (tokens.size()<2)
            return;

        // #1: request
        const string& request = tokens[1];
        SG_LOG( SG_IO, SG_DEBUG, "  PFLAV version request = " << request );
        if (request == "R")
        {
            // report some fixed version to please the requesting device
            snprintf( nmea, 256, "$PFLAV,A,2.00,6.00,");
            add_with_checksum(nmea, 256);
            SG_LOG( SG_IO, SG_DEBUG, "  PFLAV reply= " << nmea );
        }
    }
    else
    if ( tokens[0] == "PFLAS" )
    {
        // status/debug information

        if (tokens.size()<2)
            return;

        // #1: request
        const string& request = tokens[1];
        SG_LOG( SG_IO, SG_DEBUG, "  PFLAS status/debug request = " << request );
        if (request == "R")
        {
            // Report some debug data to please the requesting device.
            // Apparently debug replies are not in NMEA format and contain plain text.
            const char* FlrmDebugReply = (
                    "------------------------------------------\r\n"
                    "FlightGear " FLIGHTGEAR_VERSION "\r\n"
                    "Revision   " REVISION "\r\n"
                    "------------------------------------------\r\n");
            mNmeaSentence += FlrmDebugReply;
            SG_LOG( SG_IO, SG_DEBUG, "  PFLAS reply = " << FlrmDebugReply );
        }
    }
    else
    if ( tokens[0] == "PFLAC" )
    {
        // configuration command

        if (tokens.size()<3)
            return;

        bool Error = true;

        // #1: request
        const string& request = tokens[1];

        SG_LOG( SG_IO, SG_DEBUG, "  PFLAC config request = " << request );

        // #2: keyword
        const string& keyword = tokens[2];
        SG_LOG( SG_IO, SG_DEBUG, "  PFLAC config request = " << keyword );

        // check if the config element is supported
        SGPropertyNode* configNode = mFlarmConfig->getChild(keyword,0,false);
        if (!configNode)
        {
            // unsupported configuration element
        }
        else
        if (request == "R")
        {
            // reply with config data
            snprintf( nmea, 256, "$PFLAC,A,%s,%s",
                    keyword.c_str(), configNode->getStringValue());
            add_with_checksum(nmea, 256);
            Error = false;
        }
        else
        if (request == "S")
        {
            if (tokens.size()<4)
                return;
            Error = false;

            // special handling for some parameters
            if (keyword == "NMEAOUT")
            {
                // #3: value
                int value = (int) atof(tokens[3].c_str());
                switch(value % 10)
                {
                    case 0:
                        // disable all periodic messages
                        mNmeaMessages   = 0;
                        mGarminMessages = 0;
                        mFlarmMessages  = 0;
                        break;
                    case 1:
                        // enable all periodic messages
                        mNmeaMessages   = NMEA::SET;
                        mGarminMessages = GARMIN::PGRMZ;
                        mFlarmMessages  = FLARM::SET;
                        break;
                    case 2:
                        // enable all, except periodic Flarm messages
                        mNmeaMessages   = NMEA::SET;
                        mGarminMessages = GARMIN::PGRMZ;
                        mFlarmMessages  = 0;
                        break;
                    case 3:
                        // enable all, except periodic NMEA messages
                        mNmeaMessages   = 0;
                        mGarminMessages = GARMIN::PGRMZ;
                        mFlarmMessages  = FLARM::SET;
                        break;
                    default:
                        Error = true;
                }
            }

            if (!Error)
            {
                // just store the config data as it is
                configNode->setStringValue(tokens[3]);

                // reply
                snprintf( nmea, 256, "$PFLAC,A,%s,%s",
                        keyword.c_str(), configNode->getStringValue());
                add_with_checksum(nmea, 256);
            }
        }

        if (Error)
        {
            // report error for unsupported requests
            snprintf( nmea, 256, "$PFLAC,A,ERROR");
            add_with_checksum(nmea, 256);
        }
    }
    else
    {
        // Invalid or unsupported message.
        // In flarm mode, we only accept flarm messages as input, but no other messages, i.e. no position updates.
        // Use the Garmin or NMEA basic protocols to feed position data into FG.
        SG_LOG( SG_IO, SG_DEBUG, "  Unsupported message = " << tokens[0] );
    }
}
