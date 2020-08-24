/*
 * fgcom - VoIP-Client for the FlightGear-Radio-Infrastructure
 *
 * This program realizes the usage of the VoIP infractructure based
 * on flight data which is send from FlightGear with an external
 * protocol to this application.
 *
 * Clement de l'Hamaide - Jan 2014
 * Re-writting of FGCom standalone
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#ifndef __FGCOM_H__
#define __FGCOM_H__

// avoid name clash with winerror.h
#define FGC_SUCCESS(__x__)		(__x__ == 0)
#define FGC_FAILED(__x__)		(__x__ < 0)

#ifdef _MSC_VER
#define snprintf _snprintf
#ifdef WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#endif


#include <map>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#ifndef FGCOM_VERSION
#ifdef FLIGHTGEAR_VERSION
#define FGCOM_VERSION FLIGHTGEAR_VERSION
#else
#define FGCOM_VERSION "unknown"
#endif
#endif

#define MAXBUFLEN 1024

enum Modes {
    ATC,
    PILOT,
    OBS,
    TEST
};

enum ActiveComm {
    COM1,
    COM2
};

struct Data
{
    int     ptt;
    float   com1;
    float   com2;
    double  lon;
    double  lat;
    double  alt;
    float   outputVol;
    float   silenceThd;
    std::string callsign;
};

struct Airport
{
    double      frequency;
    double      latitude;
    double      longitude;
    double      distanceNm;
    std::string icao;
    std::string type;
    std::string name;
};

// Internal functions
int         usage();
int         version();
void        quit(int state);
bool        isInRange(std::string icao, double acftLat, double acftLon, double acftAlt);
std::string computePhoneNumber(double freq, std::string icao, bool atis = false);
std::string getClosestAirportForFreq(double freq, double acftLat, double acftLon, double acftAlt);
std::multimap<int, Airport> getAirportsData();

// Library functions
bool lib_init();
bool lib_hangup();
bool lib_shutdown();
bool lib_call(std::string icao, double freq);
bool lib_directCall(std::string icao, double freq, std::string num);

int  lib_registration();
int  iaxc_callback(iaxc_event e);

void lib_setSilenceThreshold(double thd);
void lib_setCallerId(std::string callsign);
void lib_setVolume(double input, double output);

#endif
