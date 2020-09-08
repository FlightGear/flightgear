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

#include <config.h>

#include <iostream>
#include <algorithm>
#include <cstring>
#include <errno.h>

#include <simgear/misc/stdint.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/misc/strutils.hxx>

#include <AIModel/AIManager.hxx>
#include <AIModel/AIMultiplayer.hxx>
#include <Main/fg_props.hxx>
#include "multiplaymgr.hxx"
#include "mpmessages.hxx"
#include "MPServerResolver.hxx"
#include <FDM/flightProperties.hxx>
#include <Time/TimeManager.hxx>
#include <Main/sentryIntegration.hxx>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <WS2tcpip.h>
#endif
using namespace std;

#define MAX_PACKET_SIZE 1200
#define MAX_TEXT_SIZE 768 // Increased for 2017.3 to allow for long Emesary messages.
/*
 * With the MP2017(V2) protocol it should be possible to transmit using a different type/encoding than the property has,
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
    TT_NOSEND, // Do not send this property - probably the receive element for a custom encoded property
};
/*
 * Definitions for the version of the protocol to use to transmit the items defined in the IdPropertyList
 *
 * The MP2017(V2) protocol allows for much better packing of strings, new types that are transmitted in 4bytes by transmitting
 * with short int (sometimes scaled) for the values (a lot of the properties that are transmitted will pack nicely into 16bits).
 * The MP2017(V2) protocol also allows for properties to be transmitted automatically as a different type and the encode/decode will
 * take this into consideration.
 * The pad magic is used to force older clients to use verifyProperties and as the first property transmitted is short int encoded it
 * will cause the rest of the packet to be discarded. This is the section of the packet that contains the properties defined in the list
 * here - the basic motion properties remain compatible, so the older client will see just the model, not chat, not animations etc.
 * The lower 16 bits of the prop_id (version) are the version and the upper 16bits are for meta data.
 */
const int V1_1_PROP_ID = 1;
const int V1_1_2_PROP_ID = 2;
const int V2_PROP_ID_PROTOCOL = 0x10001;

const int V2_PAD_MAGIC = 0x1face002;

/*
 * boolean arrays are transmitted in blocks of 31 mapping to a single int.
 * These parameters define where these are mapped and how they are sent.
 * The blocks should be in the same property range (with no other properties inside the range)
 * The blocksize is set to 40 to allow for ints being 32 bits, so block 0 will be 0..30 (31 bits)
 */
const int BOOLARRAY_BLOCKSIZE = 40;

const int BOOLARRAY_BASE_1 = 11000;
const int BOOLARRAY_BASE_2 = BOOLARRAY_BASE_1 + BOOLARRAY_BLOCKSIZE;
const int BOOLARRAY_BASE_3 = BOOLARRAY_BASE_2 + BOOLARRAY_BLOCKSIZE;
// define range of bool values for receive.
const int BOOLARRAY_START_ID = BOOLARRAY_BASE_1;
const int BOOLARRAY_END_ID = BOOLARRAY_BASE_3;
// Transmission uses a buffer to build the value for each array block.
const int MAX_BOOL_BUFFERS = 3;

// starting with 2018.1 we will now append new properties for each version at the end of the list - as this provides much
// better backwards compatibility.
const int V2018_1_BASE = 11990;
const int EMESARYBRIDGETYPE_BASE = 12200;
const int EMESARYBRIDGE_BASE = 12000;
const int V2018_3_BASE = 13000;
const int FALLBACK_MODEL_ID = 13000;
const int V2019_3_BASE = 13001;

/*
 * definition of properties that are to be transmitted.
 * New for 2017.2:
 * 1. TransmitAs - this causes the property to be transmitted on the wire using the
 *    specified format transparently.
 * 2. version - the minimum version of the protocol that is required to transmit a property.
 *    Does not apply to incoming properties - as these will be decoded correctly when received
 * 3. encode_for_transmit  - method that will convert from and to the packet for the value
 *    Allows specific conversion rules to be applied; such as conversion of a string to an integer for transmission.
 * 4. decode_received - decodes received data
 * - when using the encode/decode methods there should be both specified, however if the result of the encode
 *   is to transmit in a different property index the encode/decode will be on different elements in the property id list.
 *   This is used to encode/decode the launchbar state - so that with 2017.2 instead of the string being transmitted in property 108
 *   a short int encoded version is sent in property 120 - which when received will be placed into property 108. This reduces transmission space
 *   and keeps compatibility.
 */
struct IdPropertyList {
    unsigned id;
    const char* name;
    simgear::props::Type type;
    TransmissionType TransmitAs;
    int version;
    xdr_data_t* (*encode_for_transmit)(const IdPropertyList *propDef, const xdr_data_t*, FGPropertyData*);
    xdr_data_t* (*decode_received)(const IdPropertyList *propDef, const xdr_data_t*, FGPropertyData*);
};

static const IdPropertyList* findProperty(unsigned id);

/*
 * intermediate buffer used to build the ints that will be transmitted for the boolean arrays
 */
struct BoolArrayBuffer {
    int propertyId;
    int boolValue;
};

static xdr_data_t *encode_launchbar_state_for_transmission(const IdPropertyList *propDef, const xdr_data_t *_xdr, FGPropertyData*p)
{
    xdr_data_t *xdr = (xdr_data_t *)_xdr;

    if (propDef->TransmitAs == TT_NOSEND)
        return xdr;

    int v = -1;
    if (p && p->string_value)
    {
        if (!strcmp("Engaged", p->string_value))
            v = 0;
        else if (!strcmp("Launching", p->string_value))
            v = 1;
        else if (!strcmp("Completed", p->string_value))
            v = 2;
        else if (!strcmp("Disengaged", p->string_value))
            v = 3;
        else
            return (xdr_data_t*)xdr;

        *xdr++ = XDR_encode_shortints32(120, v);
    }
    return xdr;
}

static xdr_data_t *decode_received_launchbar_state(const IdPropertyList *propDef, const xdr_data_t *_xdr, FGPropertyData*p)
{
    xdr_data_t *xdr = (xdr_data_t *)_xdr;

    int v1, v2;
    XDR_decode_shortints32(*xdr, v1, v2);
    xdr++;
    const char *stringvalue = "";
    switch (v2)
    {
    case 0: stringvalue = "Engaged"; break;
    case 1: stringvalue = "Launching"; break;
    case 2: stringvalue = "Completed"; break;
    case 3: stringvalue = "Disengaged"; break;
    }

    p->id = 108; // this is for the string property for gear/launchbar/state
    if (p->string_value && p->type == simgear::props::STRING)
        delete[] p->string_value;
    p->string_value = new char[strlen(stringvalue) + 1];
    strcpy(p->string_value, stringvalue);
    p->type = simgear::props::STRING;
    return xdr;
}

// A static map of protocol property id values to property paths,
// This should be extendable dynamically for every specific aircraft ...
// For now only that static list
static const IdPropertyList sIdPropertyList[] = {
    { 10,  "sim/multiplay/protocol-version",          simgear::props::INT,   TT_SHORTINT,  V2_PROP_ID_PROTOCOL, NULL, NULL },
    { 100, "surface-positions/left-aileron-pos-norm",  simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 101, "surface-positions/right-aileron-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 102, "surface-positions/elevator-pos-norm",      simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 103, "surface-positions/rudder-pos-norm",        simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 104, "surface-positions/flap-pos-norm",          simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 105, "surface-positions/speedbrake-pos-norm",    simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 106, "gear/tailhook/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 107, "gear/launchbar/position-norm",             simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    //
    { 108, "gear/launchbar/state",                     simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, encode_launchbar_state_for_transmission, NULL },
    { 109, "gear/launchbar/holdback-position-norm",    simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 110, "canopy/position-norm",                     simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 111, "surface-positions/wing-pos-norm",          simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 112, "surface-positions/wing-fold-pos-norm",     simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },

    // to enable decoding this is the transient ID record that is in the packet. This is not sent directly - instead it is the result
    // of the conversion of property 108.
    { 120, "gear/launchbar/state-value",               simgear::props::INT, TT_NOSEND,  V1_1_2_PROP_ID, NULL, decode_received_launchbar_state },

    { 200, "gear/gear[0]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 201, "gear/gear[0]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 210, "gear/gear[1]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 211, "gear/gear[1]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 220, "gear/gear[2]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 221, "gear/gear[2]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 230, "gear/gear[3]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 231, "gear/gear[3]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 240, "gear/gear[4]/compression-norm",           simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },
    { 241, "gear/gear[4]/position-norm",              simgear::props::FLOAT, TT_SHORT_FLOAT_NORM,  V1_1_PROP_ID, NULL, NULL },

    { 300, "engines/engine[0]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 301, "engines/engine[0]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 302, "engines/engine[0]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 310, "engines/engine[1]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 311, "engines/engine[1]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 312, "engines/engine[1]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 320, "engines/engine[2]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 321, "engines/engine[2]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 322, "engines/engine[2]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 330, "engines/engine[3]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 331, "engines/engine[3]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 332, "engines/engine[3]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 340, "engines/engine[4]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 341, "engines/engine[4]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 342, "engines/engine[4]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 350, "engines/engine[5]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 351, "engines/engine[5]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 352, "engines/engine[5]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 360, "engines/engine[6]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 361, "engines/engine[6]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 362, "engines/engine[6]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 370, "engines/engine[7]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 371, "engines/engine[7]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 372, "engines/engine[7]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 380, "engines/engine[8]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 381, "engines/engine[8]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 382, "engines/engine[8]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 390, "engines/engine[9]/n1",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 391, "engines/engine[9]/n2",  simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 392, "engines/engine[9]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },

    { 800, "rotors/main/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 801, "rotors/tail/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1,  V1_1_PROP_ID, NULL, NULL },
    { 810, "rotors/main/blade[0]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 811, "rotors/main/blade[1]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 812, "rotors/main/blade[2]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 813, "rotors/main/blade[3]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 820, "rotors/main/blade[0]/flap-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 821, "rotors/main/blade[1]/flap-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 822, "rotors/main/blade[2]/flap-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 823, "rotors/main/blade[3]/flap-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 830, "rotors/tail/blade[0]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 831, "rotors/tail/blade[1]/position-deg",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },

    { 900, "sim/hitches/aerotow/tow/length",                       simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 901, "sim/hitches/aerotow/tow/elastic-constant",             simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 902, "sim/hitches/aerotow/tow/weight-per-m-kg-m",            simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 903, "sim/hitches/aerotow/tow/dist",                         simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 904, "sim/hitches/aerotow/tow/connected-to-property-node",   simgear::props::BOOL, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 905, "sim/hitches/aerotow/tow/connected-to-ai-or-mp-callsign",   simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 906, "sim/hitches/aerotow/tow/brake-force",                  simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 907, "sim/hitches/aerotow/tow/end-force-x",                  simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 908, "sim/hitches/aerotow/tow/end-force-y",                  simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 909, "sim/hitches/aerotow/tow/end-force-z",                  simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 930, "sim/hitches/aerotow/is-slave",                         simgear::props::BOOL, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 931, "sim/hitches/aerotow/speed-in-tow-direction",           simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 932, "sim/hitches/aerotow/open",                             simgear::props::BOOL, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 933, "sim/hitches/aerotow/local-pos-x",                      simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 934, "sim/hitches/aerotow/local-pos-y",                      simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 935, "sim/hitches/aerotow/local-pos-z",                      simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },

    { 1001, "controls/flight/slats",  simgear::props::FLOAT, TT_SHORT_FLOAT_4,  V1_1_PROP_ID, NULL, NULL },
    { 1002, "controls/flight/speedbrake",  simgear::props::FLOAT, TT_SHORT_FLOAT_4,  V1_1_PROP_ID, NULL, NULL },
    { 1003, "controls/flight/spoilers",  simgear::props::FLOAT, TT_SHORT_FLOAT_4,  V1_1_PROP_ID, NULL, NULL },
    { 1004, "controls/gear/gear-down",  simgear::props::FLOAT, TT_SHORT_FLOAT_4,  V1_1_PROP_ID, NULL, NULL },
    { 1005, "controls/lighting/nav-lights",  simgear::props::FLOAT, TT_SHORT_FLOAT_3,  V1_1_PROP_ID, NULL, NULL },
    { 1006, "controls/armament/station[0]/jettison-all",  simgear::props::BOOL, TT_SHORTINT,  V1_1_PROP_ID, NULL, NULL },

    { 1100, "sim/model/variant", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 1101, "sim/model/livery/file", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },

    { 1200, "environment/wildfire/data", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 1201, "environment/contrail", simgear::props::INT, TT_SHORTINT,  V1_1_PROP_ID, NULL, NULL },

    { 1300, "tanker", simgear::props::INT, TT_SHORTINT,  V1_1_PROP_ID, NULL, NULL },

    { 1400, "scenery/events", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },

    { 1500, "instrumentation/transponder/transmitted-id", simgear::props::INT, TT_SHORTINT,  V1_1_PROP_ID, NULL, NULL },
    { 1501, "instrumentation/transponder/altitude", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL },
    { 1502, "instrumentation/transponder/ident", simgear::props::BOOL, TT_SHORTINT, V1_1_PROP_ID, NULL, NULL },
    { 1503, "instrumentation/transponder/inputs/mode", simgear::props::INT, TT_SHORTINT, V1_1_PROP_ID, NULL, NULL },
    { 1504, "instrumentation/transponder/ground-bit", simgear::props::BOOL, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL },
    { 1505, "instrumentation/transponder/airspeed-kt", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL },

    { 10001, "sim/multiplay/transmission-freq-hz",  simgear::props::STRING, TT_NOSEND,  V1_1_2_PROP_ID, NULL, NULL },
    { 10002, "sim/multiplay/chat",  simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },

    { 10100, "sim/multiplay/generic/string[0]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10101, "sim/multiplay/generic/string[1]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10102, "sim/multiplay/generic/string[2]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10103, "sim/multiplay/generic/string[3]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10104, "sim/multiplay/generic/string[4]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10105, "sim/multiplay/generic/string[5]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10106, "sim/multiplay/generic/string[6]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10107, "sim/multiplay/generic/string[7]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10108, "sim/multiplay/generic/string[8]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10109, "sim/multiplay/generic/string[9]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10110, "sim/multiplay/generic/string[10]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10111, "sim/multiplay/generic/string[11]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10112, "sim/multiplay/generic/string[12]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10113, "sim/multiplay/generic/string[13]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10114, "sim/multiplay/generic/string[14]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10115, "sim/multiplay/generic/string[15]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10116, "sim/multiplay/generic/string[16]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10117, "sim/multiplay/generic/string[17]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10118, "sim/multiplay/generic/string[18]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { 10119, "sim/multiplay/generic/string[19]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },

    { 10200, "sim/multiplay/generic/float[0]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10201, "sim/multiplay/generic/float[1]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10202, "sim/multiplay/generic/float[2]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10203, "sim/multiplay/generic/float[3]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10204, "sim/multiplay/generic/float[4]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10205, "sim/multiplay/generic/float[5]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10206, "sim/multiplay/generic/float[6]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10207, "sim/multiplay/generic/float[7]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10208, "sim/multiplay/generic/float[8]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10209, "sim/multiplay/generic/float[9]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10210, "sim/multiplay/generic/float[10]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10211, "sim/multiplay/generic/float[11]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10212, "sim/multiplay/generic/float[12]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10213, "sim/multiplay/generic/float[13]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10214, "sim/multiplay/generic/float[14]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10215, "sim/multiplay/generic/float[15]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10216, "sim/multiplay/generic/float[16]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10217, "sim/multiplay/generic/float[17]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10218, "sim/multiplay/generic/float[18]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10219, "sim/multiplay/generic/float[19]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },

    { 10220, "sim/multiplay/generic/float[20]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10221, "sim/multiplay/generic/float[21]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10222, "sim/multiplay/generic/float[22]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10223, "sim/multiplay/generic/float[23]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10224, "sim/multiplay/generic/float[24]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10225, "sim/multiplay/generic/float[25]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10226, "sim/multiplay/generic/float[26]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10227, "sim/multiplay/generic/float[27]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10228, "sim/multiplay/generic/float[28]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10229, "sim/multiplay/generic/float[29]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10230, "sim/multiplay/generic/float[30]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10231, "sim/multiplay/generic/float[31]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10232, "sim/multiplay/generic/float[32]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10233, "sim/multiplay/generic/float[33]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10234, "sim/multiplay/generic/float[34]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10235, "sim/multiplay/generic/float[35]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10236, "sim/multiplay/generic/float[36]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10237, "sim/multiplay/generic/float[37]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10238, "sim/multiplay/generic/float[38]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10239, "sim/multiplay/generic/float[39]", simgear::props::FLOAT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },

    { 10300, "sim/multiplay/generic/int[0]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10301, "sim/multiplay/generic/int[1]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10302, "sim/multiplay/generic/int[2]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10303, "sim/multiplay/generic/int[3]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10304, "sim/multiplay/generic/int[4]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10305, "sim/multiplay/generic/int[5]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10306, "sim/multiplay/generic/int[6]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10307, "sim/multiplay/generic/int[7]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10308, "sim/multiplay/generic/int[8]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10309, "sim/multiplay/generic/int[9]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10310, "sim/multiplay/generic/int[10]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10311, "sim/multiplay/generic/int[11]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10312, "sim/multiplay/generic/int[12]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10313, "sim/multiplay/generic/int[13]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10314, "sim/multiplay/generic/int[14]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10315, "sim/multiplay/generic/int[15]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10316, "sim/multiplay/generic/int[16]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10317, "sim/multiplay/generic/int[17]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10318, "sim/multiplay/generic/int[18]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },
    { 10319, "sim/multiplay/generic/int[19]", simgear::props::INT, TT_ASIS,  V1_1_PROP_ID, NULL, NULL },

    { 10500, "sim/multiplay/generic/short[0]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10501, "sim/multiplay/generic/short[1]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10502, "sim/multiplay/generic/short[2]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10503, "sim/multiplay/generic/short[3]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10504, "sim/multiplay/generic/short[4]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10505, "sim/multiplay/generic/short[5]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10506, "sim/multiplay/generic/short[6]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10507, "sim/multiplay/generic/short[7]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10508, "sim/multiplay/generic/short[8]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10509, "sim/multiplay/generic/short[9]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10510, "sim/multiplay/generic/short[10]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10511, "sim/multiplay/generic/short[11]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10512, "sim/multiplay/generic/short[12]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10513, "sim/multiplay/generic/short[13]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10514, "sim/multiplay/generic/short[14]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10515, "sim/multiplay/generic/short[15]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10516, "sim/multiplay/generic/short[16]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10517, "sim/multiplay/generic/short[17]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10518, "sim/multiplay/generic/short[18]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10519, "sim/multiplay/generic/short[19]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10520, "sim/multiplay/generic/short[20]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10521, "sim/multiplay/generic/short[21]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10522, "sim/multiplay/generic/short[22]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10523, "sim/multiplay/generic/short[23]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10524, "sim/multiplay/generic/short[24]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10525, "sim/multiplay/generic/short[25]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10526, "sim/multiplay/generic/short[26]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10527, "sim/multiplay/generic/short[27]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10528, "sim/multiplay/generic/short[28]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10529, "sim/multiplay/generic/short[29]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10530, "sim/multiplay/generic/short[30]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10531, "sim/multiplay/generic/short[31]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10532, "sim/multiplay/generic/short[32]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10533, "sim/multiplay/generic/short[33]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10534, "sim/multiplay/generic/short[34]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10535, "sim/multiplay/generic/short[35]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10536, "sim/multiplay/generic/short[36]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10537, "sim/multiplay/generic/short[37]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10538, "sim/multiplay/generic/short[38]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10539, "sim/multiplay/generic/short[39]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10540, "sim/multiplay/generic/short[40]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10541, "sim/multiplay/generic/short[41]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10542, "sim/multiplay/generic/short[42]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10543, "sim/multiplay/generic/short[43]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10544, "sim/multiplay/generic/short[44]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10545, "sim/multiplay/generic/short[45]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10546, "sim/multiplay/generic/short[46]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10547, "sim/multiplay/generic/short[47]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10548, "sim/multiplay/generic/short[48]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10549, "sim/multiplay/generic/short[49]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10550, "sim/multiplay/generic/short[50]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10551, "sim/multiplay/generic/short[51]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10552, "sim/multiplay/generic/short[52]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10553, "sim/multiplay/generic/short[53]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10554, "sim/multiplay/generic/short[54]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10555, "sim/multiplay/generic/short[55]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10556, "sim/multiplay/generic/short[56]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10557, "sim/multiplay/generic/short[57]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10558, "sim/multiplay/generic/short[58]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10559, "sim/multiplay/generic/short[59]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10560, "sim/multiplay/generic/short[60]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10561, "sim/multiplay/generic/short[61]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10562, "sim/multiplay/generic/short[62]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10563, "sim/multiplay/generic/short[63]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10564, "sim/multiplay/generic/short[64]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10565, "sim/multiplay/generic/short[65]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10566, "sim/multiplay/generic/short[66]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10567, "sim/multiplay/generic/short[67]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10568, "sim/multiplay/generic/short[68]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10569, "sim/multiplay/generic/short[69]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10570, "sim/multiplay/generic/short[70]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10571, "sim/multiplay/generic/short[71]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10572, "sim/multiplay/generic/short[72]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10573, "sim/multiplay/generic/short[73]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10574, "sim/multiplay/generic/short[74]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10575, "sim/multiplay/generic/short[75]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10576, "sim/multiplay/generic/short[76]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10577, "sim/multiplay/generic/short[77]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10578, "sim/multiplay/generic/short[78]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { 10579, "sim/multiplay/generic/short[79]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },

    { BOOLARRAY_BASE_1 +  0, "sim/multiplay/generic/bool[0]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  1, "sim/multiplay/generic/bool[1]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  2, "sim/multiplay/generic/bool[2]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  3, "sim/multiplay/generic/bool[3]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  4, "sim/multiplay/generic/bool[4]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  5, "sim/multiplay/generic/bool[5]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  6, "sim/multiplay/generic/bool[6]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  7, "sim/multiplay/generic/bool[7]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  8, "sim/multiplay/generic/bool[8]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 +  9, "sim/multiplay/generic/bool[9]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 10, "sim/multiplay/generic/bool[10]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 11, "sim/multiplay/generic/bool[11]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 12, "sim/multiplay/generic/bool[12]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 13, "sim/multiplay/generic/bool[13]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 14, "sim/multiplay/generic/bool[14]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 15, "sim/multiplay/generic/bool[15]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 16, "sim/multiplay/generic/bool[16]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 17, "sim/multiplay/generic/bool[17]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 18, "sim/multiplay/generic/bool[18]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 19, "sim/multiplay/generic/bool[19]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 20, "sim/multiplay/generic/bool[20]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 21, "sim/multiplay/generic/bool[21]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 22, "sim/multiplay/generic/bool[22]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 23, "sim/multiplay/generic/bool[23]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 24, "sim/multiplay/generic/bool[24]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 25, "sim/multiplay/generic/bool[25]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 26, "sim/multiplay/generic/bool[26]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 27, "sim/multiplay/generic/bool[27]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 28, "sim/multiplay/generic/bool[28]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 29, "sim/multiplay/generic/bool[29]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_1 + 30, "sim/multiplay/generic/bool[30]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },

    { BOOLARRAY_BASE_2 + 0, "sim/multiplay/generic/bool[31]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 1, "sim/multiplay/generic/bool[32]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 2, "sim/multiplay/generic/bool[33]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 3, "sim/multiplay/generic/bool[34]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 4, "sim/multiplay/generic/bool[35]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 5, "sim/multiplay/generic/bool[36]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 6, "sim/multiplay/generic/bool[37]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 7, "sim/multiplay/generic/bool[38]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 8, "sim/multiplay/generic/bool[39]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 9, "sim/multiplay/generic/bool[40]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 10, "sim/multiplay/generic/bool[41]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
        // out of sequence between the block and the buffer becuase of a typo. repurpose the first as that way [72] will work
        // correctly on older versions.
    { BOOLARRAY_BASE_2 + 11, "sim/multiplay/generic/bool[91]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 12, "sim/multiplay/generic/bool[42]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 13, "sim/multiplay/generic/bool[43]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 14, "sim/multiplay/generic/bool[44]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 15, "sim/multiplay/generic/bool[45]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 16, "sim/multiplay/generic/bool[46]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 17, "sim/multiplay/generic/bool[47]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 18, "sim/multiplay/generic/bool[48]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 19, "sim/multiplay/generic/bool[49]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 20, "sim/multiplay/generic/bool[50]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 21, "sim/multiplay/generic/bool[51]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 22, "sim/multiplay/generic/bool[52]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 23, "sim/multiplay/generic/bool[53]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 24, "sim/multiplay/generic/bool[54]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 25, "sim/multiplay/generic/bool[55]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 26, "sim/multiplay/generic/bool[56]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 27, "sim/multiplay/generic/bool[57]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 28, "sim/multiplay/generic/bool[58]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 29, "sim/multiplay/generic/bool[59]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_2 + 30, "sim/multiplay/generic/bool[60]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },

    { BOOLARRAY_BASE_3 + 0, "sim/multiplay/generic/bool[61]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 1, "sim/multiplay/generic/bool[62]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 2, "sim/multiplay/generic/bool[63]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 3, "sim/multiplay/generic/bool[64]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 4, "sim/multiplay/generic/bool[65]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 5, "sim/multiplay/generic/bool[66]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 6, "sim/multiplay/generic/bool[67]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 7, "sim/multiplay/generic/bool[68]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 8, "sim/multiplay/generic/bool[69]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 9, "sim/multiplay/generic/bool[70]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 10, "sim/multiplay/generic/bool[71]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
        // out of sequence between the block and the buffer becuase of a typo. repurpose the first as that way [72] will work
        // correctly on older versions.
    { BOOLARRAY_BASE_3 + 11, "sim/multiplay/generic/bool[92]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 12, "sim/multiplay/generic/bool[72]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 13, "sim/multiplay/generic/bool[73]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 14, "sim/multiplay/generic/bool[74]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 15, "sim/multiplay/generic/bool[75]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 16, "sim/multiplay/generic/bool[76]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 17, "sim/multiplay/generic/bool[77]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 18, "sim/multiplay/generic/bool[78]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 19, "sim/multiplay/generic/bool[79]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 20, "sim/multiplay/generic/bool[80]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 21, "sim/multiplay/generic/bool[81]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 22, "sim/multiplay/generic/bool[82]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 23, "sim/multiplay/generic/bool[83]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 24, "sim/multiplay/generic/bool[84]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 25, "sim/multiplay/generic/bool[85]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 26, "sim/multiplay/generic/bool[86]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 27, "sim/multiplay/generic/bool[87]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 28, "sim/multiplay/generic/bool[88]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 29, "sim/multiplay/generic/bool[89]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },
    { BOOLARRAY_BASE_3 + 30, "sim/multiplay/generic/bool[90]", simgear::props::BOOL, TT_BOOLARRAY,  V1_1_2_PROP_ID, NULL, NULL },


    { V2018_1_BASE + 0,  "sim/multiplay/mp-clock-mode", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
        // Direct support for emesary bridge properties. This is mainly to ensure that these properties do not overlap with the string
        // properties; although the emesary bridge can use any string property.
    { EMESARYBRIDGE_BASE + 0,  "sim/multiplay/emesary/bridge[0]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 1,  "sim/multiplay/emesary/bridge[1]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 2,  "sim/multiplay/emesary/bridge[2]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 3,  "sim/multiplay/emesary/bridge[3]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 4,  "sim/multiplay/emesary/bridge[4]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 5,  "sim/multiplay/emesary/bridge[5]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 6,  "sim/multiplay/emesary/bridge[6]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 7,  "sim/multiplay/emesary/bridge[7]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 8,  "sim/multiplay/emesary/bridge[8]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 9,  "sim/multiplay/emesary/bridge[9]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 10, "sim/multiplay/emesary/bridge[10]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 11, "sim/multiplay/emesary/bridge[11]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 12, "sim/multiplay/emesary/bridge[12]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 13, "sim/multiplay/emesary/bridge[13]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 14, "sim/multiplay/emesary/bridge[14]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 15, "sim/multiplay/emesary/bridge[15]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 16, "sim/multiplay/emesary/bridge[16]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 17, "sim/multiplay/emesary/bridge[17]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 18, "sim/multiplay/emesary/bridge[18]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 19, "sim/multiplay/emesary/bridge[19]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 20, "sim/multiplay/emesary/bridge[20]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 21, "sim/multiplay/emesary/bridge[21]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 22, "sim/multiplay/emesary/bridge[22]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 23, "sim/multiplay/emesary/bridge[23]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 24, "sim/multiplay/emesary/bridge[24]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 25, "sim/multiplay/emesary/bridge[25]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 26, "sim/multiplay/emesary/bridge[26]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 27, "sim/multiplay/emesary/bridge[27]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 28, "sim/multiplay/emesary/bridge[28]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGE_BASE + 29, "sim/multiplay/emesary/bridge[29]", simgear::props::STRING, TT_ASIS,  V1_1_2_PROP_ID, NULL, NULL },

        // To allow the bridge to identify itself and allow quick filtering based on type/ID.
    { EMESARYBRIDGETYPE_BASE + 0,  "sim/multiplay/emesary/bridge-type[0]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 1,  "sim/multiplay/emesary/bridge-type[1]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 2,  "sim/multiplay/emesary/bridge-type[2]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 3,  "sim/multiplay/emesary/bridge-type[3]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 4,  "sim/multiplay/emesary/bridge-type[4]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 5,  "sim/multiplay/emesary/bridge-type[5]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 6,  "sim/multiplay/emesary/bridge-type[6]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 7,  "sim/multiplay/emesary/bridge-type[7]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 8,  "sim/multiplay/emesary/bridge-type[8]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 9,  "sim/multiplay/emesary/bridge-type[9]",  simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 10, "sim/multiplay/emesary/bridge-type[10]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 11, "sim/multiplay/emesary/bridge-type[11]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 12, "sim/multiplay/emesary/bridge-type[12]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 13, "sim/multiplay/emesary/bridge-type[13]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 14, "sim/multiplay/emesary/bridge-type[14]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 15, "sim/multiplay/emesary/bridge-type[15]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 16, "sim/multiplay/emesary/bridge-type[16]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 17, "sim/multiplay/emesary/bridge-type[17]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 18, "sim/multiplay/emesary/bridge-type[18]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 19, "sim/multiplay/emesary/bridge-type[19]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 20, "sim/multiplay/emesary/bridge-type[20]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 21, "sim/multiplay/emesary/bridge-type[21]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 22, "sim/multiplay/emesary/bridge-type[22]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 23, "sim/multiplay/emesary/bridge-type[23]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 24, "sim/multiplay/emesary/bridge-type[24]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 25, "sim/multiplay/emesary/bridge-type[25]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 26, "sim/multiplay/emesary/bridge-type[26]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 27, "sim/multiplay/emesary/bridge-type[27]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 28, "sim/multiplay/emesary/bridge-type[28]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { EMESARYBRIDGETYPE_BASE + 29, "sim/multiplay/emesary/bridge-type[29]", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },

    { FALLBACK_MODEL_ID, "sim/model/fallback-model-index", simgear::props::INT, TT_SHORTINT,  V1_1_2_PROP_ID, NULL, NULL },
    { V2019_3_BASE,   "sim/multiplay/comm-transmit-frequency-hz", simgear::props::INT, TT_INT,  V1_1_2_PROP_ID, NULL, NULL },
    { V2019_3_BASE+1, "sim/multiplay/comm-transmit-power-norm", simgear::props::INT, TT_SHORT_FLOAT_NORM ,  V1_1_2_PROP_ID, NULL, NULL },
    // Add new MP properties here
};
/*
 * For the 2017.x version 2 protocol the properties are sent in two partitions,
 * the first of these is a V1 protocol packet (which should be fine with all clients), and a V2 partition
 * which will contain the newly supported shortint and fixed string encoding schemes.
 * This is to possibly allow for easier V1/V2 conversion - as the packet can simply be truncated at the
 * first V2 property based on ID.
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


/*
* 2018.1 introduces a new minimal generic packet concept.
* This allows a model to choose to only transmit a few essential properties, which leaves the packet at around 380 bytes.
* The rest of the packet can then be used for bridged Emesary notifications, which over allow much more control
* at the model level, including different notifications being sent.
* see $FGData/Nasal/Notifications.nas and $FGData/Nasal/emesary_mp_bridge.nas
* The property /sim/multiplay/transmit-filter-property-base can be set to 1 to turn off all of the standard properties and only send generics.
* or this property can be set to a number greater than 1 (e.g. 12000) to only transmit properties based on their index. It is a simple filtering
* mechanism.
* - in both cases the chat and transponder properties will be transmitted for compatibility.
*/
static inline bool IsIncludedInPacket(int filter_base, int property_id)
{
    if (filter_base == 1) // transmit-property-base of 1 is equivalent to only generics.
        return property_id >= 10002
        || (property_id == V2018_1_BASE)  // MP time sync
        || (property_id >= 1500 && property_id < 1600); // include chat and generic properties.
    else
        return property_id >= filter_base
        || (property_id == V2018_1_BASE)  // MP time sync
        || (property_id >= 1500 && property_id < 1600); // include chat and generic properties.
}

//////////////////////////////////////////////////////////////////////
//
//  handle command "multiplayer-connect"
//  args:
//  server: servername to connect (mandatory)
//  txport: outgoing port number (default: 5000)
//  rxport: incoming port number (default: 5000)
//////////////////////////////////////////////////////////////////////
static bool do_multiplayer_connect(const SGPropertyNode * arg, SGPropertyNode * root) {
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
static bool do_multiplayer_disconnect(const SGPropertyNode * arg, SGPropertyNode * root) {
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
do_multiplayer_refreshserverlist (const SGPropertyNode * arg, SGPropertyNode * root)
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
  pMultiPlayTransmitPropertyBase = fgGetNode("/sim/multiplay/transmit-filter-property-base", true);
  pMultiPlayRange = fgGetNode("/sim/multiplay/visibility-range-nm", true);
  pMultiPlayRange->setIntValue(100);
  pReplayState = fgGetNode("/sim/replay/replay-state", true);
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

  
  SG_LOG(SG_NETWORK, SG_MANDATORY_INFO, "Multiplayer mode active");
  flightgear::addSentryTag("mp", "active");

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
  int protocolToUse = getProtocolToUse();
  int transmitFilterPropertyBase = pMultiPlayTransmitPropertyBase->getIntValue();
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
   * MP2017(V2) (for V1 clients) will always have an unknown property because V2 transmits
   * the protocol version as the very first property as a shortint.
   */
  if (protocolToUse > 1)
      PosMsg->pad = XDR_encode_int32(V2_PAD_MAGIC);
  else
      PosMsg->pad = 0;

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

      //if (pMultiPlayDebugLevel->getIntValue())
      //    msgBuf.zero();
      struct BoolArrayBuffer boolBuffer[MAX_BOOL_BUFFERS];
      memset(&boolBuffer, 0, sizeof(boolBuffer));

      for (int partition = 1; partition <= protocolToUse; partition++)
      {
          std::vector<FGPropertyData*>::const_iterator it = motionInfo.properties.begin();
          while (it != motionInfo.properties.end()) {
              const struct IdPropertyList* propDef = mPropertyDefinition[(*it)->id];

              /*
               * Excludes the 2017.2 property for the protocol version from V1 packets.
               */
              if (protocolToUse == 1 && propDef->version == V2_PROP_ID_PROTOCOL)
              {
                  ++it;
                  continue;
              }
              /*
               * If requested only transmit the properties that are above the filter base index; and essential other properties
               * a value of 1 is equivalent to just transmitting generics (>10002)
               * a value of 12000 is for only emesary properties.
               */
              if (transmitFilterPropertyBase && !IsIncludedInPacket(transmitFilterPropertyBase, propDef->id))
              {
                  ++it;
                  continue;
              }
              /*
               * 2017.2 partitions the buffer sent into protocol versions. Originally this was intended to allow
               * compatability with older clients; however this will only work in the future or with support from fgms
               * - so if a future version adds more properties to the protocol these can be transmitted in a third partition
               *   that will be ignored by older clients (such as 2017.2).
               */
              if ( (propDef->version & 0xffff) == partition || (propDef->version & 0xffff)  > protocolToUse)
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

                  if (propDef->TransmitAs != TT_ASIS && protocolToUse > 1)
                  {
                      transmit_type = propDef->TransmitAs;
                  }
                  else if (propDef->TransmitAs == TT_BOOLARRAY)
                      transmit_type = propDef->TransmitAs;

                  if (pMultiPlayDebugLevel->getIntValue() & 2)
                      SG_LOG(SG_NETWORK, SG_INFO,
                          "[SEND] pt " << partition <<
                          ": buf[" << (ptr - data) * sizeof(*ptr)
                          << "] id=" << (*it)->id << " type " << transmit_type);

                  if (propDef->encode_for_transmit && protocolToUse > 1)
                  {
                      ptr = (*propDef->encode_for_transmit)(propDef, ptr, (*it));
                  }
                  else
                  {
                      // The actual data representation depends on the type
                      switch (transmit_type) {
                      case TT_NOSEND:
                          break;
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
                      case TT_BOOLARRAY:
                      {
                          struct BoolArrayBuffer *boolBuf = nullptr;
                          if ((*it)->id >= BOOLARRAY_START_ID && (*it)->id <= BOOLARRAY_END_ID + BOOLARRAY_BLOCKSIZE)
                          {
                              int buffer_block = ((*it)->id - BOOLARRAY_BASE_1) / BOOLARRAY_BLOCKSIZE;
                              boolBuf = &boolBuffer[buffer_block];
                              boolBuf->propertyId = BOOLARRAY_START_ID + buffer_block * BOOLARRAY_BLOCKSIZE;
                          }
                          if (boolBuf)
                          {
                              int bitidx = (*it)->id - boolBuf->propertyId;
                              if ((*it)->int_value)
                                  boolBuf->boolValue |= 1 << bitidx;
                          }
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
                          if (protocolToUse > 1)
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
              }
              ++it;
          }
      }
      escape:

      /*
      * Send the boolean arrays (if present) as single 32bit integers.
      */
      for (int boolIdx = 0; boolIdx < MAX_BOOL_BUFFERS; boolIdx++)
      {
          if (boolBuffer[boolIdx].propertyId)
          {
              if (ptr + 2 >= msgEnd)
              {
                  SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer packet truncated prop id: " << boolBuffer[boolIdx].propertyId << ": multiplay/generic/bools[" << boolIdx * 30 << "]");
              }
              *ptr++ = XDR_encode_int32(boolBuffer[boolIdx].propertyId);
              *ptr++ = XDR_encode_int32(boolBuffer[boolIdx].boolValue);
          }
      }

      msgLen = reinterpret_cast<char*>(ptr) - msgBuf.Msg;
      FillMsgHdr(msgBuf.msgHdr(), POS_DATA_ID, msgLen);

      /*
      * Informational:
      * Save the last packet length sent, and
      * if the property is set then dump the packet length to the console.
      * ----------------------------
      * This should be sufficient for rudimentary debugging (in order of useful ness)
      *  1. loopback your own craft. fantastic for resolving animations and property transmission issues.
      *  2. see what properties are being sent
      *  3. see how much space it takes up
      *  4. dump the packet as it goes out
      *  5. dump incoming packets
      */
      pXmitLen->setIntValue(msgLen); // 2. store the size of the properties as transmitted

      if (pMultiPlayDebugLevel->getIntValue() & 2) // and dump it to the console
      {
          SG_LOG(SG_NETWORK, SG_INFO,
              "[SEND] Packet len " << msgLen);
      }
      if (pMultiPlayDebugLevel->getIntValue() & 4) // 4. hexdump the packet
          SG_LOG_HEXDUMP(SG_NETWORK, SG_INFO, data, (ptr - data) * sizeof(*ptr));
      /*
       * simple loopback of ourselves - to enable easy MP debug for model developers; see (1) above
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
             MAX_CHAT_MSG_LEN - 1);
    ChatMsg.Text[MAX_CHAT_MSG_LEN - 1] = '\0';
    memcpy (Msg, &MsgHdr, sizeof(T_MsgHdr));
    memcpy (Msg + sizeof(T_MsgHdr), &ChatMsg, sizeof(T_ChatMsg));
    mSocket->sendto (Msg, sizeof(T_MsgHdr) + sizeof(T_ChatMsg), 0, &mServer);
    iNextBlockPosition += MAX_CHAT_MSG_LEN - 1;

  }


} // FGMultiplayMgr::SendTextMessage ()
//////////////////////////////////////////////////////////////////////


// If a message is available from mSocket, copies into <msgBuf>, converts
// endiness of the T_MsgHdr, and returns length.
//
// Otherwise returns 0.
//
int FGMultiplayMgr::GetMsgNetwork(MsgBuf& msgBuf, simgear::IPAddress& SenderAddress)
{
        //////////////////////////////////////////////////
        //  Although the recv call asks for
        //  MAX_PACKET_SIZE of data, the number of bytes
        //  returned will only be that of the next
        //  packet waiting to be processed.
        //////////////////////////////////////////////////
        if (!mSocket) {
            return 0;
        }
        int RecvStatus = mSocket->recvfrom(msgBuf.Msg, sizeof(msgBuf.Msg), 0,
                                  &SenderAddress);
        //////////////////////////////////////////////////
        //  no Data received
        //////////////////////////////////////////////////
        if (RecvStatus == 0)
            return 0;

        // socket error reported?
        // errno isn't thread-safe - so only check its value when
        // socket return status < 0 really indicates a failure.
        if ((RecvStatus < 0)&&
            ((errno == EAGAIN) || (errno == 0))) // MSVC output "NoError" otherwise
        {
            // ignore "normal" errors
            return 0;
        }

        if (RecvStatus<0)
        {
    #ifdef _WIN32
            if (::WSAGetLastError() != WSAEWOULDBLOCK) // this is normal on a receive when there is no data
            {
                // with Winsock the error will not be the actual problem.
                SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr::MP_ProcessData - Unable to receive data. WSAGetLastError=" << ::WSAGetLastError());
            }
    #else
            SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - Unable to receive data. "
                << strerror(errno) << "(errno " << errno << ")");
    #endif
            return 0;
        }
        
        T_MsgHdr* MsgHdr = msgBuf.msgHdr();
        MsgHdr->Magic       = XDR_decode_uint32 (MsgHdr->Magic);
        MsgHdr->Version     = XDR_decode_uint32 (MsgHdr->Version);
        MsgHdr->MsgId       = XDR_decode_uint32 (MsgHdr->MsgId);
        MsgHdr->MsgLen      = XDR_decode_uint32 (MsgHdr->MsgLen);
        MsgHdr->ReplyPort   = XDR_decode_uint32 (MsgHdr->ReplyPort);
        MsgHdr->Callsign[MAX_CALLSIGN_LEN -1] = '\0';
        
        return RecvStatus;
}

// Returns message in msgBuf out-param.
//
// If we are in replay mode, we return recorded messages (omitting recorded
// chat messages), and live chat messages from mSocket.
//
int FGMultiplayMgr::GetMsg(MsgBuf& msgBuf, simgear::IPAddress& SenderAddress)
{
    if (pReplayState->getIntValue()) {
        // We are replaying, so return non-chat multiplayer messages from
        // mReplayMessageQueue and live chat messages from mSocket.
        //
        for(;;) {
        
            if (mReplayMessageQueue.empty()) {
                // No recorded messages available, so look for live messages
                // from <mSocket>.
                //
                int RecvStatus = GetMsgNetwork(msgBuf, SenderAddress);
                if (RecvStatus == 0) {
                    // No recorded messages, and no live messages, so return 0.
                    return 0;
                }
                
                // Always record all messages.
                //
                std::shared_ptr<std::vector<char>> data( new std::vector<char>(RecvStatus));
                memcpy( &data->front(), msgBuf.Msg, RecvStatus);
                mRecordMessageQueue.push_back(data);
                
                if (msgBuf.Header.MsgId == CHAT_MSG_ID) {
                    return RecvStatus;
                }

                // If we get here, there is a live message but it is a
                // multiplayer aircraft position so we ignore it while
                // replaying.
            }
            else {
                // Replay recorded message, unless it is a chat message.
                //
                auto replayMessage = mReplayMessageQueue.front();
                mReplayMessageQueue.pop_front();
                assert(replayMessage->size() <= sizeof(msgBuf));
                int length = replayMessage->size();
                memcpy(&msgBuf.Msg, &replayMessage->front(), length);
                // Don't return recorded chat messages.
                if (msgBuf.Header.MsgId != CHAT_MSG_ID) {
                    SG_LOG(SG_NETWORK, SG_INFO,
                            "replaying message length=" << replayMessage->size()
                            << ". num remaining messages=" << mReplayMessageQueue.size()
                            );
                    return length;
                }
            }
        }
    }
    else {
        int length = GetMsgNetwork(msgBuf, SenderAddress);
        
        // Make raw incoming packet available to recording code.
        if (length) {
            std::shared_ptr<std::vector<char>> data( new std::vector<char>(length));
            memcpy( &data->front(), msgBuf.Msg, length);
            mRecordMessageQueue.push_back(data);
        }
        return length;
    }
}


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
  // We carry on even if !mInitialised, in case we are replaying a multiplayer
  // recording.
  //

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
  //  Read from receive socket and/or multiplayer
  //  replay, and process any data.
  //////////////////////////////////////////////////
  ssize_t bytes;
  do {
    MsgBuf  msgBuf;
    simgear::IPAddress SenderAddress;
    int RecvStatus = GetMsg(msgBuf, SenderAddress);
    if (RecvStatus == 0) {
        break;
    }
    // status is positive: bytes received
    bytes = (ssize_t) RecvStatus;
    if (bytes <= static_cast<ssize_t>(sizeof(T_MsgHdr))) {
      SG_LOG( SG_NETWORK, SG_INFO, "FGMultiplayMgr::MP_ProcessData - "
              << "received message with insufficient data" );
      break;
    }
    
    //////////////////////////////////////////////////
    //  Read header
    //////////////////////////////////////////////////
    T_MsgHdr* MsgHdr = msgBuf.msgHdr();
    if (MsgHdr->Magic != MSG_MAGIC) {
        SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid magic number!" );
      break;
    }
    if (MsgHdr->Version != PROTO_VER) {
        SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid protocol number!" );
      break;
    }
    if (static_cast<ssize_t>(MsgHdr->MsgLen) != bytes) {
        SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr::MP_ProcessData - "
             << "message from " << MsgHdr->Callsign << " has invalid length!");
      break;
    }
    //hexdump the incoming packet
    if (pMultiPlayDebugLevel->getIntValue() & 16)
        SG_LOG_HEXDUMP(SG_NETWORK, SG_INFO, msgBuf.Msg, MsgHdr->MsgLen);

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
        SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr::MP_ProcessData - "
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

void FGMultiplayMgr::ClearMotion()
{
    SG_LOG(SG_NETWORK, SG_DEBUG, "Clearing all motion info");
    for (auto it: mMultiPlayerMap) {
        it.second->clearMotionInfo();
    }
}

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

    // we are now using a time immune to pause, warp etc as timestamp.
    double mp_time = globals->get_subsystem<TimeManager>()->getMPProtocolClockSec();

    //    static double lastTime = 0.0;

       // SG_LOG(SG_GENERAL, SG_INFO, "actual mp dt=" << mp_time - lastTime);
    //    lastTime = mp_time;

    FlightProperties ifce;

    // put together a motion info struct, you will get that later
    // from FGInterface directly ...
    FGExternalMotionData motionInfo;

    // The current simulation time we need to update for,
    // note that the simulation time is updated before calling all the
    // update methods. Thus it contains the time intervals *end* time.
    // The FDM is already run, so the states belong to that time.
    motionInfo.time = mp_time;
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

        switch (static_cast<int>(pData->type)) {
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
   int fallback_model_index = 0;
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

    if (plist)
    {
      FGPropertyData* pData = new FGPropertyData;
      if (plist->decode_received)
      {
          //
          // this needs the pointer prior to the extraction of the property id and possible shortint decode
          // too allow the method to redecode as it wishes
          xdr = (*plist->decode_received)(plist, xdr, pData);
      }
      else
      {
        pData->id = id;
        pData->type = plist->type;
        xdr++;
        // How we decode the remainder of the property depends on the type
        switch (pData->type) {
        case simgear::props::BOOL:
            /*
             * For 2017.2 we support boolean arrays transmitted as a single int for 30 bools.
             * this section handles the unpacking into the arrays.
             */
            if (pData->id >= BOOLARRAY_START_ID && pData->id <= BOOLARRAY_END_ID)
            {
                unsigned int val = XDR_decode_uint32(*xdr);
                bool first_bool = true;
                xdr++;
                for (int bitidx = 0; bitidx <= 30; bitidx++)
                {
                    // ensure that this property is in the master list.
                    const IdPropertyList* plistBool = findProperty(id + bitidx);

                    if (plistBool)
                    {
                        if (first_bool)
                            first_bool = false;
                        else
                            pData = new FGPropertyData;

                        pData->id = id + bitidx;
                        pData->int_value = (val & (1 << bitidx)) != 0;
                        pData->type = simgear::props::BOOL;
                        motionInfo.properties.push_back(pData);

                        // ensure that this is null because this section of code manages the property data and list directly
                        // it has to be this way because one MP value results in multiple properties being set.
                        pData = nullptr;
                    }
                }
                break;
            }
        case simgear::props::INT:
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
                xdr++;
            }
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
                    pData->string_value[i] = (char)XDR_decode_int8(*xdr);
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
    }
    if (pData) {
      motionInfo.properties.push_back(pData);

      // Special case - we need the /sim/model/fallback-model-index to create
      // the MP model
      if (pData->id == FALLBACK_MODEL_ID) {
        fallback_model_index = pData->int_value;
        SG_LOG(SG_NETWORK, SG_DEBUG, "Found Fallback model index in message " << fallback_model_index);
      }
    }
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
    mp = addMultiplayer(MsgHdr->Callsign, PosMsg->Model, fallback_model_index);
  mp->addMotionInfo(motionInfo, stamp);
} // FGMultiplayMgr::ProcessPosMsg()


std::shared_ptr<std::vector<char>> FGMultiplayMgr::popMessageHistory()
{
    if (mRecordMessageQueue.empty()) {
        return nullptr;
    }

    auto ret = mRecordMessageQueue.front();
    mRecordMessageQueue.pop_front();
    return ret;
}

void FGMultiplayMgr::pushMessageHistory(std::shared_ptr<std::vector<char>> message)
{
    mReplayMessageQueue.push_back(message);
}

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


/* If <from>/<path> exists and <to>/<path> doesn't, copy the former to the
latter. */
static void copy_default(SGPropertyNode* from, const char* path, SGPropertyNode* to) {
    SGPropertyNode* from_ = from->getNode(path);
    if (from_) {
        if (!to->getNode(path)) {
            to->setDoubleValue(path, from_->getDoubleValue());
        }
    }
}

static std::string  makeStringPropertyNameSafe(const std::string& s)
{
  std::string   ret;
  for (size_t i=0; i<s.size(); ++i) {
    char    c = s[i];
    if (i==0 && !isalpha(c) && c!='_')  c = '_';
    if (!isalnum(c) && c!='.' && c!='_' && c!='-')  c = '_';
    ret += c;
  }
  return ret;
}

FGAIMultiplayer*
FGMultiplayMgr::addMultiplayer(const std::string& callsign,
                               const std::string& modelName,
                               const int fallback_model_index)
{
  if (0 < mMultiPlayerMap.count(callsign))
    return mMultiPlayerMap[callsign].get();

  FGAIMultiplayer* mp = new FGAIMultiplayer;
  mp->setPath(modelName.c_str());
  mp->setFallbackModelIndex(fallback_model_index);
  mp->setCallSign(callsign);
  mMultiPlayerMap[callsign] = mp;

  FGAIManager *aiMgr = (FGAIManager*)globals->get_subsystem("ai-model");
  if (aiMgr) {
    aiMgr->attach(mp);

    /// FIXME: that must follow the attach ATM ...
    for (unsigned i = 0; i < numProperties; ++i)
      mp->addPropertyId(sIdPropertyList[i].id, sIdPropertyList[i].name);
  }
  
  /* Try to find a -set.xml for <modelName>, so that we can use its view
  parameters. If found, we install it into a 'set' property node.
  
  If we are reusing an old entry in /ai/models/multiplayer[], there
  might be an old set/ node, so remove it.
  
  todo: maybe we should cache the -set.xml nodes in memory and/or share them in
  properties?
  */
  mp->_getProps()->removeChildren("set");
  
  SGPropertyNode_ptr    set;
  
  if (simgear::strutils::ends_with(modelName, ".xml")
      && simgear::strutils::starts_with(modelName, "Aircraft/")) {
  
    std::string tail = modelName.substr(strlen("Aircraft/"));
    
    PathList    dirs(globals->get_aircraft_paths());
    
    /* Need to append <fgdata>/Aircraft, otherwise we won't be able to find
    c172p. */
    SGPath  fgdata_aircraft = globals->get_fg_root();
    fgdata_aircraft.append("Aircraft");
    dirs.push_back(fgdata_aircraft);
    
    SGPath model_file;
    PathList::const_iterator it = std::find_if(dirs.begin(), dirs.end(),
        [&](SGPath dir) {
            model_file = dir;
            model_file.append(tail);
            return model_file.exists();
        });
    
    if (it != dirs.end()) {
      /* We've found the model file.
      
      Now try each -set.xml file in <modelName> aircraft directory. In theory
      an aircraft could have a -set.xml in an unrelated directory so we should
      scan all directories in globals->get_aircraft_paths(), but in practice
      most -set.xml files and models are in the same aircraft directory. */
      std::string model_file_head = it->str() + '/';
      std::string model_file_tail = model_file.str().substr(model_file_head.size());
      ssize_t p = model_file_tail.find('/');
      std::string aircraft_dir = model_file_head + model_file_tail.substr(0, p);
      simgear::Dir  dir(aircraft_dir);
      std::vector<SGPath>   dir_contents = dir.children(0 /*types*/, "-set.xml");
      /* simgear::Dir::children() claims that second param is glob, but
      actually it's just a suffix. */
      
      for (auto path: dir_contents) {
        /* Load into a local SGPropertyNode.
        
        As of 2020-03-08 we don't load directly into the global property tree
        because that appears to result in runtime-specific multiplayer values
        being written to autosave*.xml and reloaded next time fgfs is run,
        which results in lots of bogus properties within /ai/multiplayer. So
        instead we load into a local SGPropertyNode, then copy selected values
        into the global tree below. */
        set = new SGPropertyNode;
        bool    ok = true;
        try {
          readProperties(path, set);
        }
        catch ( const std::exception &e ) {
          ok = false;
        }
        if (ok) {
          SGPropertyNode* sim_model_path = set->getNode("sim/model/path");
          if (sim_model_path && sim_model_path->getStringValue() == modelName) {
            /* We've found (and loaded) a matching -set.xml. */
            break;
          }
        }
        set.reset();
      }
    }
  }
  
  // Copy values from our local <set>/sim/view[]/config/* into global
  // /ai/models/multiplayer/set/sim/view[]/config/ so that we have view offsets
  // available for this multiplayer aircraft.
  SGPropertyNode* global_set = mp->_getProps()->addChild("set");
  SGPropertyNode* global_sim = global_set->addChild("sim");
  double  sim_chase_distance_m = -25;
  if (set) {
    SGPropertyNode* sim = set->getChild("sim");
    if (sim) {
      /* Override <sim_chase_distance_m> if present. */
      sim_chase_distance_m = sim->getDoubleValue("chase-distance-m", sim_chase_distance_m);
      global_sim->setDoubleValue("chase-distance-m", sim_chase_distance_m);
      
      simgear::PropertyList   views = sim->getChildren("view");
      for (auto view: views) {
        int view_index = view->getIndex();
        SGPropertyNode* global_view = global_sim->addChild("view", view_index, false /*append*/);
        assert(global_view->getIndex() == view_index);
        SGPropertyNode* config = view->getChild("config");
        SGPropertyNode* global_config = global_view->addChild("config");
        if (config) {
          int config_children_n = config->nChildren();
          for (int i=0; i<config_children_n; ++i) {
            SGPropertyNode* node = config->getChild(i);
            global_config->setDoubleValue(node->getName(), node->getDoubleValue());
          }
        }
      }
    }
  }
    
  /* For views that are similar to Helicopter View, copy across Helicopter View
  target offsets if not specified. E.g. this allows Tower View AGL to work on
  aircraft that don't know about it but need non-zero target-*-offset-m values
  to centre the view on the middle of the aircraft.

  This mimics what fgdata:Nasal/view.nas:manager does for the user aircraft's
  views.
  */
  SGPropertyNode*   view_1 = global_sim->getNode("view", 1);
  std::initializer_list<int>    views_with_default_z_offset_m = {1, 2, 3, 5, 7};
  for (int j: views_with_default_z_offset_m) {
    SGPropertyNode* v = global_sim->getChild("view", j);
    if (!v) {
        v = global_sim->addChild("view", j, false /*append*/);
    }
    SGPropertyNode* z_offset_m = v->getChild("config/z-offset-m");
    /* Setting config/z-offset-m default to <sim_chase_distance_m> here mimics
    what fgdata/defaults.xml does when it defines default views. */
    if (!z_offset_m) {
        v->setDoubleValue("config/z-offset-m", sim_chase_distance_m);
    }
    copy_default(view_1, "config/target-x-offset-m", v);
    copy_default(view_1, "config/target-y-offset-m", v);
    copy_default(view_1, "config/target-z-offset-m", v);
  }
  
  /* Create a node /ai/models/callsigns/<callsign> containing the index of the
  callsign's aircraft's entry in /ai/models/multiplayer[]. This isn't strictly
  necessary, but simpifies debugging a lot and seems pretty lightweight. Note
  that we need to avoid special characters in the node name, otherwise the
  property system forces a fatal error. */
  std::string   path = "/ai/models/callsigns/" + makeStringPropertyNameSafe(callsign);
  globals->get_props()->setIntValue(path, mp->_getProps()->getIndex());
  
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


// Register the subsystem.
SGSubsystemMgr::Registrant<FGMultiplayMgr> registrantFGMultiplayMgr(
    SGSubsystemMgr::POST_FDM,
    {{"ai-model", SGSubsystemMgr::Dependency::HARD},
     {"flight", SGSubsystemMgr::Dependency::HARD},
     {"mp", SGSubsystemMgr::Dependency::HARD},
     {"time", SGSubsystemMgr::Dependency::HARD}});
