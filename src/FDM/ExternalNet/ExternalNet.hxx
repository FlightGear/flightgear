// ExternalNet.hxx -- an net interface to an external flight dynamics model
//
// Written by Curtis Olson, started November 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#ifndef _EXTERNAL_NET_HXX
#define _EXTERNAL_NET_HXX

#include <plib/netBuffer.h>
#include <plib/netSocket.h>

#include <simgear/timing/timestamp.hxx> // fine grained timing measurements

#include <Network/net_ctrls.hxx>
#include <Network/net_fdm.hxx>


class HTTPClient : public netBufferChannel
{

    bool done;
    SGTimeStamp start;

public:

    HTTPClient ( const char* host, int port, const char* path ) :
        done( false )
    {
	open ();
	connect (host, port);

	const char* s = netFormat ( "GET %s HTTP/1.0\r\n\r\n", path );
	bufferSend( s, strlen(s) ) ;

        start.stamp();
    }

    virtual void handleBufferRead (netBuffer& buffer)
    {
	const char* s = buffer.getData();
	while (*s)
	    fputc(*s++,stdout);

	printf("done\n");
	buffer.remove();
	printf("after buffer.remove()\n");
        done = true;
    }

    bool isDone() const { return done; }
    bool isDone( long usec ) const { 
        SGTimeStamp now;
        now.stamp();
        if ( (now - start) > usec ) {
            return true;
        } else {
            return done;
        }
    }
};


class FGExternalNet: public FGInterface {

private:

    int data_in_port;
    int data_out_port;
    int cmd_port;
    string fdm_host;

    netSocket data_client;
    netSocket data_server;

    bool valid;

    FGNetCtrls ctrls;
    FGNetFDM fdm;

public:

    // Constructor
    FGExternalNet( double dt, string host, int dop, int dip, int cp );

    // Destructor
    ~FGExternalNet();

    // Reset flight params to a specific position
    void init();

    // update the fdm
    void update( double dt );

};


#endif // _EXTERNAL_NET_HXX
