// protocol.hxx -- High level protocal class
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


#ifndef _PROTOCOL_HXX
#define _PROTOCOL_HXX


#include <simgear/compiler.h>

#include STL_STRING
#include <vector>

SG_USING_STD(string);
SG_USING_STD(vector);


#define FG_MAX_MSG_SIZE 16384

// forward declaration
class SGIOChannel;


class FGProtocol {

private:

    double hz;
    int count_down;

    SGProtocolDir dir;

    // string protocol_str;

    // char buf[FG_MAX_MSG_SIZE];
    // int length;

    bool enabled;

    SGIOChannel *io;

public:

    FGProtocol();
    virtual ~FGProtocol();

    virtual bool open();
    virtual bool process();
    virtual bool close();

    inline SGProtocolDir get_direction() const { return dir; }
    inline void set_direction( const string& d ) {
	if ( d == "in" ) {
	    dir = SG_IO_IN;
	} else if ( d == "out" ) {
	    dir = SG_IO_OUT;
	} else if ( d == "bi" ) {
	    dir = SG_IO_BI;
	} else {
	    dir = SG_IO_NONE;
	}
    }

    inline double get_hz() const { return hz; }
    inline void set_hz( double t ) { hz = t; }
    inline int get_count_down() const { return count_down; }
    inline void set_count_down( int c ) { count_down = c; }
    inline void dec_count_down( int c ) { count_down -= c; }

    virtual bool gen_message();
    virtual bool parse_message();

    // inline string get_protocol() const { return protocol_str; }
    // inline void set_protocol( const string& str ) { protocol_str = str; }

    // inline char *get_buf() { return buf; }
    // inline int get_length() const { return length; }
    // inline void set_length( int l ) { length = l; }

    inline bool is_enabled() const { return enabled; }
    inline void set_enabled( const bool b ) { enabled = b; }

    inline SGIOChannel *get_io_channel() const { return io; }
    inline void set_io_channel( SGIOChannel *c ) { io = c; }
};


typedef vector < FGProtocol * > io_container;
typedef io_container::iterator io_iterator;
typedef io_container::const_iterator const_io_iterator;


#endif // _PROTOCOL_HXX


