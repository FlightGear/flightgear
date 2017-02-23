//////////////////////////////////////////////////////////////////////
//
// multiplaymgr.cxx
//
// Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
//
// Copyright (C) 2003  Airservices Australia
// Copyright (C) 2005  Oliver Schroeder
// Copyright (C) 2006  Mathias Froehlich
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
//  
//////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <algorithm>
#include <cstring>
#include <errno.h>

#include <simgear/misc/stdint.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>

#include <AIModel/AIManager.hxx>
#include <AIModel/AIMultiplayer.hxx>
#include <Main/fg_props.hxx>
#include "multiplaymgr.hxx"
#include "mpmessages.hxx"
#include "MPServerResolver.hxx"
#include <FDM/flightProperties.hxx>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <WS2tcpip.h>
#endif
using namespace std;

#define MAX_PACKET_SIZE 1200
#define MAX_TEXT_SIZE 128

/*
 * With the V2 protocol it should be possible to transmit using a different type/encoding than the property has,
 * so it should be possible to transmit a bool as 
 */
enum TransmissionType {
    TT_ASIS = 0, // transmit as defined in the property. This is the default
    TT_BOOL = simgear::props::BOOL,
    TT_INT = simgear::props::INT,
    TT_FLOAT = simgear::props::FLOAT,
    TT_STRING = simgear::props::STRING,
    TT_SHORTINT = 0x100,
    TT_SHORT_FLOAT_NORM = 0x101, // -1 .. 1 encoded into a short int (16 bit)
    TT_SHORT_FLOAT_1 = 0x102, //range -3276.7 .. 3276.7  float encoded into a short int (16 bit) 
    TT_SHORT_FLOAT_2 = 0x103, //range -327.67 .. 327.67  float encoded into a short int (16 bit) 
    TT_SHORT_FLOAT_3 = 0x104, //range -32.767 .. 32.767  float encoded into a short int (16 bit) 
    TT_SHORT_FLOAT_4 = 0x105, //range -3.2767 .. 3.2767  float encoded into a short int (16 bit) 
    TT_BOOLARRAY,
    TT_CHAR,
};
/*
 * Definitions for the version of the protocol to use to transmit the items defined in the IdPropertyList
 * 
 * There is currently a plan to deliver a revised protocol (2017-02-01) that will pack a V1 and a V2 section into 
 * the existing POS_DATA_ID packet. However this cannot function properly with pre 2017.1 clients.
 * As of 2017.1 a version of this module that will handle V2 packets, but only emit V1 packets, as an interim measure
 * to pave the way for future changes; which may be that fgms is updated to handle both packets, or it could be that
 * after a certain amount of time has elapsed this could simply become the de-facto standard.
 * So 2017.1 needs to be configurable to transmit V2 packets (via a property, available on a dialog) - 
 * so that in the future (when 2017.1 is an old release) anyone still running 2017.1 could choose to run V2 on the
 * understanding that V2 will be incompatible with V1 clients.
 */
const int V1_1_PROP_ID = 1;
const int V1_1_2_PROP_ID = 2;
const int V2_PAD_MAGIC = 0x1face1337;

struct IdPropertyList {
    unsigned id;
    const char* name;
    simgear::props::Type type;
    TransmissionType TransmitAs;
    int version;
    int(*convert)(int direction, xdr_data_t*, FGPropertyData*);
};
 
static const IdPropertyList* findProperty(unsigned id);
static int convert_launchbar_state(int direction, xdr_data_t*, FGPropertyData*)
{
	return 0; // no conversion performed
}
// A static map of protocol property id values to property paths,
// This should be extendable dynamically for every specific aircraft ...
// For now only that static list
static const IdPropertyList sIdPropertyList[] = {
    { 10,  "sim/multiplay/protocol-version",          simgear::props::INT,   TT_SHORTINT,  V1_1_PROP_ID, NULL },
    { 100, "surface-positions/left-aileron-pos-norm",  simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 101, "surface-positions/right-aileron-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 102, "surface-positions/elevator-pos-norm",      simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 103, "surface-positions/rudder-pos-norm",        simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 104, "surface-positions/flap-pos-norm",          simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 105, "surface-positions/speedbrake-pos-norm",    simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 106, "gear/tailhook/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 107, "gear/launchbar/position-norm",             simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 108, "gear/launchbar/state",                     simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 109, "gear/launchbar/holdback-position-norm",    simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 110, "canopy/position-norm",                     simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 111, "surface-positions/wing-pos-norm",          simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 112, "surface-positions/wing-fold-pos-norm",     simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },

    { 200, "gear/gear[0]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 201, "gear/gear[0]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 210, "gear/gear[1]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 211, "gear/gear[1]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 220, "gear/gear[2]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 221, "gear/gear[2]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 230, "gear/gear[3]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 231, "gear/gear[3]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 240, "gear/gear[4]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },
    { 241, "gear/gear[4]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL },

    { 300, "engines/engine[0]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 301, "engines/engine[0]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 302, "engines/engine[0]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 310, "engines/engine[1]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 311, "engines/engine[1]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 312, "engines/engine[1]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 320, "engines/engine[2]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 321, "engines/engine[2]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 322, "engines/engine[2]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 330, "engines/engine[3]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 331, "engines/engine[3]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 332, "engines/engine[3]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 340, "engines/engine[4]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 341, "engines/engine[4]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 342, "engines/engine[4]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 350, "engines/engine[5]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 351, "engines/engine[5]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 352, "engines/engine[5]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 360, "engines/engine[6]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 361, "engines/engine[6]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 362, "engines/engine[6]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 370, "engines/engine[7]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 371, "engines/engine[7]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 372, "engines/engine[7]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 380, "engines/engine[8]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 381, "engines/engine[8]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 382, "engines/engine[8]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 390, "engines/engine[9]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 391, "engines/engine[9]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 392, "engines/engine[9]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },

    { 800, "rotors/main/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 801, "rotors/tail/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL },
    { 810, "rotors/main/blade[0]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 811, "rotors/main/blade[1]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 812, "rotors/main/blade[2]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 813, "rotors/main/blade[3]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 820, "rotors/main/blade[0]/flap-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 821, "rotors/main/blade[1]/flap-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 822, "rotors/main/blade[2]/flap-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 823, "rotors/main/blade[3]/flap-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 830, "rotors/tail/blade[0]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 831, "rotors/tail/blade[1]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },

    { 900, "sim/hitches/aerotow/tow/length",                       simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 901, "sim/hitches/aerotow/tow/elastic-constant",             simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 902, "sim/hitches/aerotow/tow/weight-per-m-kg-m",            simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 903, "sim/hitches/aerotow/tow/dist",                         simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 904, "sim/hitches/aerotow/tow/connected-to-property-node",   simgear::props::BOOL, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 905, "sim/hitches/aerotow/tow/connected-to-ai-or-mp-callsign",   simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 906, "sim/hitches/aerotow/tow/brake-force",                  simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 907, "sim/hitches/aerotow/tow/end-force-x",                  simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 908, "sim/hitches/aerotow/tow/end-force-y",                  simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 909, "sim/hitches/aerotow/tow/end-force-z",                  simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 930, "sim/hitches/aerotow/is-slave",                         simgear::props::BOOL, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 931, "sim/hitches/aerotow/speed-in-tow-direction",           simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 932, "sim/hitches/aerotow/open",                             simgear::props::BOOL, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 933, "sim/hitches/aerotow/local-pos-x",                      simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 934, "sim/hitches/aerotow/local-pos-y",                      simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 935, "sim/hitches/aerotow/local-pos-z",                      simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },

    { 1001, "controls/flight/slats",  simgear::props::FLOAT, TT_SHORT_FLOAT_4,  V1_1_PROP_ID, NULL },
    { 1002, "controls/flight/speedbrake",  simgear::props::FLOAT, TT_SHORT_FLOAT_4,  V1_1_PROP_ID, NULL },
    { 1003, "controls/flight/spoilers",  simgear::props::FLOAT, TT_SHORT_FLOAT_4,  V1_1_PROP_ID, NULL },
    { 1004, "controls/gear/gear-down",  simgear::props::FLOAT, TT_SHORT_FLOAT_4,  V1_1_PROP_ID, NULL },
    { 1005, "controls/lighting/nav-lights",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL },
    { 1006, "controls/armament/station[0]/jettison-all",  simgear::props::BOOL, TT_SHORTINT,  V1_1_PROP_ID, NULL },

    { 1100, "sim/model/variant", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 1101, "sim/model/livery/file", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },

    { 1200, "environment/wildfire/data", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 1201, "environment/contrail", simgear::props::INT, TT_SHORTINT,  V1_1_PROP_ID, NULL },

    { 1300, "tanker", simgear::props::INT, TT_SHORTINT,  V1_1_PROP_ID, NULL },

    { 1400, "scenery/events", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },

    { 1500, "instrumentation/transponder/transmitted-id", simgear::props::INT, TT_SHORTINT,  V1_1_PROP_ID, NULL },
    { 1501, "instrumentation/transponder/altitude", simgear::props::INT, TT_ASIS,  TT_SHORTINT, NULL },
    { 1502, "instrumentation/transponder/ident", simgear::props::BOOL, TT_ASIS,  TT_SHORTINT, NULL },
    { 1503, "instrumentation/transponder/inputs/mode", simgear::props::INT, TT_ASIS,  TT_SHORTINT, NULL },

    { 10001, "sim/multiplay/transmission-freq-hz",  simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10002, "sim/multiplay/chat",  simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },

    { 10100, "sim/multiplay/generic/string[0]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10101, "sim/multiplay/generic/string[1]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10102, "sim/multiplay/generic/string[2]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10103, "sim/multiplay/generic/string[3]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10104, "sim/multiplay/generic/string[4]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10105, "sim/multiplay/generic/string[5]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10106, "sim/multiplay/generic/string[6]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10107, "sim/multiplay/generic/string[7]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10108, "sim/multiplay/generic/string[8]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10109, "sim/multiplay/generic/string[9]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10110, "sim/multiplay/generic/string[10]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10111, "sim/multiplay/generic/string[11]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10112, "sim/multiplay/generic/string[12]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10113, "sim/multiplay/generic/string[13]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10114, "sim/multiplay/generic/string[14]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10115, "sim/multiplay/generic/string[15]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10116, "sim/multiplay/generic/string[16]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10117, "sim/multiplay/generic/string[17]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10118, "sim/multiplay/generic/string[18]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },
    { 10119, "sim/multiplay/generic/string[19]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL },

    { 10200, "sim/multiplay/generic/float[0]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10201, "sim/multiplay/generic/float[1]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10202, "sim/multiplay/generic/float[2]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10203, "sim/multiplay/generic/float[3]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10204, "sim/multiplay/generic/float[4]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10205, "sim/multiplay/generic/float[5]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10206, "sim/multiplay/generic/float[6]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10207, "sim/multiplay/generic/float[7]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10208, "sim/multiplay/generic/float[8]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10209, "sim/multiplay/generic/float[9]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10210, "sim/multiplay/generic/float[10]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10211, "sim/multiplay/generic/float[11]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10212, "sim/multiplay/generic/float[12]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10213, "sim/multiplay/generic/float[13]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10214, "sim/multiplay/generic/float[14]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10215, "sim/multiplay/generic/float[15]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10216, "sim/multiplay/generic/float[16]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10217, "sim/multiplay/generic/float[17]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10218, "sim/multiplay/generic/float[18]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10219, "sim/multiplay/generic/float[19]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },

    { 10220, "sim/multiplay/generic/float[20]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10221, "sim/multiplay/generic/float[21]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10222, "sim/multiplay/generic/float[22]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10223, "sim/multiplay/generic/float[23]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10224, "sim/multiplay/generic/float[24]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10225, "sim/multiplay/generic/float[25]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10226, "sim/multiplay/generic/float[26]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10227, "sim/multiplay/generic/float[27]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10228, "sim/multiplay/generic/float[28]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10229, "sim/multiplay/generic/float[29]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10230, "sim/multiplay/generic/float[30]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10231, "sim/multiplay/generic/float[31]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10232, "sim/multiplay/generic/float[32]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10233, "sim/multiplay/generic/float[33]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10234, "sim/multiplay/generic/float[34]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10235, "sim/multiplay/generic/float[35]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10236, "sim/multiplay/generic/float[36]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10237, "sim/multiplay/generic/float[37]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10238, "sim/multiplay/generic/float[38]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10239, "sim/multiplay/generic/float[39]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL },

    { 10300, "sim/multiplay/generic/int[0]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10301, "sim/multiplay/generic/int[1]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10302, "sim/multiplay/generic/int[2]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10303, "sim/multiplay/generic/int[3]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10304, "sim/multiplay/generic/int[4]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10305, "sim/multiplay/generic/int[5]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10306, "sim/multiplay/generic/int[6]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10307, "sim/multiplay/generic/int[7]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10308, "sim/multiplay/generic/int[8]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10309, "sim/multiplay/generic/int[9]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10310, "sim/multiplay/generic/int[10]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10311, "sim/multiplay/generic/int[11]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10312, "sim/multiplay/generic/int[12]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10313, "sim/multiplay/generic/int[13]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10314, "sim/multiplay/generic/int[14]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10315, "sim/multiplay/generic/int[15]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10316, "sim/multiplay/generic/int[16]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10317, "sim/multiplay/generic/int[17]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10318, "sim/multiplay/generic/int[18]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    { 10319, "sim/multiplay/generic/int[19]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },

    //{ 10320, "sim/multiplay/generic/int[20]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10321, "sim/multiplay/generic/int[21]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10322, "sim/multiplay/generic/int[22]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10323, "sim/multiplay/generic/int[23]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10324, "sim/multiplay/generic/int[24]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10325, "sim/multiplay/generic/int[25]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10326, "sim/multiplay/generic/int[26]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10327, "sim/multiplay/generic/int[27]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10328, "sim/multiplay/generic/int[28]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10329, "sim/multiplay/generic/int[29]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10320, "sim/multiplay/generic/int[20]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10321, "sim/multiplay/generic/int[21]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10322, "sim/multiplay/generic/int[22]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10323, "sim/multiplay/generic/int[23]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10324, "sim/multiplay/generic/int[24]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10325, "sim/multiplay/generic/int[25]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10326, "sim/multiplay/generic/int[26]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10327, "sim/multiplay/generic/int[27]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10328, "sim/multiplay/generic/int[28]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10329, "sim/multiplay/generic/int[29]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10330, "sim/multiplay/generic/int[30]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10331, "sim/multiplay/generic/int[31]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10332, "sim/multiplay/generic/int[32]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10333, "sim/multiplay/generic/int[33]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10334, "sim/multiplay/generic/int[34]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10335, "sim/multiplay/generic/int[35]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10336, "sim/multiplay/generic/int[36]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10337, "sim/multiplay/generic/int[37]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10338, "sim/multiplay/generic/int[38]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },
    //{ 10339, "sim/multiplay/generic/int[39]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL },

    { 10500, "sim/multiplay/generic/short[0]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10501, "sim/multiplay/generic/short[1]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10502, "sim/multiplay/generic/short[2]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10503, "sim/multiplay/generic/short[3]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10504, "sim/multiplay/generic/short[4]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10505, "sim/multiplay/generic/short[5]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10506, "sim/multiplay/generic/short[6]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10507, "sim/multiplay/generic/short[7]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10508, "sim/multiplay/generic/short[8]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10509, "sim/multiplay/generic/short[9]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10510, "sim/multiplay/generic/short[10]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10511, "sim/multiplay/generic/short[11]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10512, "sim/multiplay/generic/short[12]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10513, "sim/multiplay/generic/short[13]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10514, "sim/multiplay/generic/short[14]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10515, "sim/multiplay/generic/short[15]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10516, "sim/multiplay/generic/short[16]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10517, "sim/multiplay/generic/short[17]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10518, "sim/multiplay/generic/short[18]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10519, "sim/multiplay/generic/short[19]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10520, "sim/multiplay/generic/short[20]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10521, "sim/multiplay/generic/short[21]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10522, "sim/multiplay/generic/short[22]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10523, "sim/multiplay/generic/short[23]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10524, "sim/multiplay/generic/short[24]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10525, "sim/multiplay/generic/short[25]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10526, "sim/multiplay/generic/short[26]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10527, "sim/multiplay/generic/short[27]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10528, "sim/multiplay/generic/short[28]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10529, "sim/multiplay/generic/short[29]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10520, "sim/multiplay/generic/short[20]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10521, "sim/multiplay/generic/short[21]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10522, "sim/multiplay/generic/short[22]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10523, "sim/multiplay/generic/short[23]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10524, "sim/multiplay/generic/short[24]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10525, "sim/multiplay/generic/short[25]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10526, "sim/multiplay/generic/short[26]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10527, "sim/multiplay/generic/short[27]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10528, "sim/multiplay/generic/short[28]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10529, "sim/multiplay/generic/short[29]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10530, "sim/multiplay/generic/short[30]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10531, "sim/multiplay/generic/short[31]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10532, "sim/multiplay/generic/short[32]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10533, "sim/multiplay/generic/short[33]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10534, "sim/multiplay/generic/short[34]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10535, "sim/multiplay/generic/short[35]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10536, "sim/multiplay/generic/short[36]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10537, "sim/multiplay/generic/short[37]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10538, "sim/multiplay/generic/short[38]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10539, "sim/multiplay/generic/short[39]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10540, "sim/multiplay/generic/short[40]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10541, "sim/multiplay/generic/short[41]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10542, "sim/multiplay/generic/short[42]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10543, "sim/multiplay/generic/short[43]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10544, "sim/multiplay/generic/short[44]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10545, "sim/multiplay/generic/short[45]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10546, "sim/multiplay/generic/short[46]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10547, "sim/multiplay/generic/short[47]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10548, "sim/multiplay/generic/short[48]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10549, "sim/multiplay/generic/short[49]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10550, "sim/multiplay/generic/short[50]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10551, "sim/multiplay/generic/short[51]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10552, "sim/multiplay/generic/short[52]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10553, "sim/multiplay/generic/short[53]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10554, "sim/multiplay/generic/short[54]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10555, "sim/multiplay/generic/short[55]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10556, "sim/multiplay/generic/short[56]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10557, "sim/multiplay/generic/short[57]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10558, "sim/multiplay/generic/short[58]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10559, "sim/multiplay/generic/short[59]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10560, "sim/multiplay/generic/short[60]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10561, "sim/multiplay/generic/short[61]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10562, "sim/multiplay/generic/short[62]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10563, "sim/multiplay/generic/short[63]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10564, "sim/multiplay/generic/short[64]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10565, "sim/multiplay/generic/short[65]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10566, "sim/multiplay/generic/short[66]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10567, "sim/multiplay/generic/short[67]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10568, "sim/multiplay/generic/short[68]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10569, "sim/multiplay/generic/short[69]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10570, "sim/multiplay/generic/short[70]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10571, "sim/multiplay/generic/short[71]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10572, "sim/multiplay/generic/short[72]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10573, "sim/multiplay/generic/short[73]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10574, "sim/multiplay/generic/short[74]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10575, "sim/multiplay/generic/short[75]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10576, "sim/multiplay/generic/short[76]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10577, "sim/multiplay/generic/short[77]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10578, "sim/multiplay/generic/short[78]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },
    { 10579, "sim/multiplay/generic/short[79]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL },

};
/*
 * For the 2017.x version 2 protocol (which layers inside the V1 packet) the properties are sent in two partitions,
 * the first of these is a V1 protocol packet (which should be fine with all clients), and a V2 partition
 * which will contain the newly supported shortint and fixed string encoding schemes.
 */
const int MAX_PARTITIONS = 2;
const unsigned int numProperties = (sizeof(sIdPropertyList) / sizeof(sIdPropertyList[0]));

// Look up a property ID using binary search.
namespace
{
  struct ComparePropertyId
  {
    bool operator()(const IdPropertyList& lhs,
                    const IdPropertyList& rhs)
    {
      return lhs.id < rhs.id;
    }
    bool operator()(const IdPropertyList& lhs,
                    unsigned id)
    {
      return lhs.id < id;
    }
    bool operator()(unsigned id,
                    const IdPropertyList& rhs)
    {
      return id < rhs.id;
    }
  };    
}

const IdPropertyList* findProperty(unsigned id)
{
  std::pair<const IdPropertyList*, const IdPropertyList*> result
    = std::equal_range(sIdPropertyList, sIdPropertyList + numProperties, id,
                       ComparePropertyId());
  if (result.first == result.second) {
    return 0;
  } else {
    return result.first;
  }
}

namespace
{
  bool verifyProperties(const xdr_data_t* data, const xdr_data_t* end)
  {
    using namespace simgear;
    const xdr_data_t* xdr = data;
    while (xdr < end) {
      unsigned id = XDR_decode_uint32(*xdr);
      const IdPropertyList* plist = findProperty(id);
    
      if (plist) {
        xdr++;
        // How we decode the remainder of the property depends on the type
        switch (plist->type) {
        case props::INT:
        case props::BOOL:
        case props::LONG:
          xdr++;
          break;
        case props::FLOAT:
        case props::DOUBLE:
          {
            float val = XDR_decode_float(*xdr);
            if (SGMisc<float>::isNaN(val))
              return false;
            xdr++;
            break;
          }
        case props::STRING:
        case props::UNSPECIFIED:
          {
            // String is complicated. It consists of
            // The length of the string
            // The string itself
            // Padding to the nearest 4-bytes.
            // XXX Yes, each byte is padded out to a word! Too late
            // to change...
            uint32_t length = XDR_decode_uint32(*xdr);
            xdr++;
            // Old versions truncated the string but left the length
            // unadjusted.
            if (length > MAX_TEXT_SIZE)
              length = MAX_TEXT_SIZE;
            xdr += length;
            // Now handle the padding
            while ((length % 4) != 0)
              {
                xdr++;
                length++;
                //cout << "0";
              }
          }
          break;
        default:
          // cerr << "Unknown Prop type " << id << " " << type << "\n";
          xdr++;
          break;
        }            
      }
      else {
        // give up; this is a malformed property list.
        return false;
      }
    }
    return true;
  }
}

class MPPropertyListener : public SGPropertyChangeListener
{
public:
  MPPropertyListener(FGMultiplayMgr* mp) :
    _multiplay(mp)
  {
  }

  virtual void childAdded(SGPropertyNode*, SGPropertyNode*)
  {
    _multiplay->setPropertiesChanged();
  }
private:
	FGMultiplayMgr* _multiplay;
};

//////////////////////////////////////////////////////////////////////
//
//  handle command "multiplayer-connect"
//  args:
//  server: servername to connect (mandatory)
//  txport: outgoing port number (default: 5000)
//  rxport: incoming port number (default: 5000)
//////////////////////////////////////////////////////////////////////
static bool do_multiplayer_connect(const SGPropertyNode * arg) {
	FGMultiplayMgr * self = (FGMultiplayMgr*) globals->get_subsystem("mp");
	if (!self) {
		SG_LOG(SG_NETWORK, SG_WARN, "Multiplayer subsystem not available.");
		return false;
	}

	string servername = arg->getStringValue("servername", "");
	if (servername.empty()) {
		SG_LOG(SG_NETWORK, SG_WARN,
				"do_multiplayer.connect: no server name given, command ignored.");
		return false;
	}
	int port = arg->getIntValue("rxport", -1);
	if (port > 0 && port <= 0xffff) {
		fgSetInt("/sim/multiplay/rxport", port);
	}

	port = arg->getIntValue("txport", -1);
	if (port > 0 && port <= 0xffff) {
		fgSetInt("/sim/multiplay/txport", port);
	}

	servername = servername.substr(0, servername.find_first_of(' '));
	fgSetString("/sim/multiplay/txhost", servername);
	self->reinit();
	return true;
}

//////////////////////////////////////////////////////////////////////
//
//  handle command "multiplayer-disconnect"
//  disconnect args:
//  none
//////////////////////////////////////////////////////////////////////
static bool do_multiplayer_disconnect(const SGPropertyNode * arg) {
	FGMultiplayMgr * self = (FGMultiplayMgr*) globals->get_subsystem("mp");
	if (!self) {
		SG_LOG(SG_NETWORK, SG_WARN, "Multiplayer subsystem not available.");
		return false;
	}

	fgSetString("/sim/multiplay/txhost", "");
	self->reinit();
	return true;
}

//////////////////////////////////////////////////////////////////////
//
//  handle command "multiplayer-refreshserverlist"
//  refreshserverlist args:
//  none
//
//////////////////////////////////////////////////////////////////////

static bool
do_multiplayer_refreshserverlist (const SGPropertyNode * arg)
{
  using namespace simgear;

  FGMultiplayMgr * self = (FGMultiplayMgr*) globals->get_subsystem ("mp");
  if (!self) {
    SG_LOG(SG_NETWORK, SG_WARN, "Multiplayer subsystem not available.");
    return false;
  }

  // MPServerResolver implementation to fill the mp server list
  // deletes itself when done
  class MyMPServerResolver : public MPServerResolver {
  public:
    MyMPServerResolver () :
        MPServerResolver ()
    {
      setTarget (fgGetNode ("/sim/multiplay/server-list", true));
      setDnsName (fgGetString ("/sim/multiplay/dns/query-dn", "flightgear.org"));
      setService (fgGetString ("/sim/multiplay/dns/query-srv-service", "fgms"));
      setProtocol (fgGetString ("/sim/multiplay/dns/query-srv-protocol", "udp"));
      _completeNode->setBoolValue (false);
      _failureNode->setBoolValue (false);
    }

    ~MyMPServerResolver ()
    {
    }

    virtual void
    onSuccess ()
    {
      SG_LOG(SG_NETWORK, SG_DEBUG, "MyMPServerResolver: trigger success");
      _completeNode->setBoolValue (true);
      delete this;
    }
    virtual void
    onFailure ()
    {
      SG_LOG(SG_NETWORK, SG_DEBUG, "MyMPServerResolver: trigger failure");
      _failureNode->setBoolValue (true);
      delete this;
    }

  private:
    SGPropertyNode *_completeNode = fgGetNode ("/sim/multiplay/got-servers", true);
    SGPropertyNode *_failureNode = fgGetNode ("/sim/multiplay/get-servers-failure", true);
  };

  MyMPServerResolver * mpServerResolver = new MyMPServerResolver ();
  mpServerResolver->run ();
  return true;
}

//////////////////////////////////////////////////////////////////////
//
//  MultiplayMgr constructor
//
//////////////////////////////////////////////////////////////////////
FGMultiplayMgr::FGMultiplayMgr() 
{
  mInitialised   = false;
  mHaveServer    = false;
  mListener = NULL;
  globals->get_commands()->addCommand("multiplayer-connect", do_multiplayer_connect);
  globals->get_commands()->addCommand("multiplayer-disconnect", do_multiplayer_disconnect);
  globals->get_commands()->addCommand("multiplayer-refreshserverlist", do_multiplayer_refreshserverlist);
  pXmitLen = fgGetNode("/sim/multiplay/last-xmit-packet-len", true);
  pProtocolVersion = fgGetNode("/sim/multiplay/protocol-version", true);
  pMultiPlayDebugLevel = fgGetNode("/sim/multiplay/debug-level", true);
  pMultiPlayRange = fgGetNode("/sim/multiplay/visibility-range-nm", true);
  pMultiPlayRange->setIntValue(100);
} // FGMultiplayMgr::FGMultiplayMgr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  MultiplayMgr destructor
//
//////////////////////////////////////////////////////////////////////
FGMultiplayMgr::~FGMultiplayMgr() 
{
   globals->get_commands()->removeCommand("multiplayer-connect");
   globals->get_commands()->removeCommand("multiplayer-disconnect");
   globals->get_commands()->removeCommand("multiplayer-refreshserverlist");
} // FGMultiplayMgr::~FGMultiplayMgr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Initialise object
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::init (void) 
{
  //////////////////////////////////////////////////
  //  Initialise object if not already done
  //////////////////////////////////////////////////
  if (mInitialised) {
    SG_LOG(SG_NETWORK, SG_WARN, "FGMultiplayMgr::init - already initialised");
    return;
  }

  SGPropertyNode* propOnline = fgGetNode("/sim/multiplay/online", true);
  propOnline->setBoolValue(false);
  propOnline->setAttribute(SGPropertyNode::PRESERVE, true);

  //////////////////////////////////////////////////
  //  Set members from property values
  //////////////////////////////////////////////////
  short rxPort = fgGetInt("/sim/multiplay/rxport");
  string rxAddress = fgGetString("/sim/multiplay/rxhost");
  short txPort = fgGetInt("/sim/multiplay/txport", 5000);
  string txAddress = fgGetString("/sim/multiplay/txhost");
  
  int hz = fgGetInt("/sim/multiplay/tx-rate-hz", 10);
  if (hz < 1) {
    hz = 10;
  }
  
  mDt = 1.0 / hz;
  mTimeUntilSend = 0.0;
  
  mCallsign = fgGetString("/sim/multiplay/callsign");
  fgGetNode("/sim/multiplay/callsign", true)->setAttribute(SGPropertyNode::PRESERVE, true);
    
  if ((!txAddress.empty()) && (txAddress!="0")) {
    mServer.set(txAddress.c_str(), txPort);
    if (strncmp (mServer.getHost(), "0.0.0.0", 8) == 0) {
      mHaveServer = false;
      SG_LOG(SG_NETWORK, SG_ALERT,
        "Cannot enable multiplayer mode: resolving MP server address '"
        << txAddress << "' failed.");
      return;
    } else {
      SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr - have server");
      mHaveServer = true;
    }
    if (rxPort <= 0)
      rxPort = txPort;
  } else {
    SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr - multiplayer mode disabled (no MP server specificed).");
    return;
  }

  if (rxPort <= 0) {
    SG_LOG(SG_NETWORK, SG_ALERT,
      "Cannot enable multiplayer mode: No receiver port specified.");
    return;
  }
  if (mCallsign.empty())
    mCallsign = "JohnDoe"; // FIXME: use getpwuid
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-txaddress= "<<txAddress);
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-txport= "<<txPort );
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-rxaddress="<<rxAddress );
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-rxport= "<<rxPort);
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-callsign= "<<mCallsign);
  
  mSocket.reset(new simgear::Socket());
  if (!mSocket->open(false)) {
    SG_LOG( SG_NETWORK, SG_ALERT,
            "Cannot enable multiplayer mode: creating data socket failed." );
    return;
  }
  mSocket->setBlocking(false);
  if (mSocket->bind(rxAddress.c_str(), rxPort) != 0) {
    SG_LOG( SG_NETWORK, SG_ALERT,
            "Cannot enable multiplayer mode: binding receive socket failed. "
            << strerror(errno) << "(errno " << errno << ")");
    return;
  }
  
  mPropertiesChanged = true;
  mListener = new MPPropertyListener(this);
  globals->get_props()->addChangeListener(mListener, false);

  fgSetBool("/sim/multiplay/online", true);
  mInitialised = true;

  SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer mode active!");

  if (!fgGetBool("/sim/ai/enabled"))
  {
      // multiplayer depends on AI module
      fgSetBool("/sim/ai/enabled", true);
  }
} // FGMultiplayMgr::init()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Closes and deletes the local player object. Closes
//  and deletes the tx socket. Resets the object state to unitialised.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::shutdown (void) 
{
  fgSetBool("/sim/multiplay/online", false);
  
  if (mSocket.get()) {
    mSocket->close();
    mSocket.reset(); 
  }
  
  MultiPlayerMap::iterator it = mMultiPlayerMap.begin(),
    end = mMultiPlayerMap.end();
  for (; it != end; ++it) {
    it->second->setDie(true);
  }
  mMultiPlayerMap.clear();
  
  if (mListener) {
    globals->get_props()->removeChangeListener(mListener);
    delete mListener;
    mListener = NULL;
  }
  
  mInitialised = false;
} // FGMultiplayMgr::Close(void)
//////////////////////////////////////////////////////////////////////

void
FGMultiplayMgr::reinit()
{
  shutdown();
  init();
}

//////////////////////////////////////////////////////////////////////
//
//  Description: Sends the position data for the local position.
//
//////////////////////////////////////////////////////////////////////

/**
 * The buffer that holds a multi-player message, suitably aligned.
 */
union FGMultiplayMgr::MsgBuf
{
    MsgBuf()
    {
        memset(&Msg, 0, sizeof(Msg));
    }

    T_MsgHdr* msgHdr()
    {
        return &Header;
    }

    const T_MsgHdr* msgHdr() const
    {
        return reinterpret_cast<const T_MsgHdr*>(&Header);
    }

    T_PositionMsg* posMsg()
    {
        return reinterpret_cast<T_PositionMsg*>(Msg + sizeof(T_MsgHdr));
    }

    const T_PositionMsg* posMsg() const
    {
        return reinterpret_cast<const T_PositionMsg*>(Msg + sizeof(T_MsgHdr));
    }

    xdr_data_t* properties()
    {
        return reinterpret_cast<xdr_data_t*>(Msg + sizeof(T_MsgHdr)
                                             + sizeof(T_PositionMsg));
    }

    const xdr_data_t* properties() const
    {
        return reinterpret_cast<const xdr_data_t*>(Msg + sizeof(T_MsgHdr)
                                                   + sizeof(T_PositionMsg));
    }
    /**
     * The end of the properties buffer.
     */
    xdr_data_t* propsEnd()
    {
        return reinterpret_cast<xdr_data_t*>(Msg + MAX_PACKET_SIZE);
    };

    const xdr_data_t* propsEnd() const
    {
        return reinterpret_cast<const xdr_data_t*>(Msg + MAX_PACKET_SIZE);
    };
    /**
     * The end of properties actually in the buffer. This assumes that
     * the message header is valid.
     */
    xdr_data_t* propsRecvdEnd()
    {
        return reinterpret_cast<xdr_data_t*>(Msg + Header.MsgLen);
    }

    const xdr_data_t* propsRecvdEnd() const
    {
        return reinterpret_cast<const xdr_data_t*>(Msg + Header.MsgLen);
    }
    
    xdr_data2_t double_val;
    char Msg[MAX_PACKET_SIZE];
    T_MsgHdr Header;
};

bool
FGMultiplayMgr::isSane(const FGExternalMotionData& motionInfo)
{
    // check for corrupted data (NaNs)
    bool isCorrupted = false;
    isCorrupted |= ((SGMisc<double>::isNaN(motionInfo.time           )) ||
                    (SGMisc<double>::isNaN(motionInfo.lag            )) ||
                    (osg::isNaN(motionInfo.orientation(3) )));
    for (unsigned i = 0; (i < 3)&&(!isCorrupted); ++i)
    {
        isCorrupted |= ((osg::isNaN(motionInfo.position(i)      ))||
                        (osg::isNaN(motionInfo.orientation(i)   ))||
                        (osg::isNaN(motionInfo.linearVel(i))    )||
                        (osg::isNaN(motionInfo.angularVel(i))   )||
                        (osg::isNaN(motionInfo.linearAccel(i))  )||
                        (osg::isNaN(motionInfo.angularAccel(i)) ));
    }
    return !isCorrupted;
}

void
FGMultiplayMgr::SendMyPosition(const FGExternalMotionData& motionInfo)
{
  if ((! mInitialised) || (! mHaveServer))
        return;

  if (! mHaveServer) {
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::SendMyPosition - no server");
      return;
  }

  if (!isSane(motionInfo))
  {
      // Current local data is invalid (NaN), so stop MP transmission.
      // => Be nice to older FG versions (no NaN checks) and don't waste bandwidth.
      SG_LOG(SG_NETWORK, SG_ALERT, "FGMultiplayMgr::SendMyPosition - "
              << "Local data is invalid (NaN). Data not transmitted.");
      return;
  }

  static MsgBuf msgBuf;
  static unsigned msgLen = 0;
  T_PositionMsg* PosMsg = msgBuf.posMsg();

  /*
   * This is to provide a level of compatibility with the new V2 packets.
   * By setting padding it will force older clients to use verify properties which will
   * bail out if there are any unknown props 
   * V2 (for V1 clients) will always have an unknown property because V2 transmits
   * the protocol version as the very first property as a shortint.
   */
  if (getProtocolToUse() > 1)
        PosMsg->pad = XDR_encode_int32(V2_PAD_MAGIC);

  strncpy(PosMsg->Model, fgGetString("/sim/model/path"), MAX_MODEL_NAME_LEN);
  PosMsg->Model[MAX_MODEL_NAME_LEN - 1] = '\0';
  if (fgGetBool("/sim/freeze/replay-state", true)&&
      fgGetBool("/sim/multiplay/freeze-on-replay",true))
  {
      // do not send position updates during replay
      for (unsigned i = 0 ; i < 3; ++i)
      {
          // no movement during replay
          PosMsg->linearVel[i] = XDR_encode_float (0.0);
          PosMsg->angularVel[i] = XDR_encode_float (0.0);
          PosMsg->linearAccel[i] = XDR_encode_float (0.0);
          PosMsg->angularAccel[i] = XDR_encode_float (0.0);
      }
      // all other data remains unchanged (resend last state) 
  }
  else
  {
      PosMsg->time = XDR_encode_double (motionInfo.time);
      PosMsg->lag = XDR_encode_double (motionInfo.lag);
      for (unsigned i = 0 ; i < 3; ++i)
        PosMsg->position[i] = XDR_encode_double (motionInfo.position(i));
      SGVec3f angleAxis;
      motionInfo.orientation.getAngleAxis(angleAxis);
      for (unsigned i = 0 ; i < 3; ++i)
        PosMsg->orientation[i] = XDR_encode_float (angleAxis(i));
      if (fgGetBool("/sim/crashed",true))
      {
        for (unsigned i = 0 ; i < 3; ++i)
        {
          // no speed or acceleration sent when crashed, for better mp patch
          PosMsg->linearVel[i] = XDR_encode_float (0.0);
          PosMsg->angularVel[i] = XDR_encode_float (0.0);
          PosMsg->linearAccel[i] = XDR_encode_float (0.0);
          PosMsg->angularAccel[i] = XDR_encode_float (0.0);
        }
      }
      else
      {
        //including speed up time in velocity and acceleration
        double timeAccel = fgGetDouble("/sim/speed-up");
        for (unsigned i = 0 ; i < 3; ++i)
          PosMsg->linearVel[i] = XDR_encode_float (motionInfo.linearVel(i) * timeAccel);
        for (unsigned i = 0 ; i < 3; ++i)
          PosMsg->angularVel[i] = XDR_encode_float (motionInfo.angularVel(i) * timeAccel);
        for (unsigned i = 0 ; i < 3; ++i)
          PosMsg->linearAccel[i] = XDR_encode_float (motionInfo.linearAccel(i) * timeAccel * timeAccel);
        for (unsigned i = 0 ; i < 3; ++i)
          PosMsg->angularAccel[i] = XDR_encode_float (motionInfo.angularAccel(i) * timeAccel * timeAccel);
      }
      xdr_data_t* ptr = msgBuf.properties();
      xdr_data_t* data = ptr;

      xdr_data_t* msgEnd = msgBuf.propsEnd();
      int protocolVersion = getProtocolToUse();

      //if (pMultiPlayDebugLevel->getIntValue())
      //    msgBuf.zero();

      for (int partition = 1; partition <= protocolVersion; partition++)
      {
          std::vector<FGPropertyData*>::const_iterator it = motionInfo.properties.begin();
          while (it != motionInfo.properties.end()) {
              const struct IdPropertyList* propDef = mPropertyDefinition[(*it)->id];
              if (propDef->version == partition || propDef->version > getProtocolToUse())
              {
                  if (ptr + 2 >= msgEnd)
                  {
                        SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer packet truncated prop id: " << (*it)->id << ": " << propDef->name);
                      break;
                  }

                  // First element is the ID. Write it out when we know we have room for
                  // the whole property.
                  xdr_data_t id = XDR_encode_uint32((*it)->id);


                  /*
                   * 2017.2 protocol has the ability to transmit as a different type (to save space), so
                   * process this when using this protocol (protocolVersion 2) or later
                   */
                  int transmit_type = (*it)->type;

                  if (propDef->TransmitAs != TT_ASIS && protocolVersion > 1)
                  {
                      transmit_type = propDef->TransmitAs;
                  }

                  if (pMultiPlayDebugLevel->getIntValue() & 2)
                      SG_LOG(SG_NETWORK, SG_INFO,
                          "[SEND] pt " << partition <<
                          ": buf[" << ((unsigned int)ptr) - ((unsigned int)data)
                          << "] id=" << (*it)->id << " type " << transmit_type);

                  // The actual data representation depends on the type
                  switch (transmit_type) {
                  case TT_SHORTINT:
                  {
                      *ptr++ = XDR_encode_shortints32((*it)->id, (*it)->int_value);
                      break;
                    }
                    case TT_SHORT_FLOAT_1:
                    {
                        short value = get_scaled_short((*it)->float_value, 10.0);
                        *ptr++ = XDR_encode_shortints32((*it)->id, value);
                        break;
                    }
                    case TT_SHORT_FLOAT_2:
                    {
                        short value = get_scaled_short((*it)->float_value, 100.0);
                        *ptr++ = XDR_encode_shortints32((*it)->id, value);
                        break;
                    }
                    case TT_SHORT_FLOAT_3:
                    {
                        short value = get_scaled_short((*it)->float_value, 1000.0);
                        *ptr++ = XDR_encode_shortints32((*it)->id, value);
                        break;
                    }
                    case TT_SHORT_FLOAT_4:
                    {
                        short value = get_scaled_short((*it)->float_value, 10000.0);
                        *ptr++ = XDR_encode_shortints32((*it)->id, value);
                        break;
                    }

                    case TT_SHORT_FLOAT_NORM:
                    {
                        short value = get_scaled_short((*it)->float_value, 32767.0);
                        *ptr++ = XDR_encode_shortints32((*it)->id, value);
                        break;
                    }

                  case simgear::props::INT:
                  case simgear::props::BOOL:
                  case simgear::props::LONG:
                      *ptr++ = id;
                      *ptr++ = XDR_encode_uint32((*it)->int_value);
                      break;
                  case simgear::props::FLOAT:
                  case simgear::props::DOUBLE:
                      *ptr++ = id;
                      *ptr++ = XDR_encode_float((*it)->float_value);
                      break;
                  case simgear::props::STRING:
                  case simgear::props::UNSPECIFIED:
                  {
                        if (protocolVersion > 1)
                        {
                          // New string encoding:
                          // xdr[0] : ID length packed into 32 bit containing two shorts.
                          // xdr[1..len/4] The string itself (char[length])
                          const char* lcharptr = (*it)->string_value;

                          if (lcharptr != 0)
                          {
                              uint32_t len = strlen(lcharptr);

                              if (len >= MAX_TEXT_SIZE)
                              {
                                  len = MAX_TEXT_SIZE - 1;
                                  SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer property truncated at MAX_TEXT_SIZE in string " << (*it)->id);
                              }

                              char *encodeStart = (char*)ptr;
                              char *msgEndbyte = (char*)msgEnd;

                              if (encodeStart + 2 + len >= msgEndbyte)
                              {
                                  SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer property not sent (no room) string " << (*it)->id);
                                  goto escape;
                              }

                              *ptr++ = XDR_encode_shortints32((*it)->id, len);
                              encodeStart = (char*)ptr;
                              if (len != 0)
                              {
                                  int lcount = 0;
                                  while (*lcharptr && (lcount < MAX_TEXT_SIZE))
                                  {
                                      if (encodeStart + 2 >= msgEndbyte)
                                      {
                                          SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer packet truncated in string " << (*it)->id << " lcount " << lcount);
                                          break;
                                      }
                                      *encodeStart++ = *lcharptr++;
                                      lcount++;
                                  }
                              }
                              ptr = (xdr_data_t*)encodeStart;
                          }
                          else
                          {
                              // empty string, just send the id and a zero length
                              *ptr++ = id;
                              *ptr++ = XDR_encode_uint32(0);
                          }
                      }
                      else {

                          // String is complicated. It consists of
                          // The length of the string
                          // The string itself
                          // Padding to the nearest 4-bytes.        
                          const char* lcharptr = (*it)->string_value;

                          if (lcharptr != 0)
                          {
                              // Add the length         
                              ////cout << "String length: " << strlen(lcharptr) << "\n";
                              uint32_t len = strlen(lcharptr);
                              if (len >= MAX_TEXT_SIZE)
                              {
                                  len = MAX_TEXT_SIZE - 1;
                                  SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer property truncated at MAX_TEXT_SIZE in string " << (*it)->id);
                              }

                              // XXX This should not be using 4 bytes per character!
                              // If there's not enough room for this property, drop it
                              // on the floor.
                              if (ptr + 2 + ((len + 3) & ~3) >= msgEnd)
                              {
                                  SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer property not sent (no room) string " << (*it)->id);
                                  goto escape;
                              }
                              //cout << "String length unint32: " << len << "\n";
                              *ptr++ = id;
                              *ptr++ = XDR_encode_uint32(len);
                              if (len != 0)
                              {
                                  // Now the text itself
                                  // XXX This should not be using 4 bytes per character!
                                  int lcount = 0;
                                  while ((*lcharptr != '\0') && (lcount < MAX_TEXT_SIZE))
                                  {
                                      if (ptr + 2 >= msgEnd)
                                      {
                                          SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer packet truncated in string " << (*it)->id << " lcount " << lcount);
                                          break;
                                      }
                                      *ptr++ = XDR_encode_int8(*lcharptr);
                                      lcharptr++;
                                      lcount++;
                                  }
                                  // Now pad if required
                                  while ((lcount % 4) != 0)
                                  {
                                      if (ptr + 2 >= msgEnd)
                                      {
                                          SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer packet truncated in string " << (*it)->id << " lcount " << lcount);
                                          break;
                                      }
                                      *ptr++ = XDR_encode_int8(0);
                                      lcount++;
                                  }
                              }
                          }
                          else
                          {
                              // Nothing to encode
                              *ptr++ = id;
                              *ptr++ = XDR_encode_uint32(0);
                          }
                      }
                  }
                  break;

                  default:
                      *ptr++ = id;
                      *ptr++ = XDR_encode_float((*it)->float_value);;
                      break;
                  }
              }
              ++it;
          }
      }
      escape:
      msgLen = reinterpret_cast<char*>(ptr) - msgBuf.Msg;

      FillMsgHdr(msgBuf.msgHdr(), POS_DATA_ID, msgLen);

      /*
      * Informational:
      * Save the last packet length sent, and
      * if the property is set then dump the packet length to the console.
      * This should be sufficient for rudimentary debugging - at this point I don't want
      * to spend ages creating some sort of fancy and configurable MP debugging suite that
      * would have limited uses; so what's here is what I would have like to be able to view when
      * developing MP model elements.
      */
      pXmitLen->setIntValue(msgLen);
      if (pMultiPlayDebugLevel->getIntValue() & 4)
      {
          SG_LOG(SG_NETWORK, SG_INFO,
              "[SEND] Packet len " << msgLen);
      }
      if (pMultiPlayDebugLevel->getIntValue() & 2)
          SG_LOG_HEXDUMP(SG_NETWORK, SG_INFO, data, (ptr - data) * sizeof(*ptr));
      /*
       * simple loopback of ourselves - to enable easy MP debug for model developers.
       */
      if (pMultiPlayDebugLevel->getIntValue() & 1)
      {
          long stamp = SGTimeStamp::now().getSeconds();
          ProcessPosMsg(msgBuf, mServer, stamp);
      }
  }
  if (msgLen > 0)
      mSocket->sendto(msgBuf.Msg, msgLen, 0, &mServer);
  SG_LOG(SG_NETWORK, SG_BULK, "FGMultiplayMgr::SendMyPosition");
} // FGMultiplayMgr::SendMyPosition()

short FGMultiplayMgr::get_scaled_short(double v, double scale)
{
    float nv = v * scale;
    if (nv >= 32767) return 32767;
    if (nv <= -32767) return -32767;
    short rv = (short)nv;
    return rv;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Name: SendTextMessage
//  Description: Sends a message to the player. The message must
//  contain a valid and correctly filled out header and optional
//  message body.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::SendTextMessage(const string &MsgText)
{
  if (!mInitialised || !mHaveServer)
    return;

  T_MsgHdr MsgHdr;
  FillMsgHdr(&MsgHdr, CHAT_MSG_ID);
  //////////////////////////////////////////////////
  // Divide the text string into blocks that fit
  // in the message and send the blocks.
  //////////////////////////////////////////////////
  unsigned iNextBlockPosition = 0;
  T_ChatMsg ChatMsg;
  
  char Msg[sizeof(T_MsgHdr) + sizeof(T_ChatMsg)];
  while (iNextBlockPosition < MsgText.length()) {
    strncpy (ChatMsg.Text, 
             MsgText.substr(iNextBlockPosition, MAX_CHAT_MSG_LEN - 1).c_str(),
             MAX_CHAT_MSG_LEN);
    ChatMsg.Text[MAX_CHAT_MSG_LEN - 1] = '\0';
    memcpy (Msg, &MsgHdr, sizeof(T_MsgHdr));
    memcpy (Msg + sizeof(T_MsgHdr), &ChatMsg, sizeof(T_ChatMsg));
    mSocket->sendto (Msg, sizeof(T_MsgHdr) + sizeof(T_ChatMsg), 0, &mServer);
    iNextBlockPosition += MAX_CHAT_MSG_LEN - 1;

  }
  
  
} // FGMultiplayMgr::SendTextMessage ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Name: ProcessData
//  Description: Processes data waiting at the receive socket. The
//  processing ends when there is no more data at the socket.
//  
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::update(double dt) 
{
  if (!mInitialised)
    return;

  /// Just for expiry
  long stamp = SGTimeStamp::now().getSeconds();

  //////////////////////////////////////////////////
  //  Send if required
  //////////////////////////////////////////////////
  mTimeUntilSend -= dt;
  if (mTimeUntilSend <= 0.0) {
    Send();
  }

  //////////////////////////////////////////////////
  //  Read the receive socket and process any data
  //////////////////////////////////////////////////
  ssize_t bytes;
  do {
    MsgBuf msgBuf;
    //////////////////////////////////////////////////
    //  Although the recv call asks for 
    //  MAX_PACKET_SIZE of data, the number of bytes
    //  returned will only be that of the next
    //  packet waiting to be processed.
    //////////////////////////////////////////////////
    simgear::IPAddress SenderAddress;
    int RecvStatus = mSocket->recvfrom(msgBuf.Msg, sizeof(msgBuf.Msg), 0,
                              &SenderAddress);
    //////////////////////////////////////////////////
    //  no Data received
    //////////////////////////////////////////////////
    if (RecvStatus == 0)
        break;

    // socket error reported?
    // errno isn't thread-safe - so only check its value when
    // socket return status < 0 really indicates a failure.
    if ((RecvStatus < 0)&&
        ((errno == EAGAIN) || (errno == 0))) // MSVC output "NoError" otherwise
    {
        // ignore "normal" errors
        break;
    }

    if (RecvStatus<0)
    {
#ifdef _WIN32
        if (::WSAGetLastError() != WSAEWOULDBLOCK) // this is normal on a receive when there is no data
        {
            // with Winsock the error will not be the actual problem.
            SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - Unable to receive data. WSAGetLastError=" << ::WSAGetLastError());
        }
#else
        SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - Unable to receive data. "
            << strerror(errno) << "(errno " << errno << ")");
#endif
        break;
    }

    // status is positive: bytes received
    bytes = (ssize_t) RecvStatus;
    if (bytes <= static_cast<ssize_t>(sizeof(T_MsgHdr))) {
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
              << "received message with insufficient data" );
      break;
    }
    //////////////////////////////////////////////////
    //  Read header
    //////////////////////////////////////////////////
    T_MsgHdr* MsgHdr = msgBuf.msgHdr();
    MsgHdr->Magic       = XDR_decode_uint32 (MsgHdr->Magic);
    MsgHdr->Version     = XDR_decode_uint32 (MsgHdr->Version);
    MsgHdr->MsgId       = XDR_decode_uint32 (MsgHdr->MsgId);
    MsgHdr->MsgLen      = XDR_decode_uint32 (MsgHdr->MsgLen);
    MsgHdr->ReplyPort   = XDR_decode_uint32 (MsgHdr->ReplyPort);
    MsgHdr->Callsign[MAX_CALLSIGN_LEN -1] = '\0';
    if (MsgHdr->Magic != MSG_MAGIC) {
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid magic number!" );
      break;
    }
    if (MsgHdr->Version != PROTO_VER) {
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid protocol number!" );
      break;
    }
    if (static_cast<ssize_t>(MsgHdr->MsgLen) != bytes) {
      SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
             << "message from " << MsgHdr->Callsign << " has invalid length!");
      break;
    }
    //////////////////////////////////////////////////
    //  Process messages
    //////////////////////////////////////////////////
    switch (MsgHdr->MsgId) {
    case CHAT_MSG_ID:
      ProcessChatMsg(msgBuf, SenderAddress);
      break;
    case POS_DATA_ID:
      ProcessPosMsg(msgBuf, SenderAddress, stamp);
      break;
    case UNUSABLE_POS_DATA_ID:
    case OLD_OLD_POS_DATA_ID:
    case OLD_PROP_MSG_ID:
    case OLD_POS_DATA_ID:
      break;
    default:
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
              << "Unknown message Id received: " << MsgHdr->MsgId );
      break;
    }
  } while (bytes > 0);

  // check for expiry
  MultiPlayerMap::iterator it = mMultiPlayerMap.begin();
  while (it != mMultiPlayerMap.end()) {
    if (it->second->getLastTimestamp() + 10 < stamp) {
      std::string name = it->first;
      it->second->setDie(true);
      mMultiPlayerMap.erase(it);
      it = mMultiPlayerMap.upper_bound(name);
    } else
      ++it;
  }
} // FGMultiplayMgr::ProcessData(void)
//////////////////////////////////////////////////////////////////////

void
FGMultiplayMgr::Send()
{
	using namespace simgear;

	findProperties();

	// smooth the send rate, by adjusting based on the 'remainder' time, which
	// is how -ve mTimeUntilSend is. Watch for large values and ignore them,
	// however.
	if ((mTimeUntilSend < 0.0) && (fabs(mTimeUntilSend) < mDt)) {
		mTimeUntilSend = mDt + mTimeUntilSend;
	}
	else {
		mTimeUntilSend = mDt;
	}

	double sim_time = globals->get_sim_time_sec();
	//    static double lastTime = 0.0;

	   // SG_LOG(SG_GENERAL, SG_INFO, "actual dt=" << sim_time - lastTime);
	//    lastTime = sim_time;

	FlightProperties ifce;

	// put together a motion info struct, you will get that later
	// from FGInterface directly ...
	FGExternalMotionData motionInfo;

	// The current simulation time we need to update for,
	// note that the simulation time is updated before calling all the
	// update methods. Thus it contains the time intervals *end* time.
	// The FDM is already run, so the states belong to that time.
	motionInfo.time = sim_time;
	motionInfo.lag = mDt;

	// These are for now converted from lat/lon/alt and euler angles.
	// But this should change in FGInterface ...
	double lon = ifce.get_Longitude();
	double lat = ifce.get_Latitude();
	// first the aprioriate structure for the geodetic one
	SGGeod geod = SGGeod::fromRadFt(lon, lat, ifce.get_Altitude());
	// Convert to cartesion coordinate
	motionInfo.position = SGVec3d::fromGeod(geod);

	// The quaternion rotating from the earth centered frame to the
	// horizontal local frame
	SGQuatf qEc2Hl = SGQuatf::fromLonLatRad((float)lon, (float)lat);
	// The orientation wrt the horizontal local frame
	float heading = ifce.get_Psi();
	float pitch = ifce.get_Theta();
	float roll = ifce.get_Phi();
	SGQuatf hlOr = SGQuatf::fromYawPitchRoll(heading, pitch, roll);
	// The orientation of the vehicle wrt the earth centered frame
	motionInfo.orientation = qEc2Hl*hlOr;

	if (!globals->get_subsystem("flight")->is_suspended()) {
		// velocities
		motionInfo.linearVel = SG_FEET_TO_METER*SGVec3f(ifce.get_uBody(),
			ifce.get_vBody(),
			ifce.get_wBody());
		motionInfo.angularVel = SGVec3f(ifce.get_P_body(),
			ifce.get_Q_body(),
			ifce.get_R_body());

		// accels, set that to zero for now.
		// Angular accelerations are missing from the interface anyway,
		// linear accelerations are screwed up at least for JSBSim.
  //  motionInfo.linearAccel = SG_FEET_TO_METER*SGVec3f(ifce.get_U_dot_body(),
  //                                                    ifce.get_V_dot_body(),
  //                                                    ifce.get_W_dot_body());
		motionInfo.linearAccel = SGVec3f::zeros();
		motionInfo.angularAccel = SGVec3f::zeros();
	}
	else {
		// if the interface is suspendend, prevent the client from
		// wild extrapolations
		motionInfo.linearVel = SGVec3f::zeros();
		motionInfo.angularVel = SGVec3f::zeros();
		motionInfo.linearAccel = SGVec3f::zeros();
		motionInfo.angularAccel = SGVec3f::zeros();
	}

	PropertyMap::iterator it;
	for (it = mPropertyMap.begin(); it != mPropertyMap.end(); ++it) {
		FGPropertyData* pData = new FGPropertyData;
		pData->id = it->first;
		pData->type = findProperty(pData->id)->type;

		switch (pData->type) {
		case TT_SHORTINT:
        case TT_SHORT_FLOAT_1:
        case TT_SHORT_FLOAT_2:
        case TT_SHORT_FLOAT_3:
        case TT_SHORT_FLOAT_4:
        case TT_SHORT_FLOAT_NORM:
		case props::INT:
		case props::LONG:
		case props::BOOL:
			pData->int_value = it->second->getIntValue();
			break;
		case props::FLOAT:
		case props::DOUBLE:
			pData->float_value = it->second->getFloatValue();
			break;
		case props::STRING:
		case props::UNSPECIFIED:
		{
			// FIXME: We assume unspecified are strings for the moment.

			const char* cstr = it->second->getStringValue();
			int len = strlen(cstr);

			if (len > 0)
			{
				pData->string_value = new char[len + 1];
				strcpy(pData->string_value, cstr);
			}
			else
			{
				// Size 0 - ignore
				pData->string_value = 0;
			}

			//cout << " Sending property " << pData->id << " " << pData->type << " " <<  pData->string_value << "\n";
			break;
		}
		default:
			// FIXME Currently default to a float. 
			//cout << "Unknown type when iterating through props: " << pData->type << "\n";
			pData->float_value = it->second->getFloatValue();
			break;
		}
		motionInfo.properties.push_back(pData);
	}
    SendMyPosition(motionInfo);
}


//////////////////////////////////////////////////////////////////////
//
//  handle a position message
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::ProcessPosMsg(const FGMultiplayMgr::MsgBuf& Msg,
	const simgear::IPAddress& SenderAddress, long stamp)
{
	const T_MsgHdr* MsgHdr = Msg.msgHdr();
	if (MsgHdr->MsgLen < sizeof(T_MsgHdr) + sizeof(T_PositionMsg)) {
		SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
			<< "Position message received with insufficient data");
		return;
	}
	const T_PositionMsg* PosMsg = Msg.posMsg();
	FGExternalMotionData motionInfo;
	motionInfo.time = XDR_decode_double(PosMsg->time);
	motionInfo.lag = XDR_decode_double(PosMsg->lag);
	for (unsigned i = 0; i < 3; ++i)
		motionInfo.position(i) = XDR_decode_double(PosMsg->position[i]);
	SGVec3f angleAxis;
	for (unsigned i = 0; i < 3; ++i)
		angleAxis(i) = XDR_decode_float(PosMsg->orientation[i]);
	motionInfo.orientation = SGQuatf::fromAngleAxis(angleAxis);
	for (unsigned i = 0; i < 3; ++i)
		motionInfo.linearVel(i) = XDR_decode_float(PosMsg->linearVel[i]);
	for (unsigned i = 0; i < 3; ++i)
		motionInfo.angularVel(i) = XDR_decode_float(PosMsg->angularVel[i]);
	for (unsigned i = 0; i < 3; ++i)
		motionInfo.linearAccel(i) = XDR_decode_float(PosMsg->linearAccel[i]);
	for (unsigned i = 0; i < 3; ++i)
		motionInfo.angularAccel(i) = XDR_decode_float(PosMsg->angularAccel[i]);

	// sanity check: do not allow injection of corrupted data (NaNs)
	if (!isSane(motionInfo))
	{
		// drop this message, keep old position until receiving valid data
		SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::ProcessPosMsg - "
			<< "Position message with invalid data (NaN) received from "
			<< MsgHdr->Callsign);
		return;
	}

	//cout << "INPUT MESSAGE\n";

	// There was a bug in 1.9.0 and before: T_PositionMsg was 196 bytes
	// on 32 bit architectures and 200 bytes on 64 bit, and this
	// structure is put directly on the wire. By looking at the padding,
	// we can sort through the mess, mostly:
	// If padding is 0 (which is not a valid property type), then the
	// message was produced by a new client or an old 64 bit client that
	// happened to have 0 on the stack;
	// Else if the property list starting with the padding word is
	// well-formed, then the client is probably an old 32 bit client and
	// we'll go with that;
	// Else it is an old 64-bit client and properties start after the
	// padding.
	// There is a chance that we could be fooled by garbage in the
	// padding looking like a valid property, so verifyProperties() is
	// strict about the validity of the property values.
	const xdr_data_t* xdr = Msg.properties();
	const xdr_data_t* data = xdr;
    int MsgLenBytes = Msg.Header.MsgLen;

    /*
     * with V2 we use the pad to forcefully invoke older clients to verify (and discard)
     * our new protocol.
     * This will preserve the position info but not transmit the properties; which is about
     * the most reasonable compromise we can have
     */
    if (PosMsg->pad != 0 && XDR_decode_int32(PosMsg->pad) != V2_PAD_MAGIC) {
        if (verifyProperties(&PosMsg->pad, Msg.propsRecvdEnd()))
            xdr = &PosMsg->pad;
        else if (!verifyProperties(xdr, Msg.propsRecvdEnd()))
            goto noprops;
    }
    while (xdr < Msg.propsRecvdEnd()) {
        // First element is always the ID
        unsigned id = XDR_decode_uint32(*xdr);

        /*
         * As we can detect a short int encoded value (by the upper word being non-zero) we can
         * do the decode here; set the id correctly, extract the integer and set the flag.
         * This can then be picked up by the normal processing based on the flag
         */
        int int_value = 0;
        bool short_int_encoded = false;
        if (id & 0xffff0000)
        {
            int v1, v2;
            XDR_decode_shortints32(*xdr, v1, v2);
            int_value = v2;
            id = v1;
            short_int_encoded = true;
        }

        if (pMultiPlayDebugLevel->getIntValue() & 8)
            SG_LOG(SG_NETWORK, SG_INFO,
                "[RECV] add " << std::hex << xdr
                << std::dec <<
                ": buf[" << ((char*)xdr) - ((char*)data)
                << "] id=" << id
                << " SIenc " << short_int_encoded);

        // Check the ID actually exists and get the type
        const IdPropertyList* plist = findProperty(id);
        xdr++;
    
    if (plist)
    {
      FGPropertyData* pData = new FGPropertyData;
      pData->id = id;
      pData->type = plist->type;
      // How we decode the remainder of the property depends on the type
      switch (pData->type) {
        case simgear::props::INT:
        case simgear::props::BOOL:
        case simgear::props::LONG:
                if (short_int_encoded)
                {
                    pData->int_value = int_value;
                    pData->type = simgear::props::INT;
                }
                else
                {
          pData->int_value = XDR_decode_uint32(*xdr);
          xdr++;
                }
          //cout << pData->int_value << "\n";
          break;
        case simgear::props::FLOAT:
        case simgear::props::DOUBLE:
                if (short_int_encoded)
                {
                    switch (plist->TransmitAs)
                    {
                    case TT_SHORT_FLOAT_1:
                        pData->float_value = (double)int_value / 10.0;
                        break;
                    case TT_SHORT_FLOAT_2:
                        pData->float_value = (double)int_value / 100.0;
                        break;
                    case TT_SHORT_FLOAT_3:
                        pData->float_value = (double)int_value / 1000.0;
                        break;
                    case TT_SHORT_FLOAT_4:
                        pData->float_value = (double)int_value / 10000.0;
                        break;
                    case TT_SHORT_FLOAT_NORM:
                        pData->float_value = (double)int_value / 32767.0;
                        break;
                    default:
                        break;
                    }
                }
                else
                {
          pData->float_value = XDR_decode_float(*xdr);
                }
          xdr++;
          break;
        case simgear::props::STRING:
        case simgear::props::UNSPECIFIED:
          {
                // if the string is using short int encoding then it is in the new format.
                if (short_int_encoded)
                {
                    uint32_t length = int_value;
                    pData->string_value = new char[length + 1];

                    char *cptr = (char*)xdr;
                    for (unsigned i = 0; i < length; i++)
                    {
                        pData->string_value[i] = *cptr++;
                    }
                    pData->string_value[length] = '\0';
                    xdr = (xdr_data_t*)cptr;
                }
                else {
            // String is complicated. It consists of
            // The length of the string
            // The string itself
            // Padding to the nearest 4-bytes.    
            uint32_t length = XDR_decode_uint32(*xdr);
            xdr++;
            //cout << length << " ";
            // Old versions truncated the string but left the length unadjusted.
            if (length > MAX_TEXT_SIZE)
              length = MAX_TEXT_SIZE;
            pData->string_value = new char[length + 1];
            //cout << " String: ";
            for (unsigned i = 0; i < length; i++)
              {
                pData->string_value[i] = (char) XDR_decode_int8(*xdr);
                xdr++;
              }

            pData->string_value[length] = '\0';

            // Now handle the padding
            while ((length % 4) != 0)
              {
                xdr++;
                length++;
                //cout << "0";
              }
            //cout << "\n";
          }
            }
          break;

        default:
          pData->float_value = XDR_decode_float(*xdr);
          SG_LOG(SG_NETWORK, SG_DEBUG, "Unknown Prop type " << pData->id << " " << pData->type);
          xdr++;
          break;
      }

      motionInfo.properties.push_back(pData);
    }
    else
    {
      // We failed to find the property. We'll try the next packet immediately.
      SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::ProcessPosMsg - "
             "message from " << MsgHdr->Callsign << " has unknown property id "
             << id); 

      // At this point the packet must be considered to be unreadable 
      // as we have no way of knowing the length of this property (it could be a string)
      break; 
    }
  }
 noprops:
  FGAIMultiplayer* mp = getMultiplayer(MsgHdr->Callsign);
  if (!mp)
    mp = addMultiplayer(MsgHdr->Callsign, PosMsg->Model);
  mp->addMotionInfo(motionInfo, stamp);
} // FGMultiplayMgr::ProcessPosMsg()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  handle a chat message
//  FIXME: display chat message within flightgear
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::ProcessChatMsg(const MsgBuf& Msg,
                               const simgear::IPAddress& SenderAddress)
{
  const T_MsgHdr* MsgHdr = Msg.msgHdr();
  if (MsgHdr->MsgLen < sizeof(T_MsgHdr) + 1) {
    SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
            << "Chat message received with insufficient data" );
    return;
  }
  
  char *chatStr = new char[MsgHdr->MsgLen - sizeof(T_MsgHdr)];
  const T_ChatMsg* ChatMsg
      = reinterpret_cast<const T_ChatMsg *>(Msg.Msg + sizeof(T_MsgHdr));
  strncpy(chatStr, ChatMsg->Text,
          MsgHdr->MsgLen - sizeof(T_MsgHdr));
  chatStr[MsgHdr->MsgLen - sizeof(T_MsgHdr) - 1] = '\0';
  
  SG_LOG (SG_NETWORK, SG_WARN, "Chat [" << MsgHdr->Callsign << "]"
           << " " << chatStr);

  delete [] chatStr;
} // FGMultiplayMgr::ProcessChatMsg ()
//////////////////////////////////////////////////////////////////////

void
FGMultiplayMgr::FillMsgHdr(T_MsgHdr *MsgHdr, int MsgId, unsigned _len)
{
  uint32_t len;
  switch (MsgId) {
  case CHAT_MSG_ID:
    len = sizeof(T_MsgHdr) + sizeof(T_ChatMsg);
    break;
  case POS_DATA_ID:
    len = _len;
    break;
  default:
    len = sizeof(T_MsgHdr);
    break;
  }
  MsgHdr->Magic            = XDR_encode_uint32(MSG_MAGIC);
  MsgHdr->Version          = XDR_encode_uint32(PROTO_VER);
  MsgHdr->MsgId            = XDR_encode_uint32(MsgId);
  MsgHdr->MsgLen           = XDR_encode_uint32(len);
  MsgHdr->RequestedRangeNm = XDR_encode_shortints32(0,pMultiPlayRange->getIntValue());
  MsgHdr->ReplyPort        = 0;
  strncpy(MsgHdr->Callsign, mCallsign.c_str(), MAX_CALLSIGN_LEN);
  MsgHdr->Callsign[MAX_CALLSIGN_LEN - 1] = '\0';
}

FGAIMultiplayer*
FGMultiplayMgr::addMultiplayer(const std::string& callsign,
                               const std::string& modelName)
{
  if (0 < mMultiPlayerMap.count(callsign))
    return mMultiPlayerMap[callsign].get();

  FGAIMultiplayer* mp = new FGAIMultiplayer;
  mp->setPath(modelName.c_str());
  mp->setCallSign(callsign);
  mMultiPlayerMap[callsign] = mp;

  FGAIManager *aiMgr = (FGAIManager*)globals->get_subsystem("ai-model");
  if (aiMgr) {
    aiMgr->attach(mp);

    /// FIXME: that must follow the attach ATM ...
    for (unsigned i = 0; i < numProperties; ++i)
      mp->addPropertyId(sIdPropertyList[i].id, sIdPropertyList[i].name);
  }

  return mp;
}

FGAIMultiplayer*
FGMultiplayMgr::getMultiplayer(const std::string& callsign)
{
  if (0 < mMultiPlayerMap.count(callsign))
    return mMultiPlayerMap[callsign].get();
  else
    return 0;
}

void
FGMultiplayMgr::findProperties()
{
  if (!mPropertiesChanged) {
    return;
  }
  
  mPropertiesChanged = false;
  
  for (unsigned i = 0; i < numProperties; ++i) {
      const char* name = sIdPropertyList[i].name;
      SGPropertyNode* pNode = globals->get_props()->getNode(name);
      if (!pNode) {
        continue;
      }
      
      int id = sIdPropertyList[i].id;
      if (mPropertyMap.find(id) != mPropertyMap.end()) {
        continue; // already activated
      }
      
      mPropertyMap[id] = pNode;
      mPropertyDefinition[id] = &sIdPropertyList[i];
      SG_LOG(SG_NETWORK, SG_DEBUG, "activating MP property:" << pNode->getPath());
    }
}
