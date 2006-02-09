// mpmessages.hxx -- Message definitions for multiplayer communications
// within a multiplayer Flightgear
//
// Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
//
// With minor additions be Vivian Meazza, January 2006
//
// Copyright (C) 2003  Airservices Australia
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

#ifndef MPMESSAGES_H
#define MPMESSAGES_H

#define MPMESSAGES_HID "$Id$"

/****************************************************************
* @version $Id$
*
* Description: Each message used for multiplayer communications
* consists of a header and optionally a block of data. The combined
* header and data is sent as one IP packet.
*
******************************************************************/

#include <plib/sg.h>
#include <simgear/compiler.h>
#include "tiny_xdr.hxx"

// magic value for messages
const uint32_t MSG_MAGIC = 0x46474653;  // "FGFS"
// protocoll version
const uint32_t PROTO_VER = 0x00010001;  // 1.1

// Message identifiers
#define CHAT_MSG_ID             1
#define UNUSABLE_POS_DATA_ID    2
#define OLD_POS_DATA_ID         3
#define POS_DATA_ID             4
#define PROP_MSG_ID             5

// XDR demands 4 byte alignment, but some compilers use8 byte alignment
// so it's safe to let the overall size of a network message be a 
// multiple of 8!
#define MAX_CALLSIGN_LEN        8
#define MAX_CHAT_MSG_LEN        256
#define MAX_MODEL_NAME_LEN      96
#define MAX_PROPERTY_LEN        52

/** Aircraft position message */
typedef xdr_data2_t xdrPosition[3];
typedef xdr_data_t  xdrOrientation[4];

// Header for use with all messages sent 
class T_MsgHdr {
public:  
    xdr_data_t  Magic;                  // Magic Value
    xdr_data_t  Version;                // Protocoll version
    xdr_data_t  MsgId;                  // Message identifier 
    xdr_data_t  MsgLen;                 // absolute length of message
    xdr_data_t  ReplyAddress;           // (player's receiver address
    xdr_data_t  ReplyPort;              // player's receiver port
    char Callsign[MAX_CALLSIGN_LEN];    // Callsign used by the player
};

// Chat message 
class T_ChatMsg {
public:    
    char Text[MAX_CHAT_MSG_LEN];       // Text of chat message
};

// Position message
class T_PositionMsg {
public:
    char Model[MAX_MODEL_NAME_LEN];    // Name of the aircraft model
    xdr_data_t  time;                  // Time when this packet was generated
    xdr_data_t  timeusec;              // Microsecs when this packet was generated
    xdr_data2_t lat;                   // Position, orientation, speed
    xdr_data2_t lon;                   // ...
    xdr_data2_t alt;                   // ...
    xdr_data2_t hdg;                   // ...
    xdr_data2_t roll;                  // ...
    xdr_data2_t pitch;                 // ...
    xdr_data2_t speedN;                // ...
    xdr_data2_t speedE;                // ...
    xdr_data2_t speedD;                // ...
	xdr_data_t  accN;				   // acceleration N
	xdr_data_t  accE;                  // acceleration E
	xdr_data_t  accD;                  // acceleration D
    xdr_data_t  left_aileron;          // control positions
    xdr_data_t  right_aileron;         // control positions
    xdr_data_t  elevator;              // ...
    xdr_data_t  rudder;                // ...
//    xdr_data_t  rpms[6];               // RPMs of all of the motors
    xdr_data_t  rateH;                 // Rate of change of heading
    xdr_data_t  rateR;                 // roll
    xdr_data_t  rateP;                 // and pitch
//	xdr_data_t  dummy;                 // pad message length
};

// Property message
class T_PropertyMsg {
public:
    char property[MAX_PROPERTY_LEN];	// the property name
    xdr_data_t  type;					// the type
    xdr_data2_t val;					// and value
//	xdr_data2_t dummy;					// pad message length
	
};

#endif


