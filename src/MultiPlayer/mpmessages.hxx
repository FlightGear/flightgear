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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

#include <vector>

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/math/SGMath.hxx>
#include "tiny_xdr.hxx"

// magic value for messages
const uint32_t MSG_MAGIC = 0x46474653;  // "FGFS"
// protocoll version
const uint32_t PROTO_VER = 0x00010001;  // 1.1

// Message identifiers
#define CHAT_MSG_ID             1
#define UNUSABLE_POS_DATA_ID    2
#define OLD_OLD_POS_DATA_ID     3
#define OLD_POS_DATA_ID         4
#define OLD_PROP_MSG_ID         5
#define RESET_DATA_ID           6
#define POS_DATA_ID             7

// XDR demands 4 byte alignment, but some compilers use8 byte alignment
// so it's safe to let the overall size of a network message be a 
// multiple of 8!
#define MAX_CALLSIGN_LEN        8
#define MAX_CHAT_MSG_LEN        256
#define MAX_MODEL_NAME_LEN      96
#define MAX_PROPERTY_LEN        52

// Header for use with all messages sent 
struct T_MsgHdr {
    xdr_data_t  Magic;                  // Magic Value
    xdr_data_t  Version;                // Protocoll version
    xdr_data_t  MsgId;                  // Message identifier 
    xdr_data_t  MsgLen;                 // absolute length of message
    xdr_data_t  ReplyAddress;           // (player's receiver address
    xdr_data_t  ReplyPort;              // player's receiver port
    char Callsign[MAX_CALLSIGN_LEN];    // Callsign used by the player
};

// Chat message 
struct T_ChatMsg {
    char Text[MAX_CHAT_MSG_LEN];       // Text of chat message
};

// Position message
struct T_PositionMsg {
    char Model[MAX_MODEL_NAME_LEN];    // Name of the aircraft model

    // Time when this packet was generated
    xdr_data2_t time;
    xdr_data2_t lag;

    // position wrt the earth centered frame
    xdr_data2_t position[3];
    // orientation wrt the earth centered frame, stored in the angle axis
    // representation where the angle is coded into the axis length
    xdr_data_t orientation[3];

    // linear velocity wrt the earth centered frame measured in
    // the earth centered frame
    xdr_data_t linearVel[3];
    // angular velocity wrt the earth centered frame measured in
    // the earth centered frame
    xdr_data_t angularVel[3];

    // linear acceleration wrt the earth centered frame measured in
    // the earth centered frame
    xdr_data_t linearAccel[3];
    // angular acceleration wrt the earth centered frame measured in
    // the earth centered frame
    xdr_data_t angularAccel[3];
    // Padding. The alignment is 8 bytes on x86_64 because there are
    // 8-byte types in the message, so the size should be explicitly
    // rounded out to a multiple of 8. Of course, it's a bad idea to
    // put a C struct directly on the wire, but that's a fight for
    // another day...
    xdr_data_t pad;
};

struct FGPropertyData {
  unsigned id;
  
  // While the type isn't transmitted, it is needed for the destructor
  simgear::props::Type type;
  union { 
    int int_value;
    float float_value;
    char* string_value;
  }; 
  
  ~FGPropertyData() {
    if ((type == simgear::props::STRING) || (type == simgear::props::UNSPECIFIED))
    {
      delete [] string_value;
    }
  }
};



// Position message
struct FGExternalMotionData {
  // simulation time when this packet was generated
  double time;
  // the artificial lag the client should stay behind the average
  // simulation time to arrival time difference
  // FIXME: should be some 'per model' instead of 'per packet' property
  double lag;
  
  // position wrt the earth centered frame
  SGVec3d position;
  // orientation wrt the earth centered frame
  SGQuatf orientation;
  
  // linear velocity wrt the earth centered frame measured in
  // the earth centered frame
  SGVec3f linearVel;
  // angular velocity wrt the earth centered frame measured in
  // the earth centered frame
  SGVec3f angularVel;
  
  // linear acceleration wrt the earth centered frame measured in
  // the earth centered frame
  SGVec3f linearAccel;
  // angular acceleration wrt the earth centered frame measured in
  // the earth centered frame
  SGVec3f angularAccel;
  
  // The set of properties received for this timeslot
  std::vector<FGPropertyData*> properties;

  ~FGExternalMotionData()
  {
      std::vector<FGPropertyData*>::const_iterator propIt;
      std::vector<FGPropertyData*>::const_iterator propItEnd;
      propIt = properties.begin();
      propItEnd = properties.end();

      while (propIt != propItEnd)
      {
        delete *propIt;
        propIt++;
      }
  }
};

#endif
