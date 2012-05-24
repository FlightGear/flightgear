// \file props.cxx
// Property server class.
//
// Written by Curtis Olson, started September 2000.
// Modified by Bernie Bright, May 2002.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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


#include <simgear/io/sg_netChat.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/misc/strutils.hxx>

#include <sstream>

#include "UGear_command.hxx"
#include "UGear_telnet.hxx"

using std::stringstream;
using std::ends;

/**
 * Props connection class.
 * This class represents a connection to props client.
 */
class PropsChannel : public simgear::NetChat
{
    simgear::NetBuffer buffer;

    /**
     * Current property node name.
     */
    string path;

    enum Mode {
	PROMPT,
	DATA
    };
    Mode mode;

public:

    /**
     * Constructor.
     */
    PropsChannel();
    
    /**
     * Append incoming data to our request buffer.
     *
     * @param s Character string to append to buffer
     * @param n Number of characters to append.
     */
    void collectIncomingData( const char* s, int n );

    /**
     * Process a complete request from the props client.
     */
    void foundTerminator();

private:
    /**
     * Return a "Node no found" error message to the client.
     */
    void node_not_found_error( const string& node_name );
};

/**
 * 
 */
PropsChannel::PropsChannel()
    : buffer(512),
      path("/"),
      mode(PROMPT)
{
    setTerminator( "\r\n" );
}

/**
 * 
 */
void
PropsChannel::collectIncomingData( const char* s, int n )
{
    buffer.append( s, n );
}

/**
 * 
 */
void
PropsChannel::node_not_found_error( const string& node_name )
{
    string error = "-ERR Node \"";
    error += node_name;
    error += "\" not found.";
    push( error.c_str() );
    push( getTerminator() );
}

/**
 * We have a command.
 * 
 */
void
PropsChannel::foundTerminator()
{
    const char* cmd = buffer.getData();
    SG_LOG( SG_IO, SG_INFO, "processing command = \"" << cmd << "\"" );

    vector<string> tokens = simgear::strutils::split( cmd );

    if (!tokens.empty()) {
	string command = tokens[0];

        if ( command == "send" ) {
            command_mgr.add( tokens[1] );
        } else if ( command == "quit" ) {
	    close();
	    shouldDelete();
	    return;
	} else if ( command == "data" ) {
	    mode = DATA;
	} else if ( command == "prompt" ) {
	    mode = PROMPT;
	} else {
	    const char* msg = "\
Valid commands are:\r\n\
\r\n\
data               switch to raw data mode\r\n\
prompt             switch to interactive mode (default)\r\n\
quit               terminate connection\r\n\
send <command>     send <command> to UAS\r\n";
	    push( msg );
	}
    }

    if (mode == PROMPT) {
	string prompt = "> ";
	push( prompt.c_str() );
    }

    buffer.remove();
}

/**
 * 
 */
UGTelnet::UGTelnet( const int port_num ):
    enabled(false)
{
    port = port_num;
}

/**
 * 
 */
UGTelnet::~UGTelnet()
{
}

/**
 * 
 */
bool
UGTelnet::open()
{
    if (enabled ) {
	printf("This shouldn't happen, but the telnet channel is already in use, ignoring\n" );
	return false;
    }

    simgear::NetChannel::open();
    simgear::NetChannel::bind( "", port );
    simgear::NetChannel::listen( 5 );
    printf("Telnet server started on port %d\n", port );

    enabled = true;

    return true;
}

/**
 * 
 */
bool
UGTelnet::close()
{
    SG_LOG( SG_IO, SG_INFO, "closing UGTelnet" );   
    return true;
}

/**
 * 
 */
bool
UGTelnet::process()
{
    simgear::NetChannel::poll();
    return true;
}

/**
 * 
 */
void
UGTelnet::handleAccept()
{
    simgear::IPAddress addr;
    int handle = simgear::NetChannel::accept( &addr );
    printf("Telent server accepted connection from %s:%d\n",
           addr.getHost(), addr.getPort() );
    PropsChannel* channel = new PropsChannel();
    channel->setHandle( handle );
}
