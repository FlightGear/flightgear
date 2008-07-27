// pve.hxx -- "PVE" protocal class (for Provision Entertainment)
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


#ifndef _FG_PVE_HXX
#define _FG_PVE_HXX


#include <simgear/compiler.h>

#include <string>

#include "protocol.hxx"

using std::string;


class FGPVE : public FGProtocol {

    char buf[ 10 ];
    int length;

public:

    FGPVE();
    ~FGPVE();

    bool gen_message();
    bool parse_message();
 
    // process work for this port
    bool process();
};


#endif // _FG_PVE_HXX


