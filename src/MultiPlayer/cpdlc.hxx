//////////////////////////////////////////////////////////////////////
//
// cpdlc.hxx
//
// started November 2020
// Authors: Henning Stahlke, Michael Filhol
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$
//
//////////////////////////////////////////////////////////////////////

#ifndef CPDLC_H
#define CPDLC_H

#include <deque>
#include <string>

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/io/sg_socket.hxx> 
#include "mpirc.hxx"

enum CPDLCStatus {
    CPDLC_OFFLINE,
    CPDLC_CONNECTING,
    CPDLC_ONLINE,

// private (not exposed to property tree) below this line:
    CPDLC_WAIT_IRC_READY, 
};

//
// CPDLCManager implements a ControllerPilotDataLinkConnection via an IRC connection
//
class CPDLCManager
{
public:
    CPDLCManager(IRCConnection* irc);
    ~CPDLCManager();

    bool connect(const std::string authority);
    void disconnect();
    bool send(const std::string message);
    void getMessage();
    void update();

private:
    IRCConnection *_irc;
    std::string _data_authority {""};
    std::deque<struct IRCMessage> _incoming_messages;
    CPDLCStatus _status {CPDLC_OFFLINE};

    SGPropertyNode *_pStatus {nullptr};
    SGPropertyNode *_pDataAuthority {nullptr};
    SGPropertyNode *_pMessage {nullptr};
    SGPropertyNode *_pNewMessage {nullptr};

    void processMessage(struct IRCMessage entry);
};

#endif