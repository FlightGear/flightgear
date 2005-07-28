// mpplayer.cxx -- routines for a player within a multiplayer Flightgear
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
# include <netdb.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
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


/******************************************************************
* Name: MPPlayer
* Description: Constructor.
******************************************************************/
MPPlayer::MPPlayer() {

    // Initialise private members
    m_bInitialised = false;
    m_LastUpdate = 0;
    m_PlayerAddress.set("localhost", 0);
    m_sCallsign = "none";


}


/******************************************************************
* Name: ~MPPlayer
* Description: Destructor.
******************************************************************/
MPPlayer::~MPPlayer() {

    Close();

}


/******************************************************************
* Name: Open
* Description: Initialises class.
******************************************************************/
bool MPPlayer::Open(const string &sAddress, const int &iPort, const string &sCallsign, const string &sModelName, bool bLocalPlayer) {

    bool bSuccess = true;

    if (!m_bInitialised) {

        m_PlayerAddress.set(sAddress.c_str(), iPort);
        m_sCallsign = sCallsign;
        m_sModelName = sModelName;
        m_bLocalPlayer = bLocalPlayer;

        // If the player is remote then load the model
        if (!bLocalPlayer) {
             try {
                 LoadModel();
             } catch (...) {
                 SG_LOG( SG_NETWORK, SG_ALERT, "Failed to load remote model '" << sModelName << "'." );
                 return false;
             }
        }

        m_bInitialised = bSuccess;

    } else {
        SG_LOG( SG_NETWORK, SG_ALERT, "MPPlayer::Open - Attempt to open an already open player connection." );
        bSuccess = false;
    }


    /* Return true if open succeeds */
    return bSuccess;

}


/******************************************************************
* Name: Close
* Description: Resets the object.
******************************************************************/
void MPPlayer::Close(void) {

    // Remove the model from the game
    if (m_bInitialised && !m_bLocalPlayer) {

        // Disconnect the model from the transform, then the transform from the scene.
        m_ModelTrans->removeKid(m_Model);
        globals->get_scenery()->unregister_placement_transform(m_ModelTrans);
        globals->get_scenery()->get_aircraft_branch()->removeKid( m_ModelTrans);

        // Flush the model loader so that it erases the model from its list of
        // models.
        globals->get_model_lib()->flush1();

        // Assume that plib/ssg deletes the model and transform as their
        // refcounts should be zero.

    }

    m_bInitialised = false;
    m_bUpdated = false;
    m_LastUpdate = 0;
    m_sCallsign = "none";

}


/******************************************************************
* Name: SetPosition
* Description: Updates position data held for this player and resets
* the last update time.
******************************************************************/
void MPPlayer::SetPosition(const sgQuat PlayerOrientation,
                           const sgdVec3 PlayerPosition) {


    // Save the position matrix and update time
    if (m_bInitialised) {
        sgdCopyVec3(m_ModelPosition, PlayerPosition);
        sgCopyVec4(m_ModelOrientation, PlayerOrientation);
        time(&m_LastUpdate);
        m_bUpdated = true;
    }


}


/******************************************************************
* Name: Draw
* Description: Updates the position for the player's model
* The state of the player's data is returned.
******************************************************************/
MPPlayer::TPlayerDataState MPPlayer::Draw(void) {

    MPPlayer::TPlayerDataState eResult = PLAYER_DATA_NOT_AVAILABLE;

    if (m_bInitialised && !m_bLocalPlayer) {
        if ((time(NULL) - m_LastUpdate < TIME_TO_LIVE)) {
            // Peform an update if it has changed since the last update
            if (m_bUpdated) {

                // Transform and update player model
                sgMat4 orMat;
                sgMakeIdentMat4(orMat);
                sgQuatToMatrix(orMat, m_ModelOrientation);
                m_ModelTrans->setTransform(m_ModelPosition, orMat);

                eResult = PLAYER_DATA_AVAILABLE;

                // Clear the updated flag so that the position data
                // is only available if it has changed
                m_bUpdated = false;
            }

        // Data has not been updated for some time.
        } else {
            eResult = PLAYER_DATA_EXPIRED;
        }

    }

    return eResult;

}


/******************************************************************
* Name: Callsign
* Description: Returns the player's callsign.
******************************************************************/
string MPPlayer::Callsign(void) const {

    return m_sCallsign;

}


/******************************************************************
* Name: CompareCallsign
* Description: Returns true if the player's callsign matches
* the given callsign.
******************************************************************/
bool MPPlayer::CompareCallsign(const char *sCallsign) const {

    return (m_sCallsign == sCallsign);

}


/******************************************************************
* Name: LoadModel
* Description: Loads the player's aircraft model.
******************************************************************/
void MPPlayer::LoadModel(void) {


    m_ModelTrans = new ssgPlacementTransform;

    // Load the model
    m_Model = globals->get_model_lib()->load_model( globals->get_fg_root(),
                                                    m_sModelName,
                                                    globals->get_props(),
                                                    globals->get_sim_time_sec() );
    m_Model->clrTraversalMaskBits( SSGTRAV_HOT );

    // Add model to transform
    m_ModelTrans->addKid( m_Model );

    // Optimise model and transform
    ssgFlatten( m_Model );
    ssgStripify( m_ModelTrans );

    // Place on scene under aircraft branch
    globals->get_scenery()->get_aircraft_branch()->addKid( m_ModelTrans );
    globals->get_scenery()->register_placement_transform( m_ModelTrans);


}


/******************************************************************
* Name: FillPosMsg
* Description: Populates the header and data for a position message.
******************************************************************/
void MPPlayer::FillPosMsg(T_MsgHdr *MsgHdr, T_PositionMsg *PosMsg) {

    FillMsgHdr(MsgHdr, POS_DATA_ID);

    strncpy(PosMsg->sModel, m_sModelName.c_str(), MAX_MODEL_NAME_LEN);
    PosMsg->sModel[MAX_MODEL_NAME_LEN - 1] = '\0';
    sgdCopyVec3(PosMsg->PlayerPosition, m_ModelPosition);
    sgCopyQuat(PosMsg->PlayerOrientation, m_ModelOrientation);


}


/******************************************************************
* Name: FillMsgHdr
* Description: Populates the header of a multiplayer message.
******************************************************************/
void MPPlayer::FillMsgHdr(T_MsgHdr *MsgHdr, const int iMsgId) {

    struct in_addr address;

    MsgHdr->MsgId = iMsgId;

    switch (iMsgId) {
        case CHAT_MSG_ID:
            MsgHdr->iMsgLen = sizeof(T_MsgHdr) + sizeof(T_ChatMsg);
            break;
        case POS_DATA_ID:
            MsgHdr->iMsgLen = sizeof(T_MsgHdr) + sizeof(T_PositionMsg);
            break;
        default:
            MsgHdr->iMsgLen = sizeof(T_MsgHdr);
            break;
    }

    address.s_addr = inet_addr( m_PlayerAddress.getHost() );
    MsgHdr->lReplyAddress = address.s_addr;

    MsgHdr->iReplyPort = m_PlayerAddress.getPort();

    strncpy(MsgHdr->sCallsign, m_sCallsign.c_str(), MAX_CALLSIGN_LEN);
    MsgHdr->sCallsign[MAX_CALLSIGN_LEN - 1] = '\0';


}

#endif // FG_MPLAYER_AS

