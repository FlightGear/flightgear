//////////////////////////////////////////////////////////////////////
//
// cpdlc.cxx
//
// started November 2020
// Authors: Michael Filhol, Henning Stahlke
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

#include "cpdlc.hxx"
#include <Main/fg_props.hxx>

const std::string CPDLC_IRC_SERVER {"mpirc.flightgear.org"};
const std::string CPDLC_MSGPREFIX_CONNECT {"___CPDLC_CONNECT___"};
const std::string CPDLC_MSGPREFIX_MSG {"___CPDLC_MSG___"};
const std::string CPDLC_MSGPREFIX_DISCONNECT {"___CPDLC_DISCONNECT___"};

///////////////////////////////////////////////////////////////////////////////
// CPDLCManager
///////////////////////////////////////////////////////////////////////////////

CPDLCManager::CPDLCManager(IRCConnection* irc) : 
    _irc(irc)
{
    // CPDLC link status props
    _pStatus = fgGetNode("/network/cpdlc/link/status", true);
    _pStatus->setIntValue(_status);
    _pDataAuthority = fgGetNode("/network/cpdlc/link/data-authority", true);
    _pDataAuthority->setStringValue(_data_authority);
    // CPDLC message output
    _pMessage = fgGetNode("/network/cpdlc/rx/message", true);
    _pMessage->setStringValue("");
    _pNewMessage = fgGetNode("/network/cpdlc/rx/new-message", true);
    _pNewMessage->setBoolValue(0);
}

CPDLCManager::~CPDLCManager()
{
}

/* 
connect will be called by
1) user via a fgcommand defined in multiplaymgr 
2) CPDLC::update() while waiting for IRC to become ready

authority parameter is accepted only when coming from disconnected state, 
disconnect must be called first if user wants to change authority (however, ATC 
handover is not blocked by this)

IRC connection will be initiated if necessary, it takes a while to connect to IRC 
which is why connect is called from update() when in state CPDLC_WAIT_IRC_READY
*/
bool CPDLCManager::connect(const std::string authority = "")
{
    // ensure we get an authority on first call but do not accept a change before 
    // resetting _data_authority in disconnect()
    if (_data_authority.empty()) {
        if (authority.empty()) {
            SG_LOG(SG_NETWORK, SG_WARN, "cpdlcConnect not possible: empty argument!");
            return false;
        } else {
            _data_authority = authority;
        }
    }
    if (!authority.empty() && authority != _data_authority) {
        SG_LOG(SG_NETWORK, SG_WARN, "cpdlcConnect: cannot change authority now, use disconnect first!");
        return false;
    }

    // launch IRC connection as needed
    if (!_irc->isConnected()) {
        SG_LOG(SG_NETWORK, SG_INFO, "Connecting to IRC server...");
        if (!_irc->login()) {
            SG_LOG(SG_NETWORK, SG_WARN, "IRC login failed.");
            return false;
        }
        _status = CPDLC_WAIT_IRC_READY;
        return true;        
    } else if (_irc->isReady() && _status != CPDLC_ONLINE) {
        SG_LOG(SG_NETWORK, SG_INFO, "CPDLC sending 'connect'");
        _status = CPDLC_CONNECTING;
        _pStatus->setIntValue(_status);
        return _irc->sendPrivmsg(_data_authority, CPDLC_MSGPREFIX_CONNECT);
    }
    return false;
}

void CPDLCManager::disconnect()
{
    if (_irc && _irc->isConnected() && !_data_authority.empty()) {
        _irc->sendPrivmsg(_data_authority, CPDLC_MSGPREFIX_DISCONNECT);
    }
    _data_authority = "";
    _pDataAuthority->setStringValue(_data_authority);
    _status = CPDLC_OFFLINE;
    _pStatus->setIntValue(_status);
}

bool CPDLCManager::send(const std::string message)
{
    std::string textline(CPDLC_MSGPREFIX_MSG); // TODO surely a way to format std string in line
    textline += ' ';
    textline += message;
    return _irc->sendPrivmsg(_data_authority, textline);
}

// move next message from input queue to property tree
void CPDLCManager::getMessage()
{
    if (!_incoming_messages.empty()) {
        struct IRCMessage entry = _incoming_messages.front();
        _incoming_messages.pop_front();
        _pMessage->setStringValue(entry.textline.substr(CPDLC_MSGPREFIX_MSG.length() + 1));
        if (_incoming_messages.empty()) {
            _pNewMessage->setBoolValue(0);
        }
    }
}

/*
 update() call this regularly to complete connect and check for new messages
 update frequency requirement should be low (e.g. once per second) but frame rate 
 will not hurt as the code is slim
*/
void CPDLCManager::update()
{
    if (_irc) {
        if (_irc->isConnected()) {
            if (_status == CPDLC_WAIT_IRC_READY && _irc->isReady()) {
                SG_LOG(SG_NETWORK, SG_INFO, "CPDLC IRC ready, connecting...");
                connect();
            }
            if (_irc->hasMessage()) {
                processMessage(_irc->getMessage());
            }
        }
        // IRC disconnected (unexpectedly) 
        else if (_status != CPDLC_OFFLINE) {
            _status = CPDLC_OFFLINE;
            _pStatus->setIntValue(_status);
        }
    }
}

// process incoming message
void CPDLCManager::processMessage(struct IRCMessage message)
{
    // connection accepted by ATC, or new data authority (been transferred)
    if (message.textline.find(CPDLC_MSGPREFIX_CONNECT) == 0) {
        SG_LOG(SG_NETWORK, SG_INFO, "CPDLC got connected.");
        _data_authority = message.sender;
        _pDataAuthority->setStringValue(_data_authority); // make this known to ACFT
        _status = CPDLC_ONLINE;
        _pStatus->setIntValue(_status);
    }
    // do not process message if sender does not match our current data authority
    if (message.sender != _data_authority) {
        SG_LOG(SG_NETWORK, SG_WARN, "Received CPDLC message from foreign authority.");
        return;
    }
    // connection rejected, or terminated by ATC
    if (message.textline.find(CPDLC_MSGPREFIX_DISCONNECT) == 0) {
        if (message.sender == _data_authority) {
            SG_LOG(SG_NETWORK, SG_INFO, "CPDLC got disconnect.");
            disconnect();
        }
    }
    // store valid message in queue for later retrieval by aircraft
    else if (message.textline.find(CPDLC_MSGPREFIX_MSG) == 0) {
        SG_LOG(SG_NETWORK, SG_INFO, "CPDLC message");
        _incoming_messages.push_back(message);
        _pNewMessage->setBoolValue(1);
    }
}
