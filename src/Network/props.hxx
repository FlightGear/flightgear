// props.hxx -- FGFS property manager interaction class
//
// Written by Curtis Olson, started September 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_PROPS_HXX
#define _FG_PROPS_HXX


#include <simgear/compiler.h>
#include <simgear/misc/props.hxx>

#include STL_STRING

#include "protocol.hxx"

SG_USING_STD(string);


const static int max_cmd_len = 256;

class FGProps : public FGProtocol {

    char buf[max_cmd_len];
    int length;

    // tree view of property list
    string path;

public:

    FGProps();
    ~FGProps();

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();
    bool process_command( const char *cmd );

    // close the channel
    bool close();
};


#endif // _FG_PROPS_HXX
