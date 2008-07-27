// protocol.hxx -- High level protocal class
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _PROTOCOL_HXX
#define _PROTOCOL_HXX


#include <simgear/compiler.h>
#include <simgear/io/iochannel.hxx>

#include <string>
#include <vector>

using std::string;
using std::vector;


#define FG_MAX_MSG_SIZE 16384

class FGProtocol {

private:

    double hz;
    double count_down;
    long count;

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
    void set_direction( const string& d );

    inline double get_hz() const { return hz; }
    inline void set_hz( double t ) { hz = t; }
    inline double get_count_down() const { return count_down; }
    inline void set_count_down( double c ) { count_down = c; }
    inline void inc_count_down( double amt ) { count_down += amt; }
    inline void dec_count_down( double amt ) { count_down -= amt; }
    inline void inc_count() { count++; }
    inline long get_count() { return count; }

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

#include <stdexcept>
using std::invalid_argument;

//namespace flightgear { namespace network {
class FGProtocolConfigError : public invalid_argument
{
public:
    FGProtocolConfigError( const string& what_string )
	: invalid_argument(what_string) {}
};
//}} // end namespace flightgear::network


#endif // _PROTOCOL_HXX


