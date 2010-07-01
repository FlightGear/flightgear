// jsclient.cxx -- simple UDP networked joystick client
//
// Copyright (C) 2003 by Manuel Bessler and Stephen Lowry
//
// based on joyclient.cxx by Curtis Olson
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/misc/stdint.hxx>

#include <Main/fg_props.hxx>

#include "jsclient.hxx"


FGJsClient::FGJsClient() {
	active = fgHasNode("/jsclient"); // if exist, assume bindings are defined
	SG_LOG( SG_IO, SG_INFO, "/jsclient exists, activating JsClient remote joystick support");

	for( int i = 0; i < 4; ++i )
	{
    	    axisdef[i] = fgGetNode("/jsclient/axis", i);
	    if( axisdef[i] != NULL )
	    {
        	    axisdefstr[i] = axisdef[i]->getStringValue();
		    SG_LOG( SG_IO, SG_INFO, "jsclient axis[" << i << "] mapped to property " << axisdefstr[i]);
	    }
	    else
		axisdefstr[i] = "";
	}
}

FGJsClient::~FGJsClient() {
}


// open hailing frequencies
bool FGJsClient::open() {
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

    return true;
}


// process work for this port
bool FGJsClient::process() {
    SGIOChannel *io = get_io_channel();
    int length = 4+4+4+4+4+4;

//    if( ! active )
//	    return true;
    if ( get_direction() == SG_IO_OUT ) {
	SG_LOG( SG_IO, SG_ALERT, "JsClient protocol is read only" );
	return false;
    } else if ( get_direction() == SG_IO_IN ) {
	SG_LOG( SG_IO, SG_DEBUG, "Searching for data." );
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		int32_t *msg = (int32_t *)buf;
		for( int i = 0; i < 4; ++i )
		{
			axis[i] = ((double)msg[i] / 2147483647.0);
			if ( fabs(axis[i]) < 0.05 )
			    axis[i] = 0.0;
			if( axisdefstr[i].length() != 0 )
    			    fgSetFloat(axisdefstr[i].c_str(), axis[i]);
		}
	    }
	} else {
	    while ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		int32_t *msg = (int32_t *)buf;
		SG_LOG( SG_IO, SG_DEBUG, "ax0 = " << msg[0] << " ax1 = "
			<< msg[1] << "ax2 = " << msg[2] << "ax3 = " << msg[3]);
		for( int i = 0; i < 4; ++i )
		{
			axis[i] = ((double)msg[i] / 2147483647.0);
			if ( fabs(axis[i]) < 0.05 )
			    axis[i] = 0.0;
			if( axisdefstr[i].length() != 0 )
    			    fgSetFloat(axisdefstr[i].c_str(), axis[i]);
		}
	    }
	}
    }

    return true;
}


// close the channel
bool FGJsClient::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
