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

#include <strstream.h>

#include "props.hxx"


FGProps::FGProps() {
}

FGProps::~FGProps() {
}


// open hailing frequencies
bool FGProps::open() {
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

    return true;
}


bool FGProps::process_command( const char *cmd ) {
    cout << "processing command = " << cmd << endl;
    string_list tokens;
    tokens.clear();

    istrstream in(cmd);
    
    while ( !in.eof() ) {
	string token;
	in >> token;
	tokens.push_back( token );
    }

    string command = tokens[0];

    if ( command == "ls" ) {
	
    }

    return true;
}


// process work for this port
bool FGProps::process() {
    SGIOChannel *io = get_io_channel();

    cout << "processing incoming props command" << endl;

    if ( get_direction() == SG_IO_BI ) {
	cout << "  (bi directional)" << endl;
	while ( io->read( buf, max_cmd_len ) > 0 ) {
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

    return true;
}
