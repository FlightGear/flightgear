//////////////////////////////////////////////////////////////////////
//
// mpplayer.cxx -- routines for a player within a multiplayer Flightgear
//
// Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
//
// With additions by Vivian Meazza, January 2006
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

#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/model/placementtrans.hxx>

#include <AIModel/AIBase.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/AIMultiplayer.hxx>
#include <Main/globals.hxx>
//#include <Scenery/scenery.hxx>

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
    m_LastUpdate    = 0;
    m_LastTime      = 0;
    m_LastUTime     = 0;
    m_Elapsed       = 0;
    m_TimeOffset    = 0.0;
    m_Callsign      = "none";
    m_PlayerAddress.set("localhost", 0);
    m_AIModel = NULL;
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
                 LoadAI();
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
        m_Initialised = 0;
        
        // get the model manager
        FGAIManager *aiModelMgr = (FGAIManager *) globals->get_subsystem("ai_model");
        if (!aiModelMgr) {
            SG_LOG( SG_NETWORK, SG_ALERT, "MPPlayer::Close - "
                    << "Cannot find AI model manager!" );
            return;
        }
        
        // remove it
        aiModelMgr->destroyObject(m_AIModel->getID());
        m_AIModel = NULL;
    }
    m_Initialised   = false;
    m_Updated       = false;
    m_LastUpdate    = 0;
    m_LastUpdate    = 0;
    m_LastTime      = 0;
    m_LastUTime     = 0;
    m_Elapsed       = 0;
    m_TimeOffset    = 0.0;
    m_Callsign      = "none";
}

/******************************************************************
* Name: CheckTime
* Description: Checks if the time is valid for a position update
* and perhaps sets the time offset
******************************************************************/
bool MPPlayer::CheckTime(int time, int timeusec)
{
    double curOffset;
    
    // set the offset
    struct timeval tv;
    int toff, utoff;
    gettimeofday(&tv, NULL);
    
    // calculate the offset
    toff = ((int) tv.tv_sec) - time;
    utoff = ((int) tv.tv_usec) - timeusec;
    while (utoff < 0) {
        toff--;
        utoff += 1000000;
    }
    
    // set it
    curOffset = ((double)toff) + (((double)utoff) / 1000000);
        
    if (m_LastUpdate == 0) {
        // set the main offset
        m_TimeOffset = curOffset;
        m_Elapsed = 0;
        return true;
    } else {
        // check it
        if (time < m_LastTime ||
            (time == m_LastTime && timeusec <= m_LastUTime)) {
            return false;
        } else {
            // set the current offset
            m_LastOffset = curOffset;
            // calculate the Hz
            toff = time - m_LastTime;
            utoff = timeusec - m_LastUTime;
            while (utoff < 0) {
                toff--;
                utoff += 1000000;
            }
            m_Elapsed = ((double)toff) + (((double)utoff)/1000000);
            
            m_LastTime = time;
            m_LastUTime = timeusec;
            
            return true;
        }
    }
}

/******************************************************************
* Name: SetPosition
* Description: Updates position data held for this player and resets
* the last update time.
******************************************************************/
void
MPPlayer::SetPosition
    (
        const double lat, const double lon, const double alt,
        const double heading, const double roll, const double pitch,
        const double speedN, const double speedE, const double speedD,
        const double left_aileron, const double right_aileron, const double elevator, const double rudder,
        //const double rpms[6],
        const double rateH, const double rateR, const double rateP,
		const double accN, const double accE, const double accD
    )
{
    int toff, utoff;
    
    // Save the position matrix and update time
    if (m_Initialised)
    {
        // calculate acceleration
        /*if (m_Elapsed > 0) {
            m_accN = (speedN - m_speedN) / m_Elapsed;
            m_accE = (speedE - m_speedE) / m_Elapsed;
            m_accD = (speedD - m_speedD) / m_Elapsed;
        } else {
            m_accN = 0;
            m_accE = 0;
            m_accD = 0;
        }*/
        
        // store the position
        m_lat = lat;
        m_lon = lon;
        m_alt = alt;
        m_hdg = heading;
        m_roll = roll;
        m_pitch = pitch;
        m_speedN = speedN;
        m_speedE = speedE;
        m_speedD = speedD;
		m_accN = accN;
		m_accE = accE;
		m_accD = accD;
        m_left_aileron = left_aileron;
		m_right_aileron = right_aileron;
        m_elevator = elevator;
		m_rudder = rudder;

        /*for (int i = 0; i < 6; i++) {
            m_rpms[i] = rpms[i];
        }
        m_rateH = rateH;
        m_rateR = rateR;
        m_rateP = rateP;*/
        
        if (!m_LocalPlayer) {
            m_AIModel->setLatitude(m_lat);
            m_AIModel->setLongitude(m_lon);
            m_AIModel->setAltitude(m_alt);
            m_AIModel->setHeading(m_hdg);
            m_AIModel->setBank(m_roll);
            m_AIModel->setPitch(m_pitch);
            m_AIModel->setSpeedN(m_speedN);
            m_AIModel->setSpeedE(m_speedE);
            m_AIModel->setSpeedD(m_speedD); 
            m_AIModel->setAccN(m_accN);
            m_AIModel->setAccE(m_accE);
            m_AIModel->setAccD(m_accD);
            m_AIModel->setRateH(m_rateH);
            m_AIModel->setRateR(m_rateR);
            m_AIModel->setRateP(m_rateP);
                
            // set properties
            SGPropertyNode *root = m_AIModel->getProps();
            root->getNode("surface-positions/left-aileron-pos-norm", true)->setDoubleValue(m_left_aileron);
			root->getNode("surface-positions/right-aileron-pos-norm", true)->setDoubleValue(m_right_aileron);
            root->getNode("surface-positions/elevator-pos-norm", true)->setDoubleValue(m_elevator);
            root->getNode("surface-positions/rudder-pos-norm", true)->setDoubleValue(m_rudder);
            /*root->getNode("engines/engine/rpm", true)->setDoubleValue(m_rpms[0]);
            root->getNode("engines/engine[1]/rpm", true)->setDoubleValue(m_rpms[1]);
            root->getNode("engines/engine[2]/rpm", true)->setDoubleValue(m_rpms[2]);
            root->getNode("engines/engine[3]/rpm", true)->setDoubleValue(m_rpms[3]);
            root->getNode("engines/engine[4]/rpm", true)->setDoubleValue(m_rpms[4]);
            root->getNode("engines/engine[5]/rpm", true)->setDoubleValue(m_rpms[5]);*/
                
            // Adjust by the last offset
            //cout << "OFFSET: " << (m_LastOffset - m_TimeOffset) << endl;
            
			//m_AIModel->timewarp(m_LastOffset - m_TimeOffset);

			// set the timestamp for the data update (sim elapsed time (secs))
			m_AIModel->setTimeStamp();
        }
        
        time(&m_LastUpdate);
        
        m_Updated = true;
    }
}

/******************************************************************
 * Name: SetProperty
 * Description: Sets a property of this player.
 ******************************************************************/
void MPPlayer::SetProperty(string property, SGPropertyNode::Type type, double val)
{    
    // get rid of any leading /
    while (property[0] == '/') property = property.substr(1);
    
    // get our root node
    SGPropertyNode *node = m_AIModel->getProps()->getNode(property.c_str(), true);
        
    // set the property
    switch (type) {
        case 2:
            node->setBoolValue((bool) val);
            break;
        case 3:
            node->setIntValue((int) val);
            break;
        case 4:
            node->setLongValue((long) val);
            break;
        case 5:
            node->setFloatValue((float) val);
            break;
        case 6:
        default:
            node->setDoubleValue(val);
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
                /*
                m_AIModel->setLatitude(m_lat);
                m_AIModel->setLongitude(m_lon);
                m_AIModel->setAltitude(m_alt);
                m_AIModel->setHeading(m_hdg);
                m_AIModel->setBank(m_roll);
                m_AIModel->setPitch(m_pitch);
                m_AIModel->setSpeedN(m_speedN);
                m_AIModel->setSpeedE(m_speedE);
                m_AIModel->_setVS_fps(m_speedU*60.0); // it needs input in fpm
                m_AIModel->setRateH(m_rateH);
                m_AIModel->setRateR(m_rateR);
                m_AIModel->setRateP(m_rateP);
                
                // set properties
                SGPropertyNode *root = m_AIModel->getProps();
                root->getNode("controls/flight/aileron", true)->setDoubleValue(m_aileron);
                root->getNode("controls/flight/elevator", true)->setDoubleValue(m_elevator);
                root->getNode("controls/flight/rudder", true)->setDoubleValue(m_rudder);
                root->getNode("engines/engine/rpm", true)->setDoubleValue(m_rpms[0]);
                root->getNode("engines/engine[1]/rpm", true)->setDoubleValue(m_rpms[1]);
                root->getNode("engines/engine[2]/rpm", true)->setDoubleValue(m_rpms[2]);
                root->getNode("engines/engine[3]/rpm", true)->setDoubleValue(m_rpms[3]);
                root->getNode("engines/engine[4]/rpm", true)->setDoubleValue(m_rpms[4]);
                root->getNode("engines/engine[5]/rpm", true)->setDoubleValue(m_rpms[5]);
                
                // Adjust by the last offset
                m_AIModel->update(m_LastOffset - m_TimeOffset);
                */
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
 * Name: LoadAI
 * Description: Loads the AI model into the AI core.
******************************************************************/
void
MPPlayer::LoadAI(void)
{
    // set up the model info
    FGAIModelEntity aiModel;
    aiModel.m_type = "aircraft";
    aiModel.path = m_ModelName;
    aiModel.acType = "Multiplayer";
    aiModel.company = m_Callsign;
   
    // then get the model manager
    FGAIManager *aiModelMgr = (FGAIManager *) globals->get_subsystem("ai_model");
    if (!aiModelMgr) {
        SG_LOG( SG_NETWORK, SG_ALERT, "MPPlayer::LoadAI - "
                << "Cannot find AI model manager!" );
        return;
    }
    
    // then get the model
    fgSetBool("/sim/freeze/clock", true);
    m_AIModel = (FGAIMultiplayer *) aiModelMgr->createMultiplayer(&aiModel);
    fgSetBool("/sim/freeze/clock", false);
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
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    FillMsgHdr(MsgHdr, POS_DATA_ID);
    strncpy(PosMsg->Model, m_ModelName.c_str(), MAX_MODEL_NAME_LEN);
    PosMsg->Model[MAX_MODEL_NAME_LEN - 1] = '\0';
    PosMsg->time = XDR_encode_uint32 (tv.tv_sec);
    PosMsg->timeusec = XDR_encode_uint32 (tv.tv_usec);
    PosMsg->lat = XDR_encode_double (m_lat);
    PosMsg->lon = XDR_encode_double (m_lon);
    PosMsg->alt = XDR_encode_double (m_alt);
    PosMsg->hdg = XDR_encode_double (m_hdg);
    PosMsg->roll = XDR_encode_double (m_roll);
    PosMsg->pitch = XDR_encode_double (m_pitch);
    PosMsg->speedN = XDR_encode_double (m_speedN);
    PosMsg->speedE = XDR_encode_double (m_speedE);
    PosMsg->speedD = XDR_encode_double (m_speedD);
    PosMsg->left_aileron = XDR_encode_float ((float) m_left_aileron);
    PosMsg->right_aileron = XDR_encode_float ((float) m_right_aileron);
    PosMsg->elevator = XDR_encode_float ((float) m_elevator);
    PosMsg->rudder = XDR_encode_float ((float) m_rudder);
    /*for (int i = 0; i < 6; i++) {
        PosMsg->rpms[i] = XDR_encode_float ((float) m_rpms[i]);
    }*/
    PosMsg->rateH =  XDR_encode_float ((float) m_rateH);
    PosMsg->rateR =  XDR_encode_float ((float) m_rateR);
    PosMsg->rateP =  XDR_encode_float ((float) m_rateP);
	PosMsg->accN  =  XDR_encode_float ((float) m_accN);
    PosMsg->accE =  XDR_encode_float ((float) m_accE);
    PosMsg->accD =  XDR_encode_float ((float) m_accD);
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
        case PROP_MSG_ID:
            len = sizeof(T_MsgHdr) + sizeof(T_PropertyMsg);
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

