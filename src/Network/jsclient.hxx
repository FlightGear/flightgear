// jsclient.hxx -- simple UDP networked joystick client
//
// Copyright (C) 2003 by Manuel Bessler and Stephen Lowry
//
// based on joyclient.hxx by Curtis Olson
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


#ifndef _FG_JSCLIENT_HXX
#define _FG_JSCLIENT_HXX


#include <simgear/compiler.h>

#include <string>

#include <FDM/flight.hxx>

#include "protocol.hxx"

using std::string;


class FGJsClient : public FGProtocol {

    char buf[256];
    int length;
    double axis[4];
    SGPropertyNode_ptr axisdef[4];
    string axisdefstr[4];
    bool active;
				
public:

    FGJsClient();
    ~FGJsClient();

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
};


#endif // _FG_JSCLIENT_HXX
