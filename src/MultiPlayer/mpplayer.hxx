// mpplayer.hxx -- routines for a player within a multiplayer Flightgear
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


#ifndef MPPLAYER_H
#define MPPLAYER_H

#define MPPLAYER_HID "$Id$"

/****************************************************************
* @version $Id$
*
* Description: This class holds information about a player in a
* multiplayer game. The model for a remote player is loaded and
* added onto the aircraft branch of the scene on calling Open.
* The model is unloaded from the scene when Close is called
* or the object is deleted. The model's positioning transform is
* updated by calling SetPosition. The updated position transform
* is applied to the model on the scene by calling Draw.
*
******************************************************************/

#include "mpmessages.hxx"

#include <plib/sg.h>
#include <plib/netSocket.h>
#include <simgear/io/sg_socket_udp.hxx>
#include <time.h>

#include STL_STRING
SG_USING_STD(string);

// Number of seconds before a player is consider to be lost
#define TIME_TO_LIVE 10


class ssgEntity;
class ssgTransform;


class MPPlayer {
public:

    /** Constructor */
    MPPlayer();

    /** Destructor. */
    ~MPPlayer();

    /** Enumeration of the states for the player's data */
    enum PlayerDataState {PLAYER_DATA_NOT_AVAILABLE = 0, PLAYER_DATA_AVAILABLE, PLAYER_DATA_EXPIRED};

    /** Player data state */
    typedef enum PlayerDataState TPlayerDataState;

    /** Initialises the class.
    * @param sIP IP address or host name for sending data to the player
    * @param sPort Port number for sending data to the player
    * @param sCallsign Callsign of the player (must be unique across all instances of MPPlayer).
    * @param sModelName Path and name of the aircraft model file for the player
    * @param bLocalPlayer True if this player is the local player, else false
    * @return True if class opens successfully, else false
    */
    bool Open(const string &sIP, const int &iPort, const string &sCallsign,
              const string &sModelName, const bool bLocalPlayer);

    /** Closes the player connection */
    void Close(void);

    /** Sets the positioning matrix held for this player
    * @param PlayerPosMat4 Matrix for positioning player's aircraft
    */
    void SetPosition(const sgMat4 PlayerPosMat4);

    /** Transform and place model for player
    */
    TPlayerDataState Draw(void);

    /** Returns the callsign for the player
    * @return Aircraft's callsign
    */
    string Callsign(void) const;

    /** Compares the player's callsign with the given callsign
    * @param sCallsign Callsign to compare
    * @return True if callsign matches
    */
    bool CompareCallsign(const char *sCallsign) const;

    /** Populates a position message for the player
    * @param MsgHdr Header to be populated
    * @param PosMsg Position message to be populated
    */
    void FillPosMsg(T_MsgHdr *MsgHdr, T_PositionMsg *PosMsg);

    /** Populates a mesage header with information for the player
    * @param MsgHdr Header to be populated
    * @param iMsgId Message type identifier to insert into header
    */
    void FillMsgHdr(T_MsgHdr *MsgHdr, const int iMsgId);


private:

    /** Loads the model of the aircraft */
    void LoadModel(void);

    /** True if object is initialised */
    bool m_bInitialised;

    /** Position matrix for the player's aircraft */
    sgMat4 m_ModelPos;

    /** Used to remove player if no activity */
    time_t m_LastUpdate;

    /** Set when the player data is updated and cleared when read */
    bool m_bUpdated;

    /** Player's callsign */
    string m_sCallsign;

    /** Aircraft model name for player */
    string m_sModelName;

    /** The player's loaded model */
    ssgEntity *m_Model;

    /** Model transform */
    ssgTransform *m_ModelTrans;

    /** True if this player is the local player */
    bool m_bLocalPlayer;

    /** Address information for the player */
    netAddress m_PlayerAddress;

};

#endif



