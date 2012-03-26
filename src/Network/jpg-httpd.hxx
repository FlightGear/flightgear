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


#ifndef _FG_JPEG_HTTPD_HXX
#define _FG_JPEG_HTTPD_HXX

#include <simgear/io/sg_netChat.hxx>
#include <simgear/screen/jpgfactory.hxx>

#include "protocol.hxx"


/* simple httpd server that makes an hasty stab at following the http
   1.1 rfc.  */

class HttpdImageChannel : public simgear::NetChat
{

    simgear::NetBuffer buffer ;
    trJpgFactory *JpgFactory;
    
public:

    HttpdImageChannel() : buffer(512) {

        int nWidth  = fgGetInt( "/sim/startup/xsize", 800 );
        int nHeight = fgGetInt( "/sim/startup/ysize", 600 );

        setTerminator("\r\n");
        JpgFactory = new trJpgFactory();
        JpgFactory -> init( nWidth, nHeight );
    }

    ~HttpdImageChannel() {
        JpgFactory -> destroy();
        delete JpgFactory;
    }

    virtual void collectIncomingData (const char* s, int n) {
        buffer.append(s,n);
    }

    // Handle the actual http request
    virtual void foundTerminator (void);
};


class HttpdImageServer : private simgear::NetChannel
{
    virtual bool writable (void) { return false ; }

    virtual void handleAccept (void) {
        simgear::IPAddress addr ;
        int handle = accept ( &addr ) ;
        SG_LOG( SG_IO, SG_INFO, "Client " << addr.getHost() << ":" << addr.getPort() << " connected" );

        HttpdImageChannel *hc = new HttpdImageChannel;
        hc->setHandle ( handle ) ;
    }

public:

    HttpdImageServer ( int port ) {
        open ();
        bind( "", port );
        listen( 5 );

        printf( "HttpdImage server started on port %d\n", port ) ;
    }
        
};


class FGJpegHttpd : public FGProtocol {

    int port;
    HttpdImageServer *imageServer;
    
public:

    inline FGJpegHttpd( int p ) { port = p; }

    inline ~FGJpegHttpd() { }

    bool open();

    bool process();

    bool close();
};


#endif // _FG_JPEG_HTTPD_HXX
