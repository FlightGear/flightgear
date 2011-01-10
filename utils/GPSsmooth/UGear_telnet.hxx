// \file UGear_telnet.hxx
// telnet server class.
//
// Adapted from FlightGear props.hxx/cxx code
// Written by Curtis Olson, started September 2000.
// Modified by Bernie Bright, May 2002.
//
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


#ifndef _UG_TELNET_HXX
#define _UG_TELNET_HXX

#include <string>
#include <vector>

using std::string;
using std::vector;

#include <simgear/io/sg_netChannel.hxx>


/**
 * Telent server class.
 * This class provides a telnet-like server for remote access to
 * FlightGear properties.
 */
class UGTelnet: simgear::NetChannel
{

private:

    /**
     * Server port to listen on.
     */
    int port;
    bool enabled;

public:

    /**
     * Create a new TCP server.
     * 
     * @param tokens Tokenized configuration parameters
     */
    UGTelnet( const int port_num );

    /**
     * Destructor.
     */
    ~UGTelnet();

    /**
     * Start the telnet server.
     */
    bool open();

    /**
     * Process network activity.
     */
    bool process();

    /**
     * 
     */
    bool close();

    /**
     * Accept a new client connection.
     */
    void handleAccept();

};

#endif // _UG_TELNET_HXX

