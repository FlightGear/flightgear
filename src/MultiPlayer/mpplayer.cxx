//////////////////////////////////////////////////////////////////////
//
// mpplayer.cxx -- routines for a player within a multiplayer Flightgear
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
//////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef FG_MPLAYER_AS

/******************************************************************
* $Id$
*
* Description: Provides a container for a player in a multiplayer
* game. The players network address, model, callsign and positoin
* are held. When the player is created and open called, the player's
* model is loaded onto the scene. The position transform matrix
* is updated by calling SetPosition. When Draw is called the
* elapsed time since the last update is checked. If the model
* position information has been updated in the last TIME_TO_LIVE
* seconds then the model position is updated on the scene.
*
******************************************************************/

#include "mpplayer.hxx"

#include <stdlib.h>
#if !(defined(_MSC_VER) || defined(__MINGW32__))
#   include <netdb.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#endif
#include <plib/netSocket.h>
#include <plib/sg.h>

#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/model/placementtrans.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>


// These constants are provided so that the ident command can list file versions.
const char sMPPLAYER_BID[] = "$Id$";
const char sMPPLAYER_HID[] = MPPLAYER_HID;

//////////////////////////////////////////////////////////////////////
//
//  constructor
//
//////////////////////////////////////////////////////////////////////
MPPlayer::MPPlayer()
{
    m_Initialised   = false;
    m_LastUpdate    = 0;
    m_Callsign      = "none";
    m_PlayerAddress.set("localhost", 0);
} // MPPlayer::MPPlayer()
//////////////////////////////////////////////////////////////////////

/******************************************************************
* Name: ~MPPlayer
* Description: Destructor.
******************************************************************/
MPPlayer::~MPPlayer() 
{
    Close();
}

/******************************************************************
* Name: Open
* Description: Initialises class.
******************************************************************/
bool 
MPPlayer::Open
    (
    const string &Address,
    const int &Port,
    const string &Callsign,
    const string &ModelName,
    bool LocalPlayer
    )
{
    bool Success = true;

    if (!m_Initialised)
    {
        m_PlayerAddress.set(Address.c_str(), Port);
        m_Callsign = Callsign;
        m_ModelName = ModelName;
        m_LocalPlayer = LocalPlayer;
        SG_LOG( SG_NETWORK, SG_ALERT, "Initialising " << m_Callsign
           << " using '" << m_ModelName << "'" );
        // If the player is remote then load the model
        if (!LocalPlayer)
        {
             try {
                 LoadModel();
             } catch (...) {
                 SG_LOG( SG_NETWORK, SG_ALERT,
                   "Failed to load remote model '" << ModelName << "'." );
                 return false;
             }
        }
        m_Initialised = Success;
    }
    else
    {
        SG_LOG( SG_NETWORK, SG_ALERT, "MPPlayer::Open - "
          << "Attempt to open an already opened player connection." );
        Success = false;
    }
    /* Return true if open succeeds */
    return Success;
}

/******************************************************************
* Name: Close
* Description: Resets the object.
******************************************************************/
void
MPPlayer::Close(void)
{
    // Remove the model from the game
    if (m_Initialised && !m_LocalPlayer) 
    {
        // Disconnect the model from the transform,
        // then the transform from the scene.
        m_ModelTrans->removeKid(m_Model);
        globals->get_scenery()->unregister_placement_transform(m_ModelTrans);
        globals->get_scenery()->get_aircraft_branch()->removeKid( m_ModelTrans);
        // Flush the model loader so that it erases the model from its list of
        // models.
        // globals->get_model_lib()->flush1();
        // Assume that plib/ssg deletes the model and transform as their
        // refcounts should be zero.
    }
    m_Initialised   = false;
    m_Updated       = false;
    m_LastUpdate    = 0;
    m_Callsign      = "none";
}

/******************************************************************
* Name: SetPosition
* Description: Updates position data held for this player and resets
* the last update time.
******************************************************************/
void
MPPlayer::SetPosition
    (
    const sgQuat PlayerOrientation,
    const sgdVec3 PlayerPosition
    )
{
    // Save the position matrix and update time
    if (m_Initialised)
    {
        sgdCopyVec3(m_ModelPosition, PlayerPosition);
        sgCopyVec4(m_ModelOrientation, PlayerOrientation);
        time(&m_LastUpdate);
        m_Updated = true;
    }
}

/******************************************************************
* Name: Draw
* Description: Updates the position for the player's model
* The state of the player's data is returned.
******************************************************************/
MPPlayer::TPlayerDataState
MPPlayer::Draw (void)
{
    MPPlayer::TPlayerDataState eResult = PLAYER_DATA_NOT_AVAILABLE;
    if (m_Initialised && !m_LocalPlayer) 
    {
        if ((time(NULL) - m_LastUpdate < TIME_TO_LIVE))
        {
            // Peform an update if it has changed since the last update
            if (m_Updated)
            {
                // Transform and update player model
                sgMat4 orMat;
                sgMakeIdentMat4(orMat);
                sgQuatToMatrix(orMat, m_ModelOrientation);
                m_ModelTrans->setTransform(m_ModelPosition, orMat);
                eResult = PLAYER_DATA_AVAILABLE;
                // Clear the updated flag so that the position data
                // is only available if it has changed
                m_Updated = false;
            }
        }
        else
        {   // Data has not been updated for some time.
            eResult = PLAYER_DATA_EXPIRED;
        }
    }
    return eResult;
}

/******************************************************************
* Name: Callsign
* Description: Returns the player's callsign.
******************************************************************/
string
MPPlayer::Callsign(void) const
{
    return m_Callsign;
}

/******************************************************************
* Name: CompareCallsign
* Description: Returns true if the player's callsign matches
* the given callsign.
******************************************************************/
bool
MPPlayer::CompareCallsign(const char *Callsign) const 
{
    return (m_Callsign == Callsign);
}

/******************************************************************
* Name: LoadModel
* Description: Loads the player's aircraft model.
******************************************************************/
void
MPPlayer::LoadModel (void)
{
    m_ModelTrans = new ssgPlacementTransform;
    // Load the model
    m_Model = globals->get_model_lib()->load_model( globals->get_fg_root(),
      m_ModelName, globals->get_props(), globals->get_sim_time_sec() );
    m_Model->clrTraversalMaskBits( SSGTRAV_HOT );
    // Add model to transform
    m_ModelTrans->addKid( m_Model );
    // Place on scene under aircraft branch
    globals->get_scenery()->get_aircraft_branch()->addKid( m_ModelTrans );
    globals->get_scenery()->register_placement_transform( m_ModelTrans);
}

/******************************************************************
* Name: FillPosMsg
* Description: Populates the header and data for a position message.
******************************************************************/
void
MPPlayer::FillPosMsg
    (
    T_MsgHdr *MsgHdr,
    T_PositionMsg *PosMsg
    )
{
    FillMsgHdr(MsgHdr, POS_DATA_ID);
    strncpy(PosMsg->Model, m_ModelName.c_str(), MAX_MODEL_NAME_LEN);
    PosMsg->Model[MAX_MODEL_NAME_LEN - 1] = '\0';
    PosMsg->PlayerPosition[0] = XDR_encode_double (m_ModelPosition[0]);
    PosMsg->PlayerPosition[1] = XDR_encode_double (m_ModelPosition[1]);
    PosMsg->PlayerPosition[2] = XDR_encode_double (m_ModelPosition[2]);
    PosMsg->PlayerOrientation[0] = XDR_encode_float (m_ModelOrientation[0]);
    PosMsg->PlayerOrientation[1] = XDR_encode_float (m_ModelOrientation[1]);
    PosMsg->PlayerOrientation[2] = XDR_encode_float (m_ModelOrientation[2]);
    PosMsg->PlayerOrientation[3] = XDR_encode_float (m_ModelOrientation[3]);
}

/******************************************************************
* Name: FillMsgHdr
* Description: Populates the header of a multiplayer message.
******************************************************************/
void
MPPlayer::FillMsgHdr
    (
    T_MsgHdr *MsgHdr, 
    const int MsgId
    )
{
    uint32_t        len;

    switch (MsgId)
    {
        case CHAT_MSG_ID:
            len = sizeof(T_MsgHdr) + sizeof(T_ChatMsg);
            break;
        case POS_DATA_ID:
            len = sizeof(T_MsgHdr) + sizeof(T_PositionMsg);
            break;
        default:
            len = sizeof(T_MsgHdr);
            break;
    }
    MsgHdr->Magic           = XDR_encode_uint32 (MSG_MAGIC);
    MsgHdr->Version         = XDR_encode_uint32 (PROTO_VER);
    MsgHdr->MsgId           = XDR_encode_uint32 (MsgId);
    MsgHdr->MsgLen          = XDR_encode_uint32 (len);
    // inet_addr returns address in network byte order
    // no need to encode it
    MsgHdr->ReplyAddress   = inet_addr( m_PlayerAddress.getHost() );
    MsgHdr->ReplyPort      = XDR_encode_uint32 (m_PlayerAddress.getPort());
    strncpy(MsgHdr->Callsign, m_Callsign.c_str(), MAX_CALLSIGN_LEN);
    MsgHdr->Callsign[MAX_CALLSIGN_LEN - 1] = '\0';
}

#endif // FG_MPLAYER_AS

