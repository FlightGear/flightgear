// \file telnet.cx
// Property telnet server class.
//
// Written by Bernie Bright, started May 2002.
//
// Copyright (C) 2002  Bernie Bright - bbright@bigpond.net.au
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/props.hxx>
#include <simgear/misc/props_io.hxx>

#include STL_STRSTREAM

#include <Main/globals.hxx>
#include <Main/viewmgr.hxx>

#include <plib/netChat.h>

#include "telnet.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(strstream);
SG_USING_STD(ends);
#endif

/**
 * Telnet connection class.
 * This class represents a connection to a telnet-style client.
 */
class TelnetChannel : public netChat
{
    netBuffer buffer;

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
    TelnetChannel();
    
    /**
     * Append incoming data to our request buffer.
     *
     * @param s Character string to append to buffer
     * @param n Number of characters to append.
     */
    void collectIncomingData( const char* s, int n );

    /**
     * Process a complete request from the telnet client.
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
TelnetChannel::TelnetChannel()
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
TelnetChannel::collectIncomingData( const char* s, int n )
{
    buffer.append( s, n );
}

/**
 * 
 */
void
TelnetChannel::node_not_found_error( const string& node_name )
{
    string error = "ERR Node \"";
    error += node_name;
    error += "\" not found.";
    push( error.c_str() );
    push( getTerminator() );
}

// return a human readable form of the value "type"
static string
getValueTypeString( const SGPropertyNode *node )
{
    string result;

    if ( node == NULL )
    {
	return "unknown";
    }

    SGPropertyNode::Type type = node->getType();
    if ( type == SGPropertyNode::UNSPECIFIED ) {
	result = "unspecified";
    } else if ( type == SGPropertyNode::NONE ) {
        result = "none";
    } else if ( type == SGPropertyNode::BOOL ) {
	result = "bool";
    } else if ( type == SGPropertyNode::INT ) {
	result = "int";
    } else if ( type == SGPropertyNode::LONG ) {
	result = "long";
    } else if ( type == SGPropertyNode::FLOAT ) {
	result = "float";
    } else if ( type == SGPropertyNode::DOUBLE ) {
	result = "double";
    } else if ( type == SGPropertyNode::STRING ) {
	result = "string";
    }

    return result;
}

/**
 * We have a command.
 * 
 */
void
TelnetChannel::foundTerminator()
{
    const char* cmd = buffer.getData();
    SG_LOG( SG_IO, SG_INFO, "processing command = \"" << cmd << "\"" );

    vector<string> tokens = simgear::strutils::split( cmd );

    SGPropertyNode* node = globals->get_props()->getNode( path.c_str() );

    if (!tokens.empty())
    {
	string command = tokens[0];

	if (command == "ls")
	{
	    SGPropertyNode* dir = node;
	    if (tokens.size() == 2)
	    {
		if (tokens[1][0] == '/')
		{
		    dir = globals->get_props()->getNode( tokens[1].c_str() );
		}
		else
		{
		    string s = path;
		    s += "/";
		    s += tokens[1];
		    dir = globals->get_props()->getNode( s.c_str() );
		}

		if (dir == 0)
		{
		    node_not_found_error( tokens[1] );
		    goto prompt;
		}
	    }

	    for (int i = 0; i < dir->nChildren(); i++)
	    {
		SGPropertyNode * child = dir->getChild(i);
		string line = child->getDisplayName(true);

		if ( child->nChildren() > 0 )
		{
		    line += "/";
		}
		else
		{
		    if (mode == PROMPT)
		    {
			string value = child->getStringValue();
			line += " =\t'" + value + "'\t(";
			line += getValueTypeString( child );
			line += ")";
		    }
		}

		line += getTerminator();
		push( line.c_str() );
	    }
	}
	else if ( command == "dump" )
	{
	    strstream buf;
	    if ( tokens.size() <= 1 )
	    {
		writeProperties( buf, node );
		buf << ends; // null terminate the string
		push( buf.str() );
		push( getTerminator() );
	    }
	    else
	    {
		SGPropertyNode *child = node->getNode( tokens[1].c_str() );
		if ( child )
		{
		    writeProperties ( buf, child );
		    buf << ends; // null terminate the string
		    push( buf.str() );
		    push( getTerminator() );
		}
		else
		{
		    node_not_found_error( tokens[1] );
		}
	    }
	}
	else if ( command == "cd" )
	{
	    if (tokens.size() == 2)
	    {
		try
		{
		    SGPropertyNode* child = node->getNode( tokens[1].c_str() );
		    if ( child )
		    {
			node = child;
			path = node->getPath();
		    }
		    else
		    {
			node_not_found_error( tokens[1] );
		    }
		}
		catch (...)
		{
		    // Ignore attempt to move past root node with ".."
		}
	    }
	}
	else if ( command == "pwd" )
	{
	    string ttt = node->getPath();
	    if (ttt.empty())
	    {
		ttt = "/";
	    }

	    push( ttt.c_str() );
	    push( getTerminator() );
	}
	else if ( command == "get" || command == "show" )
	{
	    if ( tokens.size() == 2 )
	    {
		string tmp;	
		string value = node->getStringValue ( tokens[1].c_str(), "" );
		if ( mode == PROMPT )
		{
		    tmp = tokens[1];
		    tmp += " = '";
		    tmp += value;
		    tmp += "' (";
		    tmp += getValueTypeString(
				     node->getNode( tokens[1].c_str() ) );
		    tmp += ")";
		}
		else
		{
		    tmp = value;
		}
		push( tmp.c_str() );
		push( getTerminator() );
	    }
	}
	else if ( command == "set" )
	{
	    if ( tokens.size() == 3 )
	    {
		node->getNode( tokens[1].c_str(), true )->setStringValue(tokens[2].c_str());

		if ( mode == PROMPT )
		{
		    // now fetch and write out the new value as confirmation
		    // of the change
		    string value = node->getStringValue ( tokens[1].c_str(), "" );
		    string tmp = tokens[1] + " = '" + value + "' (";
		    tmp += getValueTypeString( node->getNode( tokens[1].c_str() ) );
		    tmp += ")";
		    push( tmp.c_str() );
		    push( getTerminator() );
		}
	    }
	}
	else if (command == "quit")
	{
	    close();
	    shouldDelete();
	    return;
	}
	else if ( command == "data" )
	{
	    mode = DATA;
	}
	else if ( command == "prompt" )
	{
	    mode = PROMPT;
	}
	else
	{
	    const char* msg = "\
Valid commands are:\r\n\
\r\n\
cd <dir>           cd to a directory, '..' to move back\r\n\
data               switch to raw data mode\r\n\
dump               dump current state (in xml)\r\n\
get <var>          show the value of a parameter\r\n\
help               show this help message\r\n\
ls [<dir>]         list directory\r\n\
prompt             switch to interactive mode (default)\r\n\
pwd                display your current path\r\n\
quit               terminate connection\r\n\
set <var> <val>    set <var> to a new <val>\r\n\
show <var>         synonym for get\r\n";
	    push( msg );
	}
    }

 prompt:
    if (mode == PROMPT)
    {
	string prompt = node->getPath();
	if (prompt.empty())
	{
	    prompt = "/";
	}
	prompt += "> ";
	push( prompt.c_str() );
    }

    buffer.remove();
}

/**
 * 
 */
FGTelnet::FGTelnet( const vector<string>& tokens )
{
    // tokens:
    //   telnet,port#
    //   props,medium,direction,hz,hostname,port#,style
    if (tokens.size() == 2)
	port = atoi( tokens[1].c_str() );
    else if (tokens.size() == 7)
	port = atoi( tokens[5].c_str() );
    else
	throw FGProtocolConfigError( "FGTelnet: incorrect number of configuration arguments" );
}

/**
 * 
 */
FGTelnet::~FGTelnet()
{
}

/**
 * 
 */
bool
FGTelnet::open()
{
    if ( is_enabled() )
    {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    netChannel::open();
    netChannel::bind( "", port );
    netChannel::listen( 5 );
    SG_LOG( SG_IO, SG_INFO, "Telnet server started on port " << port );

    set_hz( 5 );                // default to processing requests @ 5Hz
    set_enabled( true );
    return true;
}

/**
 * 
 */
bool
FGTelnet::close()
{
    return true;
}

/**
 * 
 */
bool
FGTelnet::process()
{
    netChannel::poll();
    return true;
}

/**
 * 
 */
void
FGTelnet::handleAccept()
{
    netAddress addr;
    int handle = accept( &addr );
    SG_LOG( SG_IO, SG_INFO, "Telnet server accepted connection from "
	    << addr.getHost() << ":" << addr.getPort() );
    TelnetChannel* channel = new TelnetChannel();
    channel->setHandle( handle );
}
