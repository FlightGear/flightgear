//////////////////////////////////////////////////////////////////////
//
// mpirc.hxx
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

#ifndef MPIRC_H
#define MPIRC_H

#include <deque>
#include <string>

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/io/sg_socket.hxx> 

const std::string IRC_DEFAULT_PORT {"6667"};
const int IRC_BUFFER_SIZE = 1024;

struct IRCMessage {
    std::string sender;
    std::string textline;
};

/*
    IRCConnection implements a basic IRC client for transmitting and receiving
    private messages via an IRC server

    In general it is possible to have multiple instances of this class but you 
    have to consider the following points:
    - you cannot connect to a server with the same nickname more than once at a time
    - if you want to expose status info to the property tree, you have to pass 
      an unique prefix to setupProperties() per instance
*/
class IRCConnection : SGSocket
{
public:
    IRCConnection(const std::string &nickname, const std::string &servername, const std::string &port = IRC_DEFAULT_PORT);
    ~IRCConnection();
    
    void setupProperties(std::string path);
    void update();

    bool login(const std::string &nickname);
    bool login();
    void quit();

    bool sendPrivmsg(const std::string &recipient, const std::string &textline);
    bool join(const std::string &channel);
    bool part(const std::string &channel);
    

    bool isConnected() const { return _connected; }
    bool isReady() const { return _logged_in; }
    bool hasMessage() const { return !_incoming_private_messages.empty(); }
    IRCMessage getMessage();

private:
    bool connect();
    void disconnect();
    void pong(const std::string &recipient);
    bool parseReceivedLine(std::string irc_line);

    bool _connected {false}; // TCP session ok
    bool _logged_in {false}; // IRC login completed
    std::string _nickname {""};
    char _read_buffer[IRC_BUFFER_SIZE];
    std::deque<IRCMessage> _incoming_private_messages;

    SGPropertyNode *_pReadyFlag {nullptr};
    SGPropertyNode *_pMessageCountIn {nullptr};
    SGPropertyNode *_pMessageCountOut {nullptr};
    SGPropertyNode *_pIRCReturnCode {nullptr};
};

#endif