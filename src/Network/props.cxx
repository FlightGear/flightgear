// props.hxx -- FGFS property manager interaction class
//
// Written by Curtis Olson, started September 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#include <Main/globals.hxx>

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/props.hxx>

#include <stdlib.h>		// atoi() atof()

#include <strstream>

#include "props.hxx"

FG_USING_STD(cout);
FG_USING_STD(istrstream);

FGProps::FGProps() {
}

FGProps::~FGProps() {
}


// open hailing frequencies
bool FGProps::open() {
    path = "/";

    if ( is_enabled() ) {
	FG_LOG( FG_IO, FG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    SGIOChannel *io = get_io_channel();

    if ( ! io->open( get_direction() ) ) {
	FG_LOG( FG_IO, FG_ALERT, "Error opening channel communication layer." );
	return false;
    }

    set_enabled( true );
    FG_LOG( FG_IO, FG_INFO, "Opening properties channel communication layer." );

    return true;
}


// return a human readable form of the value "type"
static string getValueTypeString( const SGValue *v ) {
    string result;

    if ( v == NULL ) {
	return "unknown";
    }

    SGValue::Type type = v->getType();
    if ( type == SGValue::UNKNOWN ) {
	result = "unknown";
    } else if ( type == SGValue::BOOL ) {
	result = "bool";
    } else if ( type == SGValue::INT ) {
	result = "int";
    } else if ( type == SGValue::FLOAT ) {
	result = "float";
    } else if ( type == SGValue::DOUBLE ) {
	result = "double";
    } else if ( type == SGValue::STRING ) {
	result = "string";
    }

    return result;
}


bool FGProps::process_command( const char *cmd ) {
    SGIOChannel *io = get_io_channel();

    cout << "processing command = " << cmd;
    string_list tokens;
    tokens.clear();

    istrstream in(cmd);
    
    while ( !in.eof() ) {
	string token;
	in >> token;
	tokens.push_back( token );
    }

    string command = tokens[0];

    SGPropertyNode * node = globals->get_props()->getNode(path);

    if ( command == "ls" ) {
	for (int i = 0; i < (int)node->nChildren(); i++) {
	    SGPropertyNode * child = node->getChild(i);
	    string name = child->getName();
	    string line = name;
	    if ( child->nChildren() > 0 ) {
		line += "/\n";
	    } else {
		string value = node->getStringValue ( name, "" );
		line += " =\t'" + value + "'\t(";
		line += getValueTypeString( node->getValue( name ) );
		line += ")\n";
	    }
	    io->writestring( line.c_str() );
	}
    } else if ( command == "dump" ) {
	strstream buf;
	if ( tokens.size() <= 1 ) {
	    writeProperties ( buf, node);
	    io->writestring( buf.str() );
	}
	else {
	    SGPropertyNode *child = node->getNode(tokens[1]);
	    if ( child ) {
		writeProperties ( buf, child );
		io->writestring( buf.str() );
	    } else {
		tokens[1] += " Not Found\n";
		io->writestring( tokens[1].c_str() );
	    }
	}
    } else if ( command == "cd" ) {
	// string tmp = "current path = " + node.getPath() + "\n";
	// io->writestring( tmp.c_str() );

        if ( tokens.size() <= 1 ) {
	    // do nothing
	} else {
	    SGPropertyNode *child = node->getNode(tokens[1]);
	    if ( child ) {
		node = child;
		path = node->getPath();
	    } else {
		tokens[1] += " Not Found\n";
		io->writestring( tokens[1].c_str() );
	    }
	}
    } else if ( command == "pwd" ) {
	string ttt = node->getPath();
	if ( ttt == "" ) {
	    ttt = "/";
	}
	ttt += "\n";
	io->writestring( ttt.c_str() );
    } else if ( command == "get" || command == "show" ) {
	if ( tokens.size() <= 1 ) {
	    // do nothing
	} else {
	    string ttt = "debug = '" + tokens[1] + "'\n";
	    io->writestring( ttt.c_str() );

	    string value = node->getStringValue ( tokens[1], "" );
	    string tmp = tokens[1] + " = '" + value + "' (";
	    tmp += getValueTypeString( node->getValue( tokens[1] ) );
	    tmp += ")\n";
 
	    io->writestring( tmp.c_str() );
	}
    } else if ( command == "set" ) {
        if ( tokens.size() <= 2 ) {
	    // do nothing
	} else {
	    node->getValue( tokens[1], true )->setStringValue(tokens[2]);

	    // now fetch and write out the new value as confirmation
	    // of the change
	    string value = node->getStringValue ( tokens[1], "" );
	    string tmp = tokens[1] + " = '" + value + "' (";
	    tmp += getValueTypeString( node->getValue( tokens[1] ) );
	    tmp += ")\n";
 
	    io->writestring( tmp.c_str() );
	}
    } else if ( command == "quit" ) {
	close();
    } else {
	io->writestring( "\n" );
	io->writestring( "Valid commands are:\n" );
	io->writestring( "\n" );
	io->writestring( "help             show help message\n" );
	io->writestring( "ls               list current directory\n" );
	io->writestring( "dump             dump current state (in xml)\n" );
	io->writestring( "cd <dir>         cd to a directory, '..' to move back\n" );
	io->writestring( "pwd              display your current path\n" );
	io->writestring( "get <var>        show the value of a parameter\n" );
	io->writestring( "show <var>       synonym for get\n" );
	io->writestring( "set <var> <val>  set <var> to a new <val>\n" );
	io->writestring( "quit             terminate connection\n" );
	io->writestring( "\n" );
    }

    string prompt = node->getPath();
    if ( prompt == "" ) {
	prompt = "/";
    }
    prompt += "> ";
    io->writestring( prompt.c_str() );

    return true;
}


// process work for this port
bool FGProps::process() {
    SGIOChannel *io = get_io_channel();

    // cout << "processing incoming props command" << endl;

    if ( get_direction() == SG_IO_BI ) {
	// cout << "  (bi directional)" << endl;
	while ( io->readline( buf, max_cmd_len ) > 0 ) {
	    FG_LOG( FG_IO, FG_ALERT, "Success reading data." );
	    process_command( buf );
	}
    } else {
	FG_LOG( FG_IO, FG_ALERT, 
		"in or out direction not supported for FGProps." );
    }

    return true;
}


// close the channel
bool FGProps::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    cout << "successfully closed channel\n";

    return true;
}
