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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <cstdlib>		// atoi() atof()

#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/props/props.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>

#include "jpg-httpd.hxx"

#define __MAX_HTTP_BLOCK_SIZE       4096
#define __MAX_STRING_SIZE           2048
#define __TIMEOUT_COUNT             5
#define __HTTP_GET_STRING           "GET "

#include <osgUtil/SceneView>
#include <osgViewer/Viewer>
extern osg::ref_ptr<osgUtil::SceneView> sceneView;

using std::string;


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
    simgear::NetChannel::poll();

    return true;
}


bool FGJpegHttpd::close() {
    delete imageServer;

    return true;
}

// Handle http GET requests
void HttpdImageChannel :: foundTerminator( void ) {

    closeWhenDone();

    char      szTemp[256];
    char      szResponse[__MAX_STRING_SIZE];
    char      *pRequest     = buffer.getData();
    int       nStep         = 0;
    int       nBytesSent    = 0;
    int       nTimeoutCount = 0;
    int       nBufferCount  = 0;
    int       nImageLen;
    int       nBlockSize;


    if ( strstr( pRequest, __HTTP_GET_STRING ) != NULL ) {
        
        SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< HTTP Request : " << pRequest );

        double left, right, bottom, top, zNear, zFar;
	osgViewer::Viewer* viewer = globals->get_renderer()->getViewer();
        viewer->getCamera()->getProjectionMatrixAsFrustum(left, right,
                                                          bottom, top,
                                                          zNear, zFar);
        JpgFactory->setFrustum( left, right, bottom, top, zNear, zFar );

        nImageLen  = JpgFactory -> render();
	nBlockSize = ( nImageLen < __MAX_HTTP_BLOCK_SIZE ? nImageLen : __MAX_HTTP_BLOCK_SIZE );

        if( nImageLen ) {
	    strcpy( szResponse, "HTTP/1.1 200 OK" );
	    strcat( szResponse, getTerminator() );
	    strcat( szResponse, "Content-Type: image/jpeg" );
            strcat( szResponse, getTerminator() );

            SG_LOG( SG_IO, SG_DEBUG, "info->numbytes = " << nImageLen );
            sprintf( szTemp, "Content-Length: %d", nImageLen );
	    strcat( szResponse, szTemp );

            strcat( szResponse, getTerminator() );
	    strcat( szResponse, "Connection: close" );
	    strcat( szResponse, getTerminator() );
	    strcat( szResponse, getTerminator() );

	    if( getHandle() == -1 )  {
	        SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< Invalid socket handle. Ignoring request.\n" );
		buffer.remove();
		SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< End of image Transmission.\n" );
		return;
	    }

	    if( send( ( char * ) szResponse, strlen( szResponse ) ) <= 0 )  {
	        SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< Error to send HTTP response. Ignoring request.\n" );
		buffer.remove();
		SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< End of image Transmission.\n" );
		return;
	    }

	    /*
	     * Send block with size defined by __MAX_HTTP_BLOCK_SIZE
	     */
	    while( nStep <= nImageLen ) {
                nBufferCount++;

		if( getHandle() == -1 )  {
		    SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< Invalid socket handle. Ignoring request.\n" );
		    break;
		}

		nBytesSent = send( ( char * ) JpgFactory -> data() + nStep, nBlockSize );

		if( nBytesSent <= 0 )  {
  		    if( nTimeoutCount == __TIMEOUT_COUNT )  {
		        SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< Timeout reached. Exiting before end of image transmission.\n" );
			nTimeoutCount = 0;
			break;
		    }

		SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< Zero bytes sent.\n" );

#ifdef _WIN32
                Sleep(1000);
#else
	        sleep(1);
#endif
		nTimeoutCount++;
		continue;
	    }

	    SG_LOG( SG_IO, SG_DEBUG, ">>>>>>>>> (" << nBufferCount << ") BLOCK STEP " << nStep << " - IMAGELEN " << nImageLen << " - BLOCKSIZE " << nBlockSize << " - SENT " << nBytesSent );

 	    /*
	     * Calculate remaining image.
	     */
	    if( ( nStep + nBlockSize ) >= nImageLen ) {
	        nBlockSize = ( nImageLen - nStep );
		nStep += nBlockSize;
	    }

  	    nStep += nBytesSent;
	    nTimeoutCount = 0;
#ifdef _WIN32
            Sleep(1);
#else
	    usleep( 1000 );
#endif
	}

	SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< End of image Transmission.\n" );

        } else {
            SG_LOG( SG_IO, SG_DEBUG, "!!! NO IMAGE !!!  info -> numbytes = " << nImageLen );
        }

	/*
	 * Release JPEG buffer.
	 */
	JpgFactory -> destroy();
    }

    buffer.remove();
}
