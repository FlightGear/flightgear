// fg_socket.cxx -- Socket I/O routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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

#if ! defined( _MSC_VER )
#  include <sys/time.h>		// select()
#  include <sys/types.h>	// socket(), bind(), select(), accept()
#  include <sys/socket.h>	// socket(), bind(), listen(), accept()
#  include <netinet/in.h>	// struct sockaddr_in
#  include <netdb.h>		// gethostbyname()
#  include <unistd.h>		// select(), fsync()/fdatasync()
#else
#  include <sys/timeb.h>	// select()
#  include <winsock2.h>		// socket(), bind(), listen(), accept(),
				// struct sockaddr_in, gethostbyname()
#  include <windows.h>
#  include <io.h>
#endif

#if defined( sgi )
#include <strings.h>
#endif

#include STL_STRING

#include <simgear/debug/logstream.hxx>

#include "fg_socket.hxx"

FG_USING_STD(string);


FGSocket::FGSocket() :
    save_len(0)
{
}


FGSocket::~FGSocket() {
}


int FGSocket::make_server_socket () {
    struct sockaddr_in name;

#if defined( __CYGWIN__ ) || defined( __CYGWIN32__ ) || defined( sgi ) || defined( _MSC_VER )
    int length;
#else
    socklen_t length;
#endif
     
    // Create the socket.
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	FG_LOG( FG_IO, FG_ALERT, 
		"Error: socket() failed in make_server_socket()" );
	return -1;
    }
     
    // Give the socket a name.
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(port); // set port to zero to let system pick
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
	FG_LOG( FG_IO, FG_ALERT,
		"Error: bind() failed in make_server_socket()" );
	return -1;
    }
     
    // Find the assigned port number
    length = sizeof(struct sockaddr_in);
    if ( getsockname(sock, (struct sockaddr *) &name, &length) ) {
	FG_LOG( FG_IO, FG_ALERT,
		"Error: getsockname() failed in make_server_socket()" );
	return -1;
    }
    port = ntohs(name.sin_port);

    return sock;
}


int FGSocket::make_client_socket () {
    struct sockaddr_in name;
    struct hostent *hp;
     
    // Create the socket.
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	FG_LOG( FG_IO, FG_ALERT, 
		"Error: socket() failed in make_client_socket()" );
	return -1;
    }
     
    // specify address family
    name.sin_family = AF_INET;

    // get the hosts official name/info
    hp = gethostbyname( hostname.c_str() );

    // Connect this socket to the host and the port specified on the
    // command line
#if defined( __CYGWIN__ ) || defined( __CYGWIN32__ )
    bcopy(hp->h_addr, (char *)(&(name.sin_addr.s_addr)), hp->h_length);
#else
    bcopy(hp->h_addr, &(name.sin_addr.s_addr), hp->h_length);
#endif
    name.sin_port = htons(port);

    if ( connect(sock, (struct sockaddr *) &name, 
		 sizeof(struct sockaddr_in)) < 0 )
    {
	std::close(sock);
	FG_LOG( FG_IO, FG_ALERT, 
		"Error: connect() failed in make_client_socket()" );
	return -1;
    }

    return sock;
}


// If specified as a server (out direction for now) open the master
// listening socket.  If specified as a client, open a connection to a
// server.

bool FGSocket::open( FGProtocol::fgProtocolDir dir ) {
    if ( port_str == "" || port_str == "any" ) {
	port = 0; 
    } else {
	port = atoi( port_str.c_str() );
    }
    
    client_connections.clear();

    if ( dir == FGProtocol::out ) {
	// this means server for now

	// Setup socket to listen on.  Set "port" before making this
	// call.  A port of "0" indicates that we want to let the os
	// pick any available port.
	sock = make_server_socket();
	FG_LOG( FG_IO, FG_INFO, "socket is connected to port = " << port );

	// Specify the maximum length of the connection queue
	listen(sock, FG_MAX_SOCKET_QUEUE);

    } else if ( dir == FGProtocol::in ) {
	// this means client for now

	sock = make_client_socket();
    } else {
	FG_LOG( FG_IO, FG_ALERT, 
		"Error:  bidirection mode not available yet for sockets." );
	return false;
    }

    if ( sock < 0 ) {
	FG_LOG( FG_IO, FG_ALERT, "Error opening socket: " << hostname
		<< ":" << port );
	return false;
    }

    return true;
}


// read data from socket (client)
// read a block of data of specified size
int FGSocket::read( char *buf, int length ) {
    int result = 0;

    // check for potential input
    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(sock, &ready);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // test for any input read on sock (returning immediately, even if
    // nothing)
    select(32, &ready, 0, 0, &tv);

    if ( FD_ISSET(sock, &ready) ) {
	result = std::read( sock, buf, length );
	if ( result != length ) {
	    FG_LOG( FG_IO, FG_INFO, 
		    "Warning: read() not enough bytes." );
	}
    }

    return result;
}


// read a line of data, length is max size of input buffer
int FGSocket::readline( char *buf, int length ) {
    int result = 0;

    // check for potential input
    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(sock, &ready);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // test for any input read on sock (returning immediately, even if
    // nothing)
    select(32, &ready, 0, 0, &tv);

    if ( FD_ISSET(sock, &ready) ) {
	// read a chunk, keep in the save buffer until we have the
	// requested amount read

	char *buf_ptr = save_buf + save_len;
	result = std::read( sock, buf_ptr, FG_MAX_MSG_SIZE - save_len );
	save_len += result;
	// cout << "current read = " << buf_ptr << endl;
	// cout << "current save_buf = " << save_buf << endl;
	// cout << "save_len = " << save_len << endl;
    }

    // look for the end of line in save_buf
    int i;
    for ( i = 0; i < save_len && save_buf[i] != '\n'; ++i );
    if ( save_buf[i] == '\n' ) {
	result = i + 1;
    } else {
	// no end of line yet
	// cout << "no eol found" << endl;
	return 0;
    }
    // cout << "line length = " << result << endl;

    // we found an end of line

    // copy to external buffer
    strncpy( buf, save_buf, result );
    buf[result] = '\0';
    // cout << "fg_socket line = " << buf << endl;
    
    // shift save buffer
    for ( i = result; i < save_len; ++i ) {
	save_buf[ i - result ] = save_buf[i];
    }
    save_len -= result;

    return result;
}


// write data to socket (server)
int FGSocket::write( char *buf, int length ) {

    // check for any new client connection requests
    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(sock, &ready);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // test for any input on sock (returning immediately, even if
    // nothing)
    select(32, &ready, 0, 0, &tv);

    // any new connections?
    if ( FD_ISSET(sock, &ready) ) {
	int msgsock = accept(sock, 0, 0);
	if ( msgsock < 0 ) {
	    FG_LOG( FG_IO, FG_ALERT, 
		    "Error: accept() failed in write()" );
	    return 0;
	} else {
	    client_connections.push_back( msgsock );
	}
    }

    bool error_condition = false;
    FG_LOG( FG_IO, FG_INFO, "Client connections = " << 
	    client_connections.size() );
    for ( int i = 0; i < (int)client_connections.size(); ++i ) {
	int msgsock = client_connections[i];

	// read and junk any possible incoming messages.
	// char junk[ FG_MAX_MSG_SIZE ];
	// std::read( msgsock, junk, FG_MAX_MSG_SIZE );

	// write the interesting data to the socket
	if ( std::write(msgsock, buf, length) < 0 ) {
	    FG_LOG( FG_IO, FG_ALERT, "Error writing to socket: " << port );
	    error_condition = true;
	} else {
#ifdef _POSIX_SYNCHRONIZED_IO
	    // fdatasync(msgsock);
#else
	    // fsync(msgsock);
#endif
	}
    }

    if ( error_condition ) {
	return 0;
    }

    return length;
}


// write null terminated string to socket (server)
int FGSocket::writestring( char *str ) {
    int length = strlen( str );
    return write( str, length );
}


// close the port
bool FGSocket::close() {
    for ( int i = 0; i < (int)client_connections.size(); ++i ) {
	int msgsock = client_connections[i];
	std::close( msgsock );
    }

    std::close( sock );

    return true;
}
