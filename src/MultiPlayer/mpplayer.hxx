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
#include <simgear/props/props.hxx>
#include <time.h>
#include <sys/time.h>

#include STL_STRING
SG_USING_STD(string);

// Number of seconds before a player is consider to be lost
#define TIME_TO_LIVE 10

class FGAIMultiplayer;

class MPPlayer 
{
public:
    MPPlayer();
    ~MPPlayer();
    /** Enumeration of the states for the player's data */
    enum PlayerDataState
    {
        PLAYER_DATA_NOT_AVAILABLE = 0, 
        PLAYER_DATA_AVAILABLE, 
        PLAYER_DATA_EXPIRED
    };
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
    bool Open(const string &IP, const int &Port, const string &Callsign,
              const string &ModelName, const bool LocalPlayer);
    /** Closes the player connection */
    void Close(void);
    /** Checks if the time is valid for a position update and perhaps sets the time offset
     */
    bool CheckTime(int time, int timeusec);
    /** Sets the positioning matrix held for this player
    */
    void SetPosition(const double lat, const double lon, const double alt,
                     const double heading, const double roll, const double pitch,
                     const double speedN, const double speedE, const double speedD,
                     const double left_aileron, const double right_aileron, const double elevator, const double rudder,
                     //const double rpms[6],
                     const double rateH, const double rateR, const double rateP,
                     const double accN, const double accE, const double accD);
    /** Sets a property for this player
    */
    void SetProperty(string property, SGPropertyNode::Type type, double val);
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
    bool CompareCallsign(const char *Callsign) const;
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
    void    LoadAI(void);       // Loads the plane into the AI core
    bool    m_Initialised;      // True if object is initialised 
    
    double  m_lat;              // location, orientation, etc...
    double  m_lon;              // ...
    double  m_alt;              // ...
    double  m_hdg;              // ...
    double  m_roll;             // ...
    double  m_pitch;            // ...
    double  m_speedN;           // ...
    double  m_speedE;           // ...
    double  m_speedD;           // ...
    double  m_accN;             // ...
    double  m_accE;             // ...
    double  m_accD;             // ...
    double  m_left_aileron;     // ...
    double  m_right_aileron;     // ...
    double  m_elevator;         // ...
    double  m_rudder;           // ...
    //double  m_rpms[6];          // ...
    double  m_rateH;            // ...
    double  m_rateR;            // ...
    double  m_rateP;            // ...

    time_t  m_LastUpdate;       // last time update data received
    int     m_LastTime;         // last seconds according to the packet
    int     m_LastUTime;        // last microseconds according to the packet
    double  m_Elapsed;          // Elapsed other-side time between responses
    double  m_TimeOffset;       // the offset to aim for
    double  m_LastOffset;       // the last offset we got
    bool    m_Updated;          // Set when the player data is updated
    string  m_Callsign;         // players callsign
    bool    m_LocalPlayer;      // true if player is the local player
    string  m_ModelName;        // Aircraft model name for player
    netAddress m_PlayerAddress; // Address information for the player
    FGAIMultiplayer *m_AIModel; // The AI model of this aircraft
};

#endif



