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

#ifndef MULTIPLAYMGR_H
#define MULTIPLAYMGR_H

#define MULTIPLAYTXMGR_HID "$Id$"

#include "mpplayer.hxx"
#include "mpmessages.hxx"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include STL_STRING
SG_USING_STD(string);
#include <vector>
SG_USING_STD(vector);

#include <simgear/compiler.h>
#include <plib/netSocket.h>
#include <Main/globals.hxx>

// Maximum number of players that can exist at any time
// FIXME: use a list<mplayer> instead
#define MAX_PLAYERS 10

class FGMultiplayMgr 
{
public:
    FGMultiplayMgr();
    ~FGMultiplayMgr();
    bool init(void);
    void Close(void);
    // transmitter
    void SendMyPosition (const sgQuat PlayerOrientation, 
                         const sgdVec3 PlayerPosition);
    void SendTextMessage (const string &sMsgText) const;
    // receiver
    void ProcessData(void);
    void ProcessPosMsg ( const char *Msg, netAddress & SenderAddress );
    void ProcessChatMsg ( const char *Msg, netAddress & SenderAddress );
    void Update(void);
private:
    typedef vector<MPPlayer*>           t_MPClientList;
    typedef t_MPClientList::iterator    t_MPClientListIterator;
    MPPlayer*       m_LocalPlayer;
    netSocket*      m_DataSocket;
    netAddress      m_Server;
    bool            m_HaveServer;
    bool            m_Initialised;
    t_MPClientList  m_MPClientList;
    string          m_RxAddress;
    int             m_RxPort;
    string          m_Callsign;
};

#endif

