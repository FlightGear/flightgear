// multiplayrxmgr.cxx -- routines for receiving multiplayer data
//                       for Flightgear
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef FG_MPLAYER_AS

/******************************************************************
* $Id$
*
* Description: The multiplayer rx manager provides control over
* multiplayer data reception and processing for an interactive
* multiplayer FlightGear simulation.
*
* The objects that hold player information are accessed via
* a fixed size array. A fixed array is used since it provides
* speed benefits over working with linked lists and is easier
* to code. Also, there is no point allowing for an unlimited
* number of players as too many players will slow the game
* down to the point where it is unplayable.
*
* When player position data is received, the callsign of
* the player is checked against existing players. If the
* player does not exist, a new player is created in the
* next free slot of the player array. If the player does
* exist, the player's positional matrix is updated.
*
* The Update method is used to move the players on the
* scene. The return value from calling MPPlayer::Draw
* indicates the state of the player. If data for a player
* has not been received data for some time, the player object
* is deleted and the array element freed.
*
******************************************************************/

#include <sys/types.h>
#if !(defined(_MSC_VER) || defined(__MINGW32__))
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif
#include <plib/netSocket.h>
#include <stdlib.h>

#include <simgear/debug/logstream.hxx>
#include <Main/fg_props.hxx>

#include "multiplayrxmgr.hxx"
#include "mpmessages.hxx"
#include "mpplayer.hxx"

#define MAX_PACKET_SIZE 1024

// These constants are provided so that the ident command can list file versions.
const char sMULTIPLAYTXMGR_BID[] = "$Id$";
const char sMULTIPLAYRXMGR_HID[] = MULTIPLAYRXMGR_HID;



/******************************************************************
* Name: FGMultiplayRxMgr
* Description: Constructor.
******************************************************************/
FGMultiplayRxMgr::FGMultiplayRxMgr() {

    int iPlayerCnt;         // Count of players in player array

    // Initialise private members
    m_sRxAddress = "0";
    m_iRxPort = 0;
    m_bInitialised = false;

    // Clear the player array
    for (iPlayerCnt = 0; iPlayerCnt < MAX_PLAYERS; iPlayerCnt++) {
        m_Player[iPlayerCnt] = NULL;
    }

}


/******************************************************************
* Name: ~FGMultiplayRxMgr
* Description: Destructor. Closes and deletes objects owned by
* this object.
******************************************************************/
FGMultiplayRxMgr::~FGMultiplayRxMgr() {

    Close();

}


/******************************************************************
* Name: init
* Description: Initialises multiplayer receive.
******************************************************************/
bool FGMultiplayRxMgr::init(void) {

    bool bSuccess = true;                       // Result of initialisation

    // Initialise object if not already done
    if (!m_bInitialised) {

        // Set members from property values
        m_sCallsign = fgGetString("/sim/multiplay/callsign");
        m_sRxAddress = fgGetString("/sim/multiplay/rxhost");
        m_iRxPort = fgGetInt("/sim/multiplay/rxport");

        SG_LOG( SG_NETWORK, SG_INFO, "FGMultiplayRxMgr::init - rxaddress= "
                                     << m_sRxAddress );
        SG_LOG( SG_NETWORK, SG_INFO, "FGMultiplayRxMgr::init - rxport= "
                                     << m_iRxPort);
        SG_LOG( SG_NETWORK, SG_INFO, "FGMultiplayRxMgr::init - callsign= "
                                     << m_sCallsign );


        // Create and open rx socket
        mDataRxSocket = new netSocket();
        if (!mDataRxSocket->open(false)) {
            // Failed to open rx socket
            SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayRxMgr::Open - Failed to create data receive socket" );
            bSuccess = false;
        } else {

            // Configure the socket
            mDataRxSocket->setBlocking(false);
            mDataRxSocket->setBroadcast(true);
            if (mDataRxSocket->bind(m_sRxAddress.c_str(), m_iRxPort) != 0) {
                perror("bind");
                // Failed to bind
                SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayRxMgr::Open - Failed to bind receive socket" );
                bSuccess = false;
            }

        }

        // Save manager state
        m_bInitialised = bSuccess;

    } else {
        SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayRxMgr::OpenRx - Receiver open requested when receiver is already open" );
        bSuccess = false;
    }

    /* Return true if open succeeds */
    return bSuccess;

}


/******************************************************************
* Name: Close
* Description: Closes and deletes and player connections. Closes
* and deletes the rx socket. Resets the object state
* to unitialised.
******************************************************************/
void FGMultiplayRxMgr::Close(void) {

    int iPlayerCnt;         // Count of players in player array

    // Delete any existing players
    for (iPlayerCnt = 0; iPlayerCnt < MAX_PLAYERS; iPlayerCnt++) {
        if (m_Player[iPlayerCnt] != NULL) {
            delete m_Player[iPlayerCnt];
            m_Player[iPlayerCnt] = NULL;
        }
    }

    // Delete socket
    if (mDataRxSocket) {
        mDataRxSocket->close();
        delete mDataRxSocket;
        mDataRxSocket = NULL;
    }

    m_bInitialised = false;

}


/******************************************************************
* Name: ProcessData
* Description: Processes data waiting at the receive socket. The
* processing ends when there is no more data at the socket.
******************************************************************/
void FGMultiplayRxMgr::ProcessData(void) {

    char sMsg[MAX_PACKET_SIZE];         // Buffer for received message
    int iBytes;                         // Bytes received
    T_MsgHdr *MsgHdr;                   // Pointer to header in received data
    T_ChatMsg *ChatMsg;                 // Pointer to chat message in received data
    T_PositionMsg *PosMsg;              // Pointer to position message in received data
    char *sIpAddress;                   // Address information from header
    char *sModelName;                   // Model that the remote player is using
    char *sCallsign;                    // Callsign of the remote player
    struct in_addr PlayerAddress;       // Used for converting the player's address into dot notation
    int iPlayerCnt;                     // Count of players in player array
    bool bActivePlayer = false;         // The state of the player that sent the data
    int iPort;                          // Port that the remote player receives on


    if (m_bInitialised) {

        // Read the receive socket and process any data
        do {

            // Although the recv call asks for MAX_PACKET_SIZE of data,
            // the number of bytes returned will only be that of the next
            // packet waiting to be processed.
            iBytes = mDataRxSocket->recv(sMsg, MAX_PACKET_SIZE, 0);

            // Data received
            if (iBytes > 0) {
                if (iBytes >= (int)sizeof(MsgHdr)) {

                    // Read header
                    MsgHdr = (T_MsgHdr *)sMsg;
                    PlayerAddress.s_addr = MsgHdr->lReplyAddress;
                    sIpAddress = inet_ntoa(PlayerAddress);
                    iPort = MsgHdr->iReplyPort;
                    sCallsign = MsgHdr->sCallsign;

                    // Process the player data unless we generated it
                    if (m_sCallsign != MsgHdr->sCallsign) {


                        // Process messages
                        switch(MsgHdr->MsgId) {
                            case CHAT_MSG_ID:
                                if (MsgHdr->iMsgLen == sizeof(T_MsgHdr) + sizeof(T_ChatMsg)) {
                                    ChatMsg = (T_ChatMsg *)(sMsg + sizeof(T_MsgHdr));
                                    SG_LOG( SG_NETWORK, SG_BULK, "Chat [" << MsgHdr->sCallsign << "]" << " " << ChatMsg->sText );
                                } else {
                                    SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayRxMgr::MP_ProcessData - Chat message received with insufficient data" );
                                }
                                break;

                            case POS_DATA_ID:
                                if (MsgHdr->iMsgLen == sizeof(T_MsgHdr) + sizeof(T_PositionMsg)) {
                                    PosMsg = (T_PositionMsg *)(sMsg + sizeof(T_MsgHdr));
                                    sModelName = PosMsg->sModel;

                                    // Check if the player is already in the game by using the Callsign.
                                    bActivePlayer = false;
                                    for (iPlayerCnt = 0; iPlayerCnt < MAX_PLAYERS; iPlayerCnt++) {
                                        if (m_Player[iPlayerCnt] != NULL) {
                                            if (m_Player[iPlayerCnt]->CompareCallsign(MsgHdr->sCallsign)) {

                                                // Player found. Update the data for the player.
                                                m_Player[iPlayerCnt]->SetPosition(PosMsg->PlayerOrientation, PosMsg->PlayerPosition);
                                                bActivePlayer = true;

                                            }
                                        }
                                    }

                                    // Player not active, so add as new player
                                    if (!bActivePlayer) {
                                        iPlayerCnt = 0;
                                        do {
                                            if (m_Player[iPlayerCnt] == NULL) {
                                                SG_LOG( SG_NETWORK, SG_INFO, "FGMultiplayRxMgr::ProcessRxData - Add new player. IP: " << sIpAddress << ", Call: " <<  sCallsign << ", model: " << sModelName );
                                                m_Player[iPlayerCnt] = new MPPlayer;
                                                m_Player[iPlayerCnt]->Open(sIpAddress, iPort, sCallsign, sModelName, false);
                                                m_Player[iPlayerCnt]->SetPosition(PosMsg->PlayerOrientation, PosMsg->PlayerPosition);
                                                bActivePlayer = true;
                                            }
                                            iPlayerCnt++;
                                        } while (iPlayerCnt < MAX_PLAYERS && !bActivePlayer);

                                        // Check if the player was added
                                        if (!bActivePlayer) {
                                            if (iPlayerCnt == MAX_PLAYERS) {
                                                SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayRxMgr::MP_ProcessData - Unable to add new player (" << sCallsign << "). Too many players." );
                                            }
                                        }
                                    }

                                } else {
                                    SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayRxMgr::MP_ProcessData - Position message received with insufficient data" );
                                }
                                break;

                            default:
                                SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayRxMgr::MP_ProcessData - Unknown message Id received: " << MsgHdr->MsgId );
                                break;


                        }
                    }
                }


            // Error or no data
            } else if (iBytes == -1) {
                if (errno != EAGAIN) {
                    perror("FGMultiplayRxMgr::MP_ProcessData");
                }
            }

        } while (iBytes > 0);

    }


}


/******************************************************************
* Name: Update
* Description: For each active player, tell the player object
* to update its position on the scene. If a player object
* returns status information indicating that it has not
* had an update for some time then the player is deleted.
******************************************************************/
void FGMultiplayRxMgr::Update(void) {

    MPPlayer::TPlayerDataState ePlayerDataState;
    int iPlayerId;

    for (iPlayerId = 0; iPlayerId < MAX_PLAYERS; iPlayerId++) {
        if (m_Player[iPlayerId] != NULL) {
            ePlayerDataState = m_Player[iPlayerId]->Draw();

            // If the player has not received an update for some
            // time then assume that the player has quit.
            if (ePlayerDataState == MPPlayer::PLAYER_DATA_EXPIRED) {
                SG_LOG( SG_NETWORK, SG_BULK, "FGMultiplayRxMgr::Update - Deleting player from game. Callsign: "
                        << m_Player[iPlayerId]->Callsign() );
                delete m_Player[iPlayerId];
                m_Player[iPlayerId] = NULL;
            }

        }
    }

}

#endif // FG_MPLAYER_AS

