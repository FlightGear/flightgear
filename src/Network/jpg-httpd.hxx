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


#ifndef _FG_JPEG_HTTPD_HXX
#define _FG_JPEG_HTTPD_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/netChat.h>

#ifdef FG_JPEG_SERVER
#  include <simgear/screen/jpgfactory.hxx>
#else
// dummy it in to keep the compiler happy
class trJpgFactory {
public:
    trJpgFactory();
    void init(int, int);
    void destroy();
    int render();
  void *data();
};
#endif

#include "protocol.hxx"

class trJpgFactory;


/* simple httpd server that makes an hasty stab at following the http
   1.1 rfc.  */

class HttpdImageChannel : public netChat
{

    netBuffer buffer ;
    trJpgFactory *JpgFactory;
    
public:

    HttpdImageChannel() : buffer(512) {
        setTerminator("\r\n");
        JpgFactory = new trJpgFactory();

        // This is a terrible hack but it can't be initialized until
        // after OpenGL is up an running
        JpgFactory->init(400,300);
    }

    ~HttpdImageChannel() {
        JpgFactory->destroy();
        delete JpgFactory;
    }

    virtual void collectIncomingData (const char* s, int n) {
        buffer.append(s,n);
    }

    // Handle the actual http request
    virtual void foundTerminator (void);
};


class HttpdImageServer : private netChannel
{
    virtual bool writable (void) { return false ; }

    virtual void handleAccept (void) {
        netAddress addr ;
        int handle = accept ( &addr ) ;
        printf("Client %s:%d connected\n", addr.getHost(), addr.getPort());

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
