// multiplaytxmgr.hxx -- routines for transmitting multiplayer data
//                       for Flghtgear
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

#ifndef MULTIPLAYTXMGR_H
#define MULTIPLAYTXMGR_H

#define MULTIPLAYTXMGR_HID "$Id$"


#include "mpplayer.hxx"
#include "mpmessages.hxx"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include STL_STRING
SG_USING_STD(string);

#include <simgear/compiler.h>
#include <plib/netSocket.h>

// Maximum number of players that can exist at any time
#define MAX_PLAYERS 10

/****************************************************************
* @version $Id$
*
* Description: The multiplay tx manager is responsible for
* sending data to another player or a multiplayer server.
*
* The position information for the local player is transmitted
* on each call to SendMyPosition.
*
*******************************************************************/
class FGMultiplayTxMgr {
public:

    /** Constructor */
    FGMultiplayTxMgr();

    /** Destructor. */
    ~FGMultiplayTxMgr();

    /** Initialises the multiplayer transmitter.
    * @return True if initialisation succeeds, else false
    */
    bool init(void);

    /** Sends the position data for the local player
    * @param PlayerPosMat4 Transformation matrix for the player's position
    */
    void SendMyPosition(const sgMat4 PlayerPosMat4);
    
    /** Sends a tex chat message.
    * @param sMsgText Message text to send
    */
    void SendTextMessage(const string &sMsgText) const;

    /** Closes the multiplayer manager. Stops any further player packet processing.
    */
    void Close(void);


private:

    /** The local player */
    MPPlayer *mLocalPlayer;

    /** Socket for sending to the server or another player if playing point to point */
    netSocket *mDataTxSocket;

    /** True if multiplay transmit is initialised */
    bool m_bInitialised;

};

#endif



