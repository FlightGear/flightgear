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



#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/props.hxx>
#include <simgear/misc/props_io.hxx>

#include <Main/globals.hxx>

#include <stdlib.h>		// atoi() atof()

#include STL_STRSTREAM

#include "props.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
SG_USING_STD(istrstream);
SG_USING_STD(strstream);
#endif

FGProps::FGProps() {
}

FGProps::~FGProps() {
}


// open hailing frequencies
bool FGProps::open() {
    reset();

    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    SGIOChannel *io = get_io_channel();

    if ( ! io->open( get_direction() ) ) {
	SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
	return false;
    }

    set_enabled( true );
    SG_LOG( SG_IO, SG_INFO, "Opening properties channel communication layer." );

    return true;
}


// prepare for new connection
bool FGProps::reset() {
    path = "/";
    mode = PROMPT;
    return true;
}


// return a human readable form of the value "type"
static string getValueTypeString( const SGPropertyNode *node ) {
    string result;

    if ( node == NULL ) {
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

    SGPropertyNode * node = globals->get_props()->getNode(path.c_str());

    if ( command == "ls" ) {
	SGPropertyNode * dir = node;
	if ( tokens.size() > 2 ) {
	    if ( tokens[1][0] == '/' ) {
		dir = globals->get_props()->getNode(tokens[1].c_str());
	    } else {
		dir = globals->get_props()->getNode((path + "/" + tokens[1]).c_str());
	    }
	    if ( dir == 0 ) {
		tokens[1] = "ERR Node \"" + tokens[1] + "\" not found.\n";
		io->writestring( tokens[1].c_str() );
		return true;
	    }
	}
	
	for (int i = 0; i < (int)dir->nChildren(); i++) {
	    SGPropertyNode * child = dir->getChild(i);
	    string name = child->getName();
	    string line = name;
	    if ( dir->getChild(name.c_str(), 1) ) {
		char buf[16];
		sprintf(buf, "[%d]", child->getIndex());
		line += buf;
	    }
	    if ( child->nChildren() > 0 ) {
		line += "/";
	    } else {
		if ( mode == PROMPT ) {
		    string value = dir->getStringValue ( name.c_str(), "" );
		    line += " =\t'" + value + "'\t(";
		    line += getValueTypeString( dir->getNode( name.c_str() ) );
		    line += ")";
		}
	    }
	    line += "\n";
	    io->writestring( line.c_str() );
	}
    } else if ( command == "dump" ) {
	strstream buf;
	if ( tokens.size() <= 1 ) {
	    writeProperties ( buf, node);
	    io->writestring( buf.str() );
	}
	else {
	    SGPropertyNode *child = node->getNode(tokens[1].c_str());
	    if ( child ) {
		writeProperties ( buf, child );
		io->writestring( buf.str() );
	    } else {
		tokens[1] = "ERR Node \"" + tokens[1] + "\" not found.\n";
		io->writestring( tokens[1].c_str() );
	    }
	}
    } else if ( command == "cd" ) {
	// string tmp = "current path = " + node.getPath() + "\n";
	// io->writestring( tmp.c_str() );

        if ( tokens.size() <= 1 ) {
	    // do nothing
	} else {
	    SGPropertyNode *child = node->getNode(tokens[1].c_str());
	    if ( child ) {
		node = child;
		path = node->getPath();
	    } else {
		tokens[1] = "ERR Node \"" + tokens[1] + "\" not found.\n";
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
	    string tmp;	
	    string value = node->getStringValue ( tokens[1].c_str(), "" );
	    if ( mode == PROMPT ) {
		//string ttt = "debug = '" + tokens[1] + "'\n";
		//io->writestring( ttt.c_str() );

		tmp = tokens[1] + " = '" + value + "' (";
		tmp += getValueTypeString( node->getNode( tokens[1].c_str() ) );
		tmp += ")\n";
 	    } else {
		tmp = value + "\n";
	    }
	    io->writestring( tmp.c_str() );
	}
    } else if ( command == "set" ) {
        if ( tokens.size() <= 2 ) {
	    // do nothing
	} else {
	    node->getNode( tokens[1].c_str(), true )->setStringValue(tokens[2].c_str());

	    // now fetch and write out the new value as confirmation
	    // of the change
	    string value = node->getStringValue ( tokens[1].c_str(), "" );
	    string tmp = tokens[1] + " = '" + value + "' (";
	    tmp += getValueTypeString( node->getNode( tokens[1].c_str() ) );
	    tmp += ")\n";
 
	    io->writestring( tmp.c_str() );
	}
    } else if ( command == "quit" ) {
	close();
	reset();
	return true;
    } else if ( command == "data" ) {
    	mode = DATA;
    } else if ( command == "prompt" ) {
	mode = PROMPT;
    } else {
	io->writestring( "\n" );
	io->writestring( "Valid commands are:\n" );
	io->writestring( "\n" );
	io->writestring( "help             show help message\n" );
	io->writestring( "ls [<dir>]       list directory\n" );
	io->writestring( "dump             dump current state (in xml)\n" );
	io->writestring( "cd <dir>         cd to a directory, '..' to move back\n" );
	io->writestring( "pwd              display your current path\n" );
	io->writestring( "get <var>        show the value of a parameter\n" );
	io->writestring( "show <var>       synonym for get\n" );
	io->writestring( "set <var> <val>  set <var> to a new <val>\n" );
	io->writestring( "data             switch to raw data mode\n" );
	io->writestring( "prompt           switch to interactive mode (default)\n" );
	io->writestring( "quit             terminate connection\n" );
	io->writestring( "\n" );
    }

    if ( mode == PROMPT ) {
	string prompt = node->getPath();
	if ( prompt == "" ) {
	    prompt = "/";
	}
	prompt += "> ";
	io->writestring( prompt.c_str() );
    }
    return true;
}


// process work for this port
bool FGProps::process() {
    SGIOChannel *io = get_io_channel();
    char buf[max_cmd_len];

    // cout << "processing incoming props command" << endl;

    if ( get_direction() == SG_IO_BI ) {
	// cout << "  (bi directional)" << endl;
	while ( io->readline( buf, max_cmd_len ) > 0 ) {
	    SG_LOG( SG_IO, SG_ALERT, "Success reading data." );
	    process_command( buf );
	}
    } else {
	SG_LOG( SG_IO, SG_ALERT, 
		"in or out direction not supported for FGProps." );
    }

    return true;
}


// close the channel
bool FGProps::close() {
    SGIOChannel *io = get_io_channel();

    if ( ! io->close() ) {
	return false;
    }

    cout << "successfully closed channel\n";

    return true;
}
