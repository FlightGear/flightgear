// httpd.hxx -- FGFS http property manager interface / external script
//              and control class
//
// Written by Curtis Olson, started June 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
//
// Jpeg Image Support added August 2001
//  by Norman Vine - nhv@cape.com
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

#include <stdlib.h>		// atoi() atof()

#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/props/props.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "jpg-httpd.hxx"

SG_USING_STD(string);


bool FGJpegHttpd::open() {
    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    imageServer = new HttpdImageServer( port );
    
    set_hz( 5 );                // default to processing requests @ 5Hz
    set_enabled( true );

    return true;
}


bool FGJpegHttpd::process() {
    netChannel::poll();

    return true;
}


bool FGJpegHttpd::close() {
    delete imageServer;

    return true;
}


// Handle http GET requests
void HttpdImageChannel::foundTerminator (void) {

    closeWhenDone ();

    string response;

    const string s = buffer.getData();
    if ( s.find( "GET " ) == 0 ) {
        
        printf("echo: %s\n", s.c_str());

        int ImageLen = JpgFactory->render();

        if( ImageLen ) {
            response = "HTTP/1.1 200 OK";
            response += getTerminator();
            response += "Content-Type: image/jpeg";
            response += getTerminator();
            push( response.c_str() );

            char ctmp[256];
            printf( "info->numbytes = %d\n", ImageLen );
            sprintf( ctmp, "Content-Length: %d", ImageLen );
            push( ctmp );

            response = getTerminator();
            response += "Connection: close";
            response += getTerminator();
            response += getTerminator();
            push( response.c_str() );

            /* can't use strlen on binary data */
            bufferSend ( (char *)JpgFactory->data(), ImageLen ) ;
        } else {
            printf("!!! NO IMAGE !!!\n\tinfo->numbytes = %d\n", ImageLen );
        }
    }

    buffer.remove();
}
