// multiplayrxmgr.hxx -- routines for receiving multiplayer data
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

#ifndef MULTIPLAYRXMGR_H
#define MULTIPLAYRXMGR_H

#define MULTIPLAYRXMGR_HID "$Id$"


#include "mpplayer.hxx"
#include "mpmessages.hxx"

#include STL_STRING
SG_USING_STD(string);

#include <simgear/compiler.h>
#include <plib/netSocket.h>

// Maximum number of players that can exist at any time
#define MAX_PLAYERS 10

/****************************************************************
* @version $Id$
*
* Description: The multiplay rx manager is responsible for
* receiving and processing data from other players.

* Data from remote players is read from the network and processed
* via calling ProcessData. The models for the remote player are
* positioned onto the scene by calling Update.
*
*******************************************************************/
class FGMultiplayRxMgr {
public:

    /** Constructor */
    FGMultiplayRxMgr();

    /** Destructor. */
    ~FGMultiplayRxMgr();

    /** Initialises the multiplayer receiver.
    * @return True if initialisation succeeds, else false
    */
    bool init(void);

    /** Initiates processing of any data waiting at the rx socket.
    */
    void ProcessData(void);
    void ProcessPosMsg ( const char *Msg );
    void ProcessChatMsg ( const char *Msg );

    /** Updates the model positions for the players
    */
    void Update(void);

    /** Closes the multiplayer manager. Stops any further player packet processing.
    */
    void Close(void);


private:


    /** Holds the players that exist in the game */
    MPPlayer *m_Player[MAX_PLAYERS];

    /** Socket for receiving data from the server or another player */
    netSocket *mDataRxSocket;

    /** True if multiplay receive is initialised */
    bool m_bInitialised;

    /** Receive address for multiplayer messages */
    string m_sRxAddress;

    /** Receive port for multiplayer messages */
    int m_iRxPort;

    /** Local player's callsign */
    string m_sCallsign;

};

#endif



