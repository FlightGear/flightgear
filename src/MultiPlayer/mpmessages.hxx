// mpmessages.hxx -- Message definitions for multiplayer communications
// within a multiplayer Flightgear
//
// Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
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
#define POS_DATA_ID             3

// XDR demands 4 byte alignment, but some compilers use8 byte alignment
// so it's safe to let the overall size of a netmork message be a 
// multiple of 8!
#define MAX_CALLSIGN_LEN        8
#define MAX_CHAT_MSG_LEN        48
#define MAX_MODEL_NAME_LEN      48

/** Aircraft position message */
typedef xdr_data2_t xdrPosition[3];
typedef xdr_data_t  xdrOrientation[4];

// Header for use with all messages sent 
class T_MsgHdr {
public:  
    xdr_data_t  Magic;                  // Magic Value
    xdr_data_t  Version;                // Protocoll version
    xdr_data_t  MsgId;                  // Message identifier 
    xdr_data_t  MsgLen;                 // absolue length of message
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
    xdrPosition     PlayerPosition;     // players position
    xdrOrientation  PlayerOrientation;  // players orientation
};

#endif


