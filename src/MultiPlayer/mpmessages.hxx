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

#include <stdint.h>
#include <plib/sg.h>

// Message identifiers
#define CHAT_MSG_ID 1
#define UNUSABLE_POS_DATA_ID 2
#define POS_DATA_ID 3
/* should be a multiple of 8! */
#define MAX_CALLSIGN_LEN 8
/** Header for use with all messages sent */
typedef struct {

    /** Message identifier, multiple of 8! */
    uint32_t MsgId;

    /** Length of the message inclusive of this header */
    uint32_t iMsgLen;

    /** IP address for reply to message (player's receiver address) */
    uint32_t lReplyAddress;

    /** Port for replies (player's receiver port) */
    uint32_t iReplyPort;

    /** Callsign used by the player */
    char sCallsign[MAX_CALLSIGN_LEN];

} T_MsgHdr;

#define MAX_CHAT_MSG_LEN 48
/** Chat message */
typedef struct {

    /** Text of chat message */
    char sText[MAX_CHAT_MSG_LEN];

} T_ChatMsg;

/* should be a multiple of 8! */
#define MAX_MODEL_NAME_LEN 48
/** Aircraft position message */
typedef struct {

    /** Name of the aircraft model */
    char sModel[MAX_MODEL_NAME_LEN];

    /** Position data for the aircraft */
    sgdVec3 PlayerPosition;
    sgQuat PlayerOrientation;

} T_PositionMsg;


#endif


