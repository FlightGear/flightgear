// serial.cxx -- Unix serial I/O support
//
// Written by Curtis Olson, started November 1998.
//
// Copyright (C) 1998  Curtis L. Olson - curt@flightgear.org
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
// (Log is kept at end of this file)


#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <Debug/logstream.hxx>

#include "serial.hxx"


fgSERIAL::fgSERIAL() {
    dev_open = false;
}

fgSERIAL::fgSERIAL(const string& device, int baud) {
    open_port(device);
    
    if ( dev_open ) {
	set_baud(baud);
    }
}

fgSERIAL::~fgSERIAL() {
    // closing the port here screws us up because if we would even so
    // much as make a copy of an fgSERIAL object and then delete it,
    // the file descriptor gets closed.  Doh!!!

    // close(fd);
}

bool fgSERIAL::open_port(const string& device) {
    struct termios config;

    fd = open(device.c_str(), O_RDWR | O_NONBLOCK);
    cout << "Serial fd created = " << fd << endl;

    if ( fd  == -1 ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Cannot open " << device
		<< " for serial I/O" );
	return false;
    } else {
	dev_open = true;
    }

    // set required port parameters 
    if ( tcgetattr( fd, &config ) != 0 ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Unable to poll port settings" );
	return false;
    }

    // cfmakeraw( &config );

    // cout << "config.c_iflag = " << config.c_iflag << endl;

    // software flow control on
    config.c_iflag |= IXON;
    // config.c_iflag |= IXOFF;

    // config.c_cflag |= CLOCAL;

#if ! defined( sgi )    
    // disable hardware flow control
    config.c_cflag &= ~(CRTSCTS);
#endif

    // cout << "config.c_iflag = " << config.c_iflag << endl;

    if ( tcsetattr( fd, TCSANOW, &config ) != 0 ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Unable to update port settings" );
	return false;
    }

    return true;
}


bool fgSERIAL::close_port() {
    close(fd);
    return true;
}


bool fgSERIAL::set_baud(int baud) {
    struct termios config;
    speed_t speed = B9600;

    if ( tcgetattr( fd, &config ) != 0 ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Unable to poll port settings" );
	return false;
    }

    if ( baud == 300 ) {
	speed = B300;
    } else if ( baud == 1200 ) {
	speed = B1200;
    } else if ( baud == 2400 ) {
	speed = B2400;
    } else if ( baud == 4800 ) {
	speed = B4800;
    } else if ( baud == 9600 ) {
	speed = B9600;
    } else if ( baud == 19200 ) {
	speed = B19200;
    } else if ( baud == 38400 ) {
	speed = B38400;
    } else if ( baud == 57600 ) {
	speed = B57600;
    } else if ( baud == 115200 ) {
	speed = B115200;
#if defined( linux ) || defined( __FreeBSD__ )
    } else if ( baud == 230400 ) {
	speed = B230400;
#endif
    } else {
	FG_LOG( FG_SERIAL, FG_ALERT, "Unsupported baud rate " << baud );
	return false;
    }

    if ( cfsetispeed( &config, speed ) != 0 ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Problem setting input baud rate" );
	return false;
    }

    if ( cfsetospeed( &config, speed ) != 0 ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Problem setting output baud rate" );
	return false;
    }

    if ( tcsetattr( fd, TCSANOW, &config ) != 0 ) {
	FG_LOG( FG_SERIAL, FG_ALERT, "Unable to update port settings" );
	return false;
    }

    return true;
}

string fgSERIAL::read_port() {
    const int max_count = 1024;
    char buffer[max_count+1];
    int count;
    string result;

    count = read(fd, buffer, max_count);
    // cout << "read " << count << " bytes" << endl;

    if ( count < 0 ) {
	// error condition
	if ( errno != EAGAIN ) {
	    FG_LOG( FG_SERIAL, FG_ALERT, 
		    "Serial I/O on read, error number = " << errno );
	}

	return "";
    } else {
	buffer[count] = '\0';
	result = buffer;

	return result;
    }
}

int fgSERIAL::write_port(const string& value) {
    static bool error = false;
    int count;

    if ( error ) {
	// attempt some sort of error recovery
	count = write(fd, "\n", 1);
	if ( count == 1 ) {
	    // cout << "Serial error recover successful!\n";
	    error = false;
	} else {
	    return 0;
	}
    }

    count = write(fd, value.c_str(), value.length());
    // cout << "write '" << value << "'  " << count << " bytes" << endl;

    if ( (int)count == (int)value.length() ) {
	error = false;
    } else {
	error = true;
	if ( errno == EAGAIN ) {
	    // ok ... in our context we don't really care if we can't
	    // write a string, we'll just get it the next time around
	} else {
	    FG_LOG( FG_SERIAL, FG_ALERT,
		    "Serial I/O on write, error number = " << errno );
	}
    }

    return count;
}


// $Log$
// Revision 1.8  1999/01/20 13:42:21  curt
// Tweaked FDM interface.
// Testing check sum support for NMEA serial output.
//
// Revision 1.7  1998/12/04 01:24:35  curt
// Tweak for SGI portability.
//
// Revision 1.6  1998/11/30 17:15:29  curt
// Having the class destructor close the fd was a bad idea ... especially if you
// ever make a copy of the instance and then subsequently destroy either.
// close_port() is now a separate member function.
//
// Revision 1.5  1998/11/25 01:33:23  curt
// Remove call to cfmakeraw()
//
// Revision 1.4  1998/11/23 21:47:00  curt
// Cygnus tools compatibility tweaks.
//
// Revision 1.3  1998/11/19 13:52:54  curt
// port configuration tweaks & experiments.
//
// Revision 1.2  1998/11/19 03:35:43  curt
// Updates ...
//
// Revision 1.1  1998/11/16 13:53:02  curt
// Initial revision.
//
