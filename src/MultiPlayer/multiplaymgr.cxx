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

enum TransmissionType {
	TT_BOOL,
	TT_BOOLARRAY,
	TT_CHAR,
	TT_INT,
	TT_FLOAT,
	TT_STRING,
};


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
	{ 100, "surface-positions/left-aileron-pos-norm",  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 101, "surface-positions/right-aileron-pos-norm", simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 102, "surface-positions/elevator-pos-norm",      simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 103, "surface-positions/rudder-pos-norm",        simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 104, "surface-positions/flap-pos-norm",          simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 105, "surface-positions/speedbrake-pos-norm",    simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 106, "gear/tailhook/position-norm",              simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 107, "gear/launchbar/position-norm",             simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	//  {108, "gear/launchbar/state",                     simgear::props::STRING, TT_STRING,  1, NULL},
	{ 109, "gear/launchbar/holdback-position-norm",    simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 110, "canopy/position-norm",                     simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 111, "surface-positions/wing-pos-norm",          simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 112, "surface-positions/wing-fold-pos-norm",     simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 113, "gear/launchbar/state",                     simgear::props::STRING, TT_STRING,  2, convert_launchbar_state},

	{ 200, "gear/gear[0]/compression-norm",           simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 201, "gear/gear[0]/position-norm",              simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 210, "gear/gear[1]/compression-norm",           simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 211, "gear/gear[1]/position-norm",              simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 220, "gear/gear[2]/compression-norm",           simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 221, "gear/gear[2]/position-norm",              simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 230, "gear/gear[3]/compression-norm",           simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 231, "gear/gear[3]/position-norm",              simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 240, "gear/gear[4]/compression-norm",           simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 241, "gear/gear[4]/position-norm",              simgear::props::FLOAT, TT_FLOAT,  1, NULL },

	{ 300, "engines/engine[0]/n1",  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 301, "engines/engine[0]/n2",  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 302, "engines/engine[0]/rpm", simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 310, "engines/engine[1]/n1",  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 311, "engines/engine[1]/n2",  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 312, "engines/engine[1]/rpm", simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 320, "engines/engine[2]/n1",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 321, "engines/engine[2]/n2",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 322, "engines/engine[2]/rpm", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 330, "engines/engine[3]/n1",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 331, "engines/engine[3]/n2",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 332, "engines/engine[3]/rpm", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 340, "engines/engine[4]/n1",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 341, "engines/engine[4]/n2",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 342, "engines/engine[4]/rpm", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 350, "engines/engine[5]/n1",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 351, "engines/engine[5]/n2",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 352, "engines/engine[5]/rpm", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 360, "engines/engine[6]/n1",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 361, "engines/engine[6]/n2",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 362, "engines/engine[6]/rpm", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 370, "engines/engine[7]/n1",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 371, "engines/engine[7]/n2",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 372, "engines/engine[7]/rpm", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 380, "engines/engine[8]/n1",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 381, "engines/engine[8]/n2",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 382, "engines/engine[8]/rpm", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 390, "engines/engine[9]/n1",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 391, "engines/engine[9]/n2",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 392, "engines/engine[9]/rpm", simgear::props::FLOAT, TT_FLOAT,  2, NULL },

	{ 800, "rotors/main/rpm", simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 801, "rotors/tail/rpm", simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 810, "rotors/main/blade[0]/position-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 811, "rotors/main/blade[1]/position-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 812, "rotors/main/blade[2]/position-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 813, "rotors/main/blade[3]/position-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 820, "rotors/main/blade[0]/flap-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 821, "rotors/main/blade[1]/flap-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 822, "rotors/main/blade[2]/flap-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 823, "rotors/main/blade[3]/flap-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 830, "rotors/tail/blade[0]/position-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 831, "rotors/tail/blade[1]/position-deg",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },

	{ 900, "sim/hitches/aerotow/tow/length",                       simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 901, "sim/hitches/aerotow/tow/elastic-constant",             simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 902, "sim/hitches/aerotow/tow/weight-per-m-kg-m",            simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 903, "sim/hitches/aerotow/tow/dist",                         simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 904, "sim/hitches/aerotow/tow/connected-to-property-node",   simgear::props::BOOL, TT_BOOL,  1, NULL },
	{ 905, "sim/hitches/aerotow/tow/connected-to-ai-or-mp-callsign",   simgear::props::STRING, TT_STRING,  1, NULL },
	{ 906, "sim/hitches/aerotow/tow/brake-force",                  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 907, "sim/hitches/aerotow/tow/end-force-x",                  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 908, "sim/hitches/aerotow/tow/end-force-y",                  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 909, "sim/hitches/aerotow/tow/end-force-z",                  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 930, "sim/hitches/aerotow/is-slave",                         simgear::props::BOOL, TT_BOOL,  1, NULL },
	{ 931, "sim/hitches/aerotow/speed-in-tow-direction",           simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 932, "sim/hitches/aerotow/open",                             simgear::props::BOOL, TT_BOOL,  1, NULL },
	{ 933, "sim/hitches/aerotow/local-pos-x",                      simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 934, "sim/hitches/aerotow/local-pos-y",                      simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 935, "sim/hitches/aerotow/local-pos-z",                      simgear::props::FLOAT, TT_FLOAT,  1, NULL },

	{ 1001, "controls/flight/slats",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 1002, "controls/flight/speedbrake",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 1003, "controls/flight/spoilers",  simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 1004, "controls/gear/gear-down",  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 1005, "controls/lighting/nav-lights",  simgear::props::FLOAT, TT_FLOAT,  1, NULL },
	{ 1006, "controls/armament/station[0]/jettison-all",  simgear::props::BOOL, TT_BOOL,  2, NULL },

	{ 1100, "sim/model/variant", simgear::props::INT, TT_INT,  2, NULL },
	{ 1101, "sim/model/livery/file", simgear::props::STRING, TT_STRING,  2, NULL },

	{ 1200, "environment/wildfire/data", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 1201, "environment/contrail", simgear::props::INT, TT_INT,  2, NULL },

	{ 1300, "tanker", simgear::props::INT, TT_INT,  1, NULL },

	{ 1400, "scenery/events", simgear::props::STRING, TT_STRING,  2, NULL },

	{ 1500, "instrumentation/transponder/transmitted-id", simgear::props::INT, TT_INT,  1, NULL },
	{ 1501, "instrumentation/transponder/altitude", simgear::props::INT, TT_INT,  1, NULL },
	{ 1502, "instrumentation/transponder/ident", simgear::props::BOOL, TT_BOOL,  1, NULL },
	{ 1503, "instrumentation/transponder/inputs/mode", simgear::props::INT, TT_INT,  1, NULL },

	{ 10001, "sim/multiplay/transmission-freq-hz",  simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10002, "sim/multiplay/chat",  simgear::props::STRING, TT_STRING,  1, NULL },

	{ 10100, "sim/multiplay/generic/string[0]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10101, "sim/multiplay/generic/string[1]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10102, "sim/multiplay/generic/string[2]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10103, "sim/multiplay/generic/string[3]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10104, "sim/multiplay/generic/string[4]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10105, "sim/multiplay/generic/string[5]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10106, "sim/multiplay/generic/string[6]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10107, "sim/multiplay/generic/string[7]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10108, "sim/multiplay/generic/string[8]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10109, "sim/multiplay/generic/string[9]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10110, "sim/multiplay/generic/string[10]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10111, "sim/multiplay/generic/string[11]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10112, "sim/multiplay/generic/string[12]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10113, "sim/multiplay/generic/string[13]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10114, "sim/multiplay/generic/string[14]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10115, "sim/multiplay/generic/string[15]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10116, "sim/multiplay/generic/string[16]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10117, "sim/multiplay/generic/string[17]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10118, "sim/multiplay/generic/string[18]", simgear::props::STRING, TT_STRING,  2, NULL },
	{ 10119, "sim/multiplay/generic/string[19]", simgear::props::STRING, TT_STRING,  2, NULL },

	{ 10200, "sim/multiplay/generic/float[0]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10201, "sim/multiplay/generic/float[1]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10202, "sim/multiplay/generic/float[2]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10203, "sim/multiplay/generic/float[3]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10204, "sim/multiplay/generic/float[4]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10205, "sim/multiplay/generic/float[5]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10206, "sim/multiplay/generic/float[6]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10207, "sim/multiplay/generic/float[7]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10208, "sim/multiplay/generic/float[8]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10209, "sim/multiplay/generic/float[9]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10210, "sim/multiplay/generic/float[10]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10211, "sim/multiplay/generic/float[11]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10212, "sim/multiplay/generic/float[12]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10213, "sim/multiplay/generic/float[13]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10214, "sim/multiplay/generic/float[14]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10215, "sim/multiplay/generic/float[15]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10216, "sim/multiplay/generic/float[16]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10217, "sim/multiplay/generic/float[17]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10218, "sim/multiplay/generic/float[18]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },
	{ 10219, "sim/multiplay/generic/float[19]", simgear::props::FLOAT, TT_FLOAT,  2, NULL },

	{ 10220, "sim/multiplay/generic/float[20]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10221, "sim/multiplay/generic/float[21]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10222, "sim/multiplay/generic/float[22]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10223, "sim/multiplay/generic/float[23]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10224, "sim/multiplay/generic/float[24]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10225, "sim/multiplay/generic/float[25]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10226, "sim/multiplay/generic/float[26]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10227, "sim/multiplay/generic/float[27]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10228, "sim/multiplay/generic/float[28]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10229, "sim/multiplay/generic/float[29]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10230, "sim/multiplay/generic/float[30]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10231, "sim/multiplay/generic/float[31]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10232, "sim/multiplay/generic/float[32]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10233, "sim/multiplay/generic/float[33]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10234, "sim/multiplay/generic/float[34]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10235, "sim/multiplay/generic/float[35]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10236, "sim/multiplay/generic/float[36]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10237, "sim/multiplay/generic/float[37]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10238, "sim/multiplay/generic/float[38]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },
	{ 10239, "sim/multiplay/generic/float[39]", simgear::props::FLOAT, TT_FLOAT ,  2, NULL },

	{ 10300, "sim/multiplay/generic/int[0]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10301, "sim/multiplay/generic/int[1]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10302, "sim/multiplay/generic/int[2]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10303, "sim/multiplay/generic/int[3]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10304, "sim/multiplay/generic/int[4]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10305, "sim/multiplay/generic/int[5]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10306, "sim/multiplay/generic/int[6]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10307, "sim/multiplay/generic/int[7]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10308, "sim/multiplay/generic/int[8]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10309, "sim/multiplay/generic/int[9]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10310, "sim/multiplay/generic/int[10]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10311, "sim/multiplay/generic/int[11]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10312, "sim/multiplay/generic/int[12]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10313, "sim/multiplay/generic/int[13]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10314, "sim/multiplay/generic/int[14]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10315, "sim/multiplay/generic/int[15]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10316, "sim/multiplay/generic/int[16]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10317, "sim/multiplay/generic/int[17]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10318, "sim/multiplay/generic/int[18]", simgear::props::INT, TT_INT,  2, NULL },
	{ 10319, "sim/multiplay/generic/int[19]", simgear::props::INT, TT_INT,  2, NULL },

	{ 10320, "sim/multiplay/generic/int[20]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10321, "sim/multiplay/generic/int[21]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10322, "sim/multiplay/generic/int[22]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10323, "sim/multiplay/generic/int[23]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10324, "sim/multiplay/generic/int[24]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10325, "sim/multiplay/generic/int[25]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10326, "sim/multiplay/generic/int[26]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10327, "sim/multiplay/generic/int[27]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10328, "sim/multiplay/generic/int[28]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10329, "sim/multiplay/generic/int[29]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10320, "sim/multiplay/generic/int[20]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10321, "sim/multiplay/generic/int[21]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10322, "sim/multiplay/generic/int[22]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10323, "sim/multiplay/generic/int[23]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10324, "sim/multiplay/generic/int[24]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10325, "sim/multiplay/generic/int[25]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10326, "sim/multiplay/generic/int[26]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10327, "sim/multiplay/generic/int[27]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10328, "sim/multiplay/generic/int[28]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10329, "sim/multiplay/generic/int[29]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10330, "sim/multiplay/generic/int[30]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10331, "sim/multiplay/generic/int[31]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10332, "sim/multiplay/generic/int[32]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10333, "sim/multiplay/generic/int[33]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10334, "sim/multiplay/generic/int[34]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10335, "sim/multiplay/generic/int[35]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10336, "sim/multiplay/generic/int[36]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10337, "sim/multiplay/generic/int[37]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10338, "sim/multiplay/generic/int[38]", simgear::props::INT, TT_INT ,  2, NULL },
	{ 10339, "sim/multiplay/generic/int[39]", simgear::props::INT, TT_INT ,  2, NULL },


	{ 12100, "sim/multiplay/generic/string[20]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12101, "sim/multiplay/generic/string[21]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12102, "sim/multiplay/generic/string[22]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12103, "sim/multiplay/generic/string[23]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12104, "sim/multiplay/generic/string[24]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12105, "sim/multiplay/generic/string[25]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12106, "sim/multiplay/generic/string[26]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12107, "sim/multiplay/generic/string[27]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12108, "sim/multiplay/generic/string[28]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12109, "sim/multiplay/generic/string[29]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12110, "sim/multiplay/generic/string[20]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12111, "sim/multiplay/generic/string[31]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12112, "sim/multiplay/generic/string[32]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12113, "sim/multiplay/generic/string[33]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12114, "sim/multiplay/generic/string[34]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12115, "sim/multiplay/generic/string[35]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12116, "sim/multiplay/generic/string[36]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12117, "sim/multiplay/generic/string[37]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12118, "sim/multiplay/generic/string[38]", simgear::props::STRING, TT_STRING ,  2, NULL },
	{ 12119, "sim/multiplay/generic/string[39]", simgear::props::STRING, TT_STRING ,  2, NULL },
};
const int MAX_PARTITIONS = 2;
const int NEW_STRING_ENCODING_START = 12000; // anything below this uses the old string encoding scheme
const unsigned int numProperties = (sizeof(sIdPropertyList)
                                 / sizeof(sIdPropertyList[0]));

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
//		  printf("[VRFY] %8x: buf[%d] type %d\n", xdr, ((char*)xdr) - ((char*)data), plist->id);
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
			if (id >= NEW_STRING_ENCODING_START) {
				uint32_t length = XDR_decode_uint32(*xdr);
				xdr++;
				xdr = (xdr_data_t*)(((char*)xdr) + length);
			}
			else {
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
#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif

void hexdump(void *mem, unsigned int len)
{
	unsigned int i, j;

	for (i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
	{
		/* print offset */
		if (i % HEXDUMP_COLS == 0)
		{
			printf("0x%06x: ", i);
		}

		/* print hex data */
		if (i < len)
		{
			printf("%02x ", 0xFF & ((char*)mem)[i]);
		}
		else /* end of block, just aligning for ASCII dump */
		{
			printf("   ");
		}

		/* print ASCII dump */
		if (i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
		{
			for (j = i - (HEXDUMP_COLS - 1); j <= i; j++)
			{
				if (j >= len) /* end of block, not really printing */
				{
					putchar(' ');
				}
				else if (isprint(((char*)mem)[j])) /* printable char */
				{
					putchar(0xFF & ((char*)mem)[j]);
				}
				else /* other char */
				{
					putchar('.');
				}
			}
			putchar('\n');
		}
	}
}

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
		return reinterpret_cast<xdr_data_t*>(Msg + Header.MsgLen + Header.MsgLen2);
	}

    const xdr_data_t* propsRecvdEnd() const
    {
		return reinterpret_cast<const xdr_data_t*>(Msg + Header.MsgLen + Header.MsgLen2);
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
	  int previous_partitions_len = 0;
	  for (int partition = 1; partition <= MAX_PARTITIONS; partition++)
	  {
		  std::vector<FGPropertyData*>::const_iterator it = motionInfo.properties.begin();
		  while (it != motionInfo.properties.end()) {
			  const struct IdPropertyList* propDef = mPropertyDefinition[(*it)->id];
			  if (propDef->version == partition)
			  {
				  if (ptr + 2 >= msgEnd)
				  {
					  SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer packet truncated prop id: " << (*it)->id);
					  break;
				  }
//				  printf("[SEND] %8x: buf[%d] type %d : p%d TA:%d\n", ptr, ((unsigned int)ptr) - ((unsigned int)data), (*it)->id, propDef->version, propDef->TransmitAs);

				  // First element is the ID. Write it out when we know we have room for
				  // the whole property.
				  xdr_data_t id = XDR_encode_uint32((*it)->id);
				  // The actual data representation depends on the type
				  switch ((*it)->type) {
				  case simgear::props::INT:
				  case simgear::props::BOOL:
				  case simgear::props::LONG:
					  *ptr++ = id;
					  *ptr++ = XDR_encode_uint32((*it)->int_value);
					  //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->int_value << "\n";
					  break;
				  case simgear::props::FLOAT:
				  case simgear::props::DOUBLE:
					  *ptr++ = id;
					  *ptr++ = XDR_encode_float((*it)->float_value);
					  //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->float_value << "\n";
					  break;
				  case simgear::props::STRING:
				  case simgear::props::UNSPECIFIED:
				  {
					  if ((*it)->id >= NEW_STRING_ENCODING_START) {
						  // New string encoding:
						  // The length of the string (int32)
						  // The string itself (char[length])
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

							  *ptr++ = id;
							  *ptr++ = XDR_encode_uint32(len);
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
								  //cout << "Prop:" << (*it)->id << " " << (*it)->type << " " << len << " " << (*it)->string_value;

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
									  //cout << "0";
								  }

								  //cout << "\n";
							  }
						  }
						  else
						  {
							  // Nothing to encode
							  *ptr++ = id;
							  *ptr++ = XDR_encode_uint32(0);
							  //cout << "Prop:" << (*it)->id << " " << (*it)->type << " 0\n";
						  }
					  }
				  }
				  break;

				  default:
					  //cout << " Unknown Type: " << (*it)->type << "\n";
					  *ptr++ = id;
					  *ptr++ = XDR_encode_float((*it)->float_value);;
					  //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->float_value << "\n";
					  break;
				  }
			  }
			  ++it;
		  }
	  escape:
		  msgLen = reinterpret_cast<char*>(ptr) - msgBuf.Msg;
		  /*
		   * 2017.x MP changes:
		   * The buffer is partitioned into two logical sections; the first contains the backwards compatible section, which
		   * is only to be read by older clients.
		   * The second partition (in the buf) contains all of the rest of the data for all other versions
		   */
		  if (partition == 1)
		  {
			  FillMsgHdr(msgBuf.msgHdr(), POS_DATA_ID, msgLen);
			  previous_partitions_len += msgLen;
		  }
		  else
		  {
			  int ml2 = msgLen - previous_partitions_len;
			  //previous_partitions_len += ml2;
			  msgBuf.msgHdr()->MsgLen2 = XDR_encode_int32(ml2);
		  }
		  //printf("[SEND] %d: %8x: buf[%d] p1l %4d p2 %4d\n", partition, ptr, ((unsigned int)ptr) - ((unsigned int)data), 
			 //XDR_decode_int32(msgBuf.msgHdr()->MsgLen), 
			 // XDR_decode_int32(msgBuf.msgHdr()->MsgLen2));
		  //hexdump(data, (ptr - data) * sizeof(*ptr));
		  //	  long stamp = SGTimeStamp::now().getSeconds();
		  //	  ProcessPosMsg(msgBuf, mServer, stamp);
	  }
  }
  if (msgLen>0)
      mSocket->sendto(msgBuf.Msg, msgLen, 0, &mServer);
  SG_LOG(SG_NETWORK, SG_BULK, "FGMultiplayMgr::SendMyPosition");
} // FGMultiplayMgr::SendMyPosition()

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
    SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
            << "Position message received with insufficient data" );
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
  if (PosMsg->pad != 0) {
    if (verifyProperties(&PosMsg->pad, Msg.propsRecvdEnd()))
      xdr = &PosMsg->pad;
    else if (!verifyProperties(xdr, Msg.propsRecvdEnd()))
      goto noprops;
  }
  while (xdr < Msg.propsRecvdEnd()) {
    // simgear::props::Type type = simgear::props::UNSPECIFIED;
    
    // First element is always the ID
    unsigned id = XDR_decode_uint32(*xdr);
    //cout << pData->id << " ";
//	printf("[RECV] %8x: buf[%d] type %d\n", xdr, ((char*)xdr) - ((char*)data), id);
	xdr++;
    
    // Check the ID actually exists and get the type
    const IdPropertyList* plist = findProperty(id);
    
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
          pData->int_value = XDR_decode_uint32(*xdr);
          xdr++;
          //cout << pData->int_value << "\n";
          break;
        case simgear::props::FLOAT:
        case simgear::props::DOUBLE:
          pData->float_value = XDR_decode_float(*xdr);
          xdr++;
          //cout << pData->float_value << "\n";
          break;
        case simgear::props::STRING:
        case simgear::props::UNSPECIFIED:
		{
			if (pData->id >= NEW_STRING_ENCODING_START) {
				uint32_t length = XDR_decode_uint32(*xdr);
				xdr++;
				pData->string_value = new char[length + 1];

				char *cptr = (char*)xdr;
				for (unsigned i = 0; i < length; i++)
				{
					pData->string_value[i] = *cptr++;
				}
				pData->string_value[length] = '\0';
//				printf(">> string recv %s\n", pData->string_value);
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
					pData->string_value[i] = (char)XDR_decode_int8(*xdr);
					xdr++;
					//cout << pData->string_value[i];
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
	  break; // C     TESTING ONLY AS PROBABLY INDICATES BAD PACKET.
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
  MsgHdr->Magic           = XDR_encode_uint32(MSG_MAGIC);
  MsgHdr->Version         = XDR_encode_uint32(PROTO_VER);
  MsgHdr->MsgId           = XDR_encode_uint32(MsgId);
  MsgHdr->MsgLen          = XDR_encode_uint32(len);
  MsgHdr->MsgLen2         = 0; // Used to be ReplyAddress; repurposed with MP2017.x changes
  MsgHdr->ReplyPort       = 0; // now
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
