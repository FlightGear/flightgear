//////////////////////////////////////////////////////////////////////
//
// mpirc.cxx
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

#include "mpirc.hxx"
#include <Main/fg_props.hxx>

const std::string IRC_TEST_CHANNEL{"#mptest"}; // for development
const std::string IRC_MSG_TERMINATOR {"\r\n"};
// https://www.alien.net.au/irc/irc2numerics.html
const std::string IRC_RPL_WELCOME {"001"};
const std::string IRC_RPL_YOURID {"042"};
const std::string IRC_RPL_MOTD {"372"};
const std::string IRC_RPL_MOTDSTART {"375"};
const std::string IRC_RPL_ENDOFMOTD {"376"};
const std::string IRC_ERR_NOSUCHNICK {"401"};

IRCConnection::IRCConnection(const std::string &nickname, const std::string &servername, const std::string &port) : SGSocket(servername, port, "tcp"),
                                                                                                                 _nickname(nickname)
{
}

IRCConnection::~IRCConnection()
{
}

// setup properties to reflect the status of this IRC connection in the prop tree
void IRCConnection::setupProperties(std::string path)
{
    if (path.back() != '/') path.push_back('/');
    if (!_pReadyFlag) _pReadyFlag = fgGetNode(path + "irc-ready", true);
    _pReadyFlag->setBoolValue(_logged_in);

    if (!_pMessageCountIn) _pMessageCountIn = fgGetNode(path + "msg-count-in", true);
    if (!_pMessageCountOut) _pMessageCountOut = fgGetNode(path + "msg-count-out", true);
    if (!_pIRCReturnCode) _pIRCReturnCode = fgGetNode(path + "last-return-code", true);    
}


bool IRCConnection::login(const std::string &nickname)
{
    if (!_connected && !connect()) {
        return false;
    }
    if (!nickname.empty()) {
        _nickname = nickname;
    } else {
        SG_LOG(SG_NETWORK, SG_WARN, "IRC login requires nickname argument.");
        return false;
    } 

    std::string lines("NICK ");
    lines += _nickname;
    lines += IRC_MSG_TERMINATOR;
    lines += "USER ";
    lines += _nickname; //IRC <user>
    lines += " 0 * :";  //IRC <mode> <unused>
    lines += _nickname; //IRC <realname>
    lines += IRC_MSG_TERMINATOR;
    return writestring(lines.c_str());
}

// login with nickname given to constructor
bool IRCConnection::login()
{
    return login(_nickname);
}

// the polite way to leave
void IRCConnection::quit()
{
    if (!_connected) return;
    writestring("QUIT goodbye\r\n");
    disconnect();
}

bool IRCConnection::sendPrivmsg(const std::string &recipient, const std::string &textline)
{
    if (!_logged_in) {
        SG_LOG(SG_NETWORK, SG_WARN, "IRC 'privmsg' command unvailable. Login first!");
        return false;
    }
    std::string line("PRIVMSG ");
    line += recipient;
    line += " :";
    line += textline;
    line += IRC_MSG_TERMINATOR;
    if (writestring(line.c_str())) {
        if (_pMessageCountOut) _pMessageCountOut->setIntValue(_pMessageCountOut->getIntValue() + 1);
        if (_pIRCReturnCode) _pIRCReturnCode->setStringValue("");
        return true;
    } else {
        SG_LOG(SG_NETWORK, SG_WARN, "IRC send privmsg failed.");
        return false;
    }
}

// join an IRC channel
bool IRCConnection::join(const std::string &channel)
{
    if (!_logged_in) {
        SG_LOG(SG_NETWORK, SG_WARN, "IRC 'join' command unvailable. Login first!");
        return false;
    }
    std::string lines("JOIN ");
    lines += channel;
    lines += IRC_MSG_TERMINATOR;
    return writestring(lines.c_str());
}

// leave an IRC channel
bool IRCConnection::part(const std::string &channel)
{
    if (!_logged_in) {
        SG_LOG(SG_NETWORK, SG_WARN, "IRC 'part' command unvailable. Login first!");
        return false;
    }
    std::string lines("PART ");
    lines += channel;
    lines += IRC_MSG_TERMINATOR;
    return writestring(lines.c_str());
}

/*
    Call update() regularly to maintain connection (ping/pong) and process messages.
    For information only:
    The ping timeout appears to depend on the server settings and can be in the order 
    of minutes. However, for smooth message processing the update frequency should be 
    at least a few times per second and calling this at frame rate should not hurt.
*/
void IRCConnection::update()
{
    if (_connected && readline(_read_buffer, sizeof(_read_buffer) - 1) > 0) {
        std::string line(_read_buffer); // TODO: buffer size check required?
        parseReceivedLine(line);
    }
}

///////////////////////////////////////////////////////////////////////////////
// private methods
///////////////////////////////////////////////////////////////////////////////

// open a connection to IRC server
bool IRCConnection::connect()
{
    if (_connected) {
        return true;
    }

    _connected = open(SG_IO_OUT);
    if (_connected) {
        nonblock();
    } else {
        disconnect();
        SG_LOG(SG_NETWORK, SG_WARN, "IRCConnection::connect error");
    }
    return _connected;
}

void IRCConnection::disconnect()
{
    _logged_in = false;
    if (_pReadyFlag) _pReadyFlag->setBoolValue(_logged_in);

    if (_connected) {
        _connected = false;
        close();
        SG_LOG(SG_NETWORK, SG_INFO, "IRCConnection::disconnect");
    }
}

void IRCConnection::pong(const std::string &recipient)
{
    if (!_connected) return;
    std::string line("PONG ");
    line += recipient;
    line += IRC_MSG_TERMINATOR;
    writestring(line.c_str());
}

bool IRCConnection::parseReceivedLine(std::string line)
{
    /*
    https://tools.ietf.org/html/rfc2812#section-3.7.2
    2.3.1 Message format in Augmented BNF

    The protocol messages must be extracted from the contiguous stream of
    octets.  The current solution is to designate two characters, CR and
    LF, as message separators.  Empty messages are silently ignored,
    which permits use of the sequence CR-LF between messages without
    extra problems.

    The extracted message is parsed into the components <prefix>,
    <command> and list of parameters (<params>).

        The Augmented BNF representation for this is:

        message    =  [ ":" prefix SPACE ] command [ params ] crlf
        prefix     =  servername / ( nickname [ [ "!" user ] "@" host ] )
        command    =  1*letter / 3digit
        params     =  *14( SPACE middle ) [ SPACE ":" trailing ]
                =/ 14( SPACE middle ) [ SPACE [ ":" ] trailing ]

        nospcrlfcl =  %x01-09 / %x0B-0C / %x0E-1F / %x21-39 / %x3B-FF
                        ; any octet except NUL, CR, LF, " " and ":"
        middle     =  nospcrlfcl *( ":" / nospcrlfcl )
        trailing   =  *( ":" / " " / nospcrlfcl )

        SPACE      =  %x20        ; space character
        crlf       =  %x0D %x0A   ; "carriage return" "linefeed"

    */

    // removes trailing '\r\n'
    // TODO: for length(IRC_MSG_TERMINATOR) do  line.pop_back();
    line.pop_back();
    line.pop_back();

    std::string prefix;
    std::string command;
    std::string params;

    std::size_t pos = line.find(" ", 1);
    //prefix
    if (line.at(0) == ':') {
        prefix = line.substr(1, pos - 1); // remove leading ":"
        std::size_t end = line.find(" ", pos + 1);
        command = line.substr(pos + 1, end - pos - 1);
        pos = end;
    } else {
        command = line.substr(0, pos);
    }
    params = line.substr(pos + 1);

    // uncomment next line for debug output
    //cout << "[prefix]" << prefix << "[cmd]" << command << "[params]" << params << "[end]" << endl;

    // receiving a message
    if (command == "PRIVMSG") {
        if (_pMessageCountIn) _pMessageCountIn->setIntValue(_pMessageCountIn->getIntValue() + 1);
        std::string recipient = params.substr(0, params.find(" :"));
        // direct private message
        if (recipient == _nickname) {
            struct IRCMessage rcv;
            rcv.sender = prefix.substr(0, prefix.find("!"));
            rcv.textline = params.substr(params.find(":") + 1);
            _incoming_private_messages.push_back(rcv);
        } else {
            // Most likely from an IRC channel if we joined any. In this case
            // recipient equals channel name (e.g. "#mptest"). IRC channel
            // support could be implemented here in future.
            SG_LOG(SG_NETWORK, SG_DEV_WARN, "Ignoring PRIVMSG to '" + recipient + "' (should be '" + _nickname + "')");
        }
    } else if (command == "PING") {
        // server pings us
        std::string server = params.substr(0, params.find(" "));
        pong(server);
    } else if (command == "JOIN") {
        // server acks our join request
        std::string channel = params.substr(0, params.find(" "));
        SG_LOG(SG_NETWORK, SG_DEV_WARN, "Joined IRC channel " + channel); //DEBUG
    } else if (command == IRC_RPL_WELCOME) {
        // after welcome we are logged in and allowed to send commands/messages to the IRC
        _logged_in = true;
        if (_pReadyFlag) _pReadyFlag->setBoolValue(1);

        //joining channel might help while development, maybe removed later
        //join(IRC_TEST_CHANNEL);
    } 
    else if (command == IRC_RPL_MOTD) {
    } 
    else if (command == IRC_RPL_MOTDSTART) {
    } 
    else if (command == IRC_RPL_ENDOFMOTD) {
    }
    else if (command == IRC_ERR_NOSUCHNICK) {
        // server return code if we send to invalid nickname
        if (_pIRCReturnCode) _pIRCReturnCode->setStringValue(IRC_ERR_NOSUCHNICK);
    } 
    else if (command == "ERROR") {
        if (_pIRCReturnCode) _pIRCReturnCode->setStringValue(params);
        disconnect();
    }
    // unexpected IRC message
    else {
        //SG_LOG(SG_NETWORK, SG_MANDATORY_INFO, "Unhandled IRC message "); //DEBUG
        //cout << "[prefix]" << prefix << "[cmd]" << command << "[params]" << params << "[end]" << endl;

        // TODO: anything sensitive here that we should handle?
        // e.g. IRC user has disconnected and username == {current-cpdlc-authority}
        return false;
    }
    return true;
}

IRCMessage IRCConnection::getMessage()
{
    struct IRCMessage entry {
        "", ""
    };
    if (!_incoming_private_messages.empty()) {
        entry = _incoming_private_messages.front();
        _incoming_private_messages.pop_front();
    }
    return entry;
}
