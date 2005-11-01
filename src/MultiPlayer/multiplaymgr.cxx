//////////////////////////////////////////////////////////////////////
//
// multiplaymgr.hpp
//
// Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
//
// Copyright (C) 2003  Airservices Australia
// Copyright (C) 2005  Oliver Schroeder
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
// $Id$
//  
//////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef FG_MPLAYER_AS

#include <sys/types.h>
#if !(defined(_MSC_VER) || defined(__MINGW32__))
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#endif
#include <stdlib.h>

#include <plib/netSocket.h>

#include <simgear/debug/logstream.hxx>

#include <Main/fg_props.hxx>
#include "multiplaymgr.hxx"
#include "mpmessages.hxx"
#include "mpplayer.hxx"

#define MAX_PACKET_SIZE 1024

// These constants are provided so that the ident 
// command can list file versions
const char sMULTIPLAYMGR_BID[] = 
    "$Id$";
const char sMULTIPLAYMGR_HID[] = MULTIPLAYTXMGR_HID;

//////////////////////////////////////////////////////////////////////
//
//  MultiplayMgr constructor
//
//////////////////////////////////////////////////////////////////////
FGMultiplayMgr::FGMultiplayMgr() 
{
    m_Initialised   = false;
    m_LocalPlayer   = NULL;
    m_RxAddress     = "0";
    m_RxPort        = 0;
    m_Initialised   = false;
    m_HaveServer    = false;
} // FGMultiplayMgr::FGMultiplayMgr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  MultiplayMgr destructor
//
//////////////////////////////////////////////////////////////////////
FGMultiplayMgr::~FGMultiplayMgr() 
{
    Close();
} // FGMultiplayMgr::~FGMultiplayMgr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Initialise object
//
//////////////////////////////////////////////////////////////////////
bool
FGMultiplayMgr::init (void) 
{
    string  TxAddress;      // Destination address
    int     TxPort;

    //////////////////////////////////////////////////
    //  Initialise object if not already done
    //////////////////////////////////////////////////
    if (m_Initialised) 
    {
        SG_LOG( SG_NETWORK, SG_WARN,
          "FGMultiplayMgr::init - already initialised" );
        return (false);
    }
    //////////////////////////////////////////////////
    //  Set members from property values
    //////////////////////////////////////////////////
    TxAddress      = fgGetString   ("/sim/multiplay/txhost");
    TxPort         = fgGetInt      ("/sim/multiplay/txport");
    m_Callsign     = fgGetString   ("/sim/multiplay/callsign");
    m_RxAddress    = fgGetString   ("/sim/multiplay/rxhost");
    m_RxPort       = fgGetInt      ("/sim/multiplay/rxport");
    if (m_RxPort <= 0)
    {
        m_RxPort = 5000;
    }
    if (m_Callsign == "")
    {
        // FIXME: use getpwuid
        m_Callsign = "JohnDoe"; 
    }
    if (m_RxAddress == "")
    {
        m_RxAddress = "127.0.0.1";
    }
    if ((TxPort > 0) && (TxAddress != ""))
    {
        m_HaveServer = true;
        m_Server.set (TxAddress.c_str(), TxPort); 
    }
    SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-txaddress= "<<TxAddress);
    SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-txport= "<<TxPort );
    SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-rxaddress="<<m_RxAddress );
    SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-rxport= "<<m_RxPort);
    SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-callsign= "<<m_Callsign);
    m_DataSocket = new netSocket();
    if (!m_DataSocket->open(false))
    {
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplayMgr::init - Failed to create data socket" );
        return (false);
    }
    m_DataSocket->setBlocking(false);
    m_DataSocket->setBroadcast(true);
    if (m_DataSocket->bind(m_RxAddress.c_str(), m_RxPort) != 0)
    {
        perror("bind");
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplayMgr::Open - Failed to bind receive socket" );
        return (false);
    }
    m_LocalPlayer = new MPPlayer();
    if (!m_LocalPlayer->Open(m_RxAddress, m_RxPort, m_Callsign,
                            fgGetString("/sim/model/path"), true)) 
    {
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplayMgr::init - Failed to create local player" );
        return (false);
    }
    m_Initialised = true;
    return (true);
} // FGMultiplayMgr::init()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Closes and deletes the local player object. Closes
//  and deletes the tx socket. Resets the object state to unitialised.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::Close (void) 
{
    //////////////////////////////////////////////////
    //  Delete local player
    //////////////////////////////////////////////////
    if (m_LocalPlayer) 
    {
        delete m_LocalPlayer;
        m_LocalPlayer = NULL;
    }
    //////////////////////////////////////////////////
    //  Delete any existing players
    //////////////////////////////////////////////////
    t_MPClientListIterator CurrentPlayer;
    t_MPClientListIterator P;
    CurrentPlayer  = m_MPClientList.begin ();
    while (CurrentPlayer != m_MPClientList.end ())
    {
        P = CurrentPlayer;
        delete (*P);
        *P = 0;
        CurrentPlayer = m_MPClientList.erase (P);
    }
    //////////////////////////////////////////////////
    //  Delete socket
    //////////////////////////////////////////////////
    if (m_DataSocket) 
    {
        m_DataSocket->close();
        delete m_DataSocket;
        m_DataSocket = NULL;
    }
    m_Initialised = false;
} // FGMultiplayMgr::Close(void)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Description: Sends the position data for the local position.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::SendMyPosition
    (
    const sgQuat PlayerOrientation,
    const sgdVec3 PlayerPosition
    )
{
    T_MsgHdr        MsgHdr;
    T_PositionMsg   PosMsg;
    char Msg[sizeof(T_MsgHdr) + sizeof(T_PositionMsg)];

    if ((! m_Initialised) || (! m_HaveServer))
    {
        if (! m_Initialised)
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplayMgr::SendMyPosition - not initialised" );
        if (! m_HaveServer)
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplayMgr::SendMyPosition - no server" );
        return;
    }
    m_LocalPlayer->SetPosition(PlayerOrientation, PlayerPosition);
    m_LocalPlayer->FillPosMsg(&MsgHdr, &PosMsg);
    memcpy(Msg, &MsgHdr, sizeof(T_MsgHdr));
    memcpy(Msg + sizeof(T_MsgHdr), &PosMsg, sizeof(T_PositionMsg));
    m_DataSocket->sendto (Msg,
      sizeof(T_MsgHdr) + sizeof(T_PositionMsg), 0, &m_Server);
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
FGMultiplayMgr::SendTextMessage
    (
    const string &MsgText
    ) const
{
    T_MsgHdr    MsgHdr;
    T_ChatMsg   ChatMsg;
    unsigned int iNextBlockPosition = 0;
    char Msg[sizeof(T_MsgHdr) + sizeof(T_ChatMsg)];

    if ((! m_Initialised) || (! m_HaveServer))
    {
        return;
    }
    m_LocalPlayer->FillMsgHdr(&MsgHdr, CHAT_MSG_ID);
    //////////////////////////////////////////////////
    // Divide the text string into blocks that fit
    // in the message and send the blocks.
    //////////////////////////////////////////////////
    while (iNextBlockPosition < MsgText.length()) 
    {
        strncpy (ChatMsg.Text, 
          MsgText.substr(iNextBlockPosition, MAX_CHAT_MSG_LEN - 1).c_str(),
          MAX_CHAT_MSG_LEN);
        ChatMsg.Text[MAX_CHAT_MSG_LEN - 1] = '\0';
        memcpy (Msg, &MsgHdr, sizeof(T_MsgHdr));
        memcpy (Msg + sizeof(T_MsgHdr), &ChatMsg, sizeof(T_ChatMsg));
        m_DataSocket->sendto (Msg,
          sizeof(T_MsgHdr) + sizeof(T_ChatMsg), 0, &m_Server);
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
FGMultiplayMgr::ProcessData (void) 
{
    char        Msg[MAX_PACKET_SIZE];   // Buffer for received message
    int         Bytes;                  // Bytes received
    T_MsgHdr*   MsgHdr;                 // Pointer to header in received data
    netAddress  SenderAddress;

    if (! m_Initialised)
    {
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplayMgr::ProcessData - not initialised" );
        return;
    }
    //////////////////////////////////////////////////
    //  Read the receive socket and process any data
    //////////////////////////////////////////////////
    do {
        //////////////////////////////////////////////////
        //  Although the recv call asks for 
        //  MAX_PACKET_SIZE of data, the number of bytes
        //  returned will only be that of the next
        //  packet waiting to be processed.
        //////////////////////////////////////////////////
        Bytes = m_DataSocket->recvfrom (Msg, MAX_PACKET_SIZE, 0,
                                         &SenderAddress);
        //////////////////////////////////////////////////
        //  no Data received
        //////////////////////////////////////////////////
        if (Bytes <= 0)
        {
            if (errno != EAGAIN)
            {
                perror("FGMultiplayMgr::MP_ProcessData");
            }
            return;
        }
        if (Bytes <= (int)sizeof(MsgHdr)) 
        {
            SG_LOG( SG_NETWORK, SG_ALERT,
              "FGMultiplayMgr::MP_ProcessData - "
              << "received message with insufficient data" );
            return;
        }
        //////////////////////////////////////////////////
        //  Read header
        //////////////////////////////////////////////////
        MsgHdr = (T_MsgHdr *)Msg;
        MsgHdr->Magic       = XDR_decode_uint32 (MsgHdr->Magic);
        MsgHdr->Version     = XDR_decode_uint32 (MsgHdr->Version);
        MsgHdr->MsgId       = XDR_decode_uint32 (MsgHdr->MsgId);
        MsgHdr->MsgLen      = XDR_decode_uint32 (MsgHdr->MsgLen);
        MsgHdr->ReplyPort   = XDR_decode_uint32 (MsgHdr->ReplyPort);
        if (MsgHdr->Magic != MSG_MAGIC)
        {
            SG_LOG( SG_NETWORK, SG_ALERT,
              "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid magic number!" );
        }
        if (MsgHdr->Version != PROTO_VER) 
        {
            SG_LOG( SG_NETWORK, SG_ALERT,
              "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid protocoll number!" );
        }
        //////////////////////////////////////////////////
        //  Process the player data unless we generated it
        //////////////////////////////////////////////////
        if (m_Callsign == MsgHdr->Callsign) 
        {
            return;
        }
        //////////////////////////////////////////////////
        //  Process messages
        //////////////////////////////////////////////////
        switch(MsgHdr->MsgId)
        {
            case CHAT_MSG_ID:
                ProcessChatMsg ((char*) & Msg, SenderAddress);
                break;
            case POS_DATA_ID:
                ProcessPosMsg ((char*) & Msg, SenderAddress);
                break;
            default:
                SG_LOG( SG_NETWORK, SG_ALERT,
                  "FGMultiplayMgr::MP_ProcessData - "
                  << "Unknown message Id received: " 
                  << MsgHdr->MsgId );
                break;
        } // switch
    } while (Bytes > 0);
} // FGMultiplayMgr::ProcessData(void)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  handle a position message
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::ProcessPosMsg
    (
    const char *Msg,
    netAddress & SenderAddress
    )
{
    T_PositionMsg*  PosMsg;     // Pointer to position message in received data
    T_MsgHdr*       MsgHdr;     // Pointer to header in received data
    bool            ActivePlayer; 
    sgQuat          Orientation;
    sgdVec3         Position;
    struct in_addr  PlayerAddress;
    t_MPClientListIterator CurrentPlayer;
    int iPlayerCnt;
    char *sIpAddress;

    ActivePlayer = false;
    MsgHdr = (T_MsgHdr *)Msg;
    if (MsgHdr->MsgLen != sizeof(T_MsgHdr) + sizeof(T_PositionMsg))
    {
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplayMgr::MP_ProcessData - "
          << "Position message received with insufficient data" );
        return;
    }
    PosMsg = (T_PositionMsg *)(Msg + sizeof(T_MsgHdr));
    Position[0] = XDR_decode_double (PosMsg->PlayerPosition[0]);
    Position[1] = XDR_decode_double (PosMsg->PlayerPosition[1]);
    Position[2] = XDR_decode_double (PosMsg->PlayerPosition[2]);
    Orientation[0] = XDR_decode_float (PosMsg->PlayerOrientation[0]);
    Orientation[1] = XDR_decode_float (PosMsg->PlayerOrientation[1]);
    Orientation[2] = XDR_decode_float (PosMsg->PlayerOrientation[2]);
    Orientation[3] = XDR_decode_float (PosMsg->PlayerOrientation[3]);
    //////////////////////////////////////////////////
    //  Check if the player is already in the game 
    //  by using the Callsign
    //////////////////////////////////////////////////
    for (CurrentPlayer  = m_MPClientList.begin ();
         CurrentPlayer != m_MPClientList.end ();
         CurrentPlayer++)
    {
        if ((*CurrentPlayer)->CompareCallsign(MsgHdr->Callsign))
        {
            // Player found. Update the data for the player.
            (*CurrentPlayer)->SetPosition(Orientation, Position);
            ActivePlayer = true;
        }
    } // for (...)
    if (ActivePlayer) 
    {   // nothing more to do
        return;
    }
    //////////////////////////////////////////////////
    //  Player not active, so add as new player
    //////////////////////////////////////////////////
    MPPlayer* NewPlayer;
    NewPlayer = new MPPlayer;
    NewPlayer->Open(SenderAddress.getHost(), MsgHdr->ReplyPort,
        MsgHdr->Callsign, PosMsg->Model, false);
    NewPlayer->SetPosition(Orientation, Position);
    m_MPClientList.push_back (NewPlayer);
} // FGMultiplayMgr::ProcessPosMsg()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  handle a chat message
//  FIXME: display chat message withi flightgear
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::ProcessChatMsg
    (
    const char *Msg,
    netAddress & SenderAddress
    )
{
    T_ChatMsg*  ChatMsg;    // Pointer to chat message in received data
    T_MsgHdr*   MsgHdr;     // Pointer to header in received data

    MsgHdr = (T_MsgHdr *)Msg;
    if (MsgHdr->MsgLen != sizeof(T_MsgHdr) + sizeof(T_ChatMsg))
    {
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplayMgr::MP_ProcessData - "
          << "Chat message received with insufficient data" );
        return;
    }
    ChatMsg = (T_ChatMsg *)(Msg + sizeof(T_MsgHdr));
    SG_LOG ( SG_NETWORK, SG_ALERT, 
      "Chat [" << MsgHdr->Callsign << "]" << " " << ChatMsg->Text << endl);
} // FGMultiplayMgr::ProcessChatMsg ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  For each active player, tell the player object
//  to update its position on the scene. If a player object
//  returns status information indicating that it has not
//  had an update for some time then the player is deleted.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::Update (void) 
{
    MPPlayer::TPlayerDataState ePlayerDataState;
    t_MPClientListIterator CurrentPlayer;

    CurrentPlayer  = m_MPClientList.begin ();
    while (CurrentPlayer != m_MPClientList.end ())
    {
        ePlayerDataState = (*CurrentPlayer)->Draw();
        //////////////////////////////////////////////////
        // If the player has not received an update for
        // some time then assume that the player has quit.
        //////////////////////////////////////////////////
        if (ePlayerDataState == MPPlayer::PLAYER_DATA_EXPIRED) 
        {
            SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::Update - "
              << "Deleting player from game. Callsign: "
              << (*CurrentPlayer)->Callsign() );
            t_MPClientListIterator P;
	        P = CurrentPlayer;
	        delete (*P);
            *P = 0;
            CurrentPlayer = m_MPClientList.erase (P);
        }
        else    CurrentPlayer++;
    }
} // FGMultiplayMgr::Update()
//////////////////////////////////////////////////////////////////////

#endif // FG_MPLAYER_AS

