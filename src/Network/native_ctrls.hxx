// native_ctrls.hxx -- FGFS "Native" controls I/O class
//
// Written by Curtis Olson, started July 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NATIVE_CTRLS_HXX
#define _FG_NATIVE_CTRLS_HXX


#include <simgear/compiler.h>

#include <string>

#include <Aircraft/controls.hxx>
#include "protocol.hxx"
#include "net_ctrls.hxx"

using std::string;

class FGNativeCtrls : public FGProtocol {

    FGNetCtrls net_ctrls;
    FGControls ctrls;

    int length;

public:

    FGNativeCtrls();
    ~FGNativeCtrls();

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
};


// Helper functions which may be useful outside this class

// Populate the FGNetCtrls structure from the property tree.
void FGProps2NetCtrls( FGNetCtrls *net, bool honor_freezes,
                       bool net_byte_order );

// Update the property tree from the FGNetCtrls structure.
void FGNetCtrls2Props( FGNetCtrls *net, bool honor_freezes,
                       bool net_byte_order );


#endif // _FG_NATIVE_CTRLS_HXX


