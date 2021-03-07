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

#include "protocol.hxx"
#include "net_ctrls.hxx"
#if FG_HAVE_DDS
#include "DDS/dds_ctrls.h"
#else
using FG_DDS_Ctrls = FGNetCtrls;
#endif

using std::string;

class FGNativeCtrls : public FGProtocol {

    union {
        FG_DDS_Ctrls dds;
        FGNetCtrls net;
    } ctrls;

public:

    FGNativeCtrls() = default;
    ~FGNativeCtrls() = default;

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
};


// Helper functions which may be useful outside this class

// Populate the FGNetCtrls/FG_DDS_Ctrls structure from the property tree.
template<typename T>
void FGProps2Ctrls( T *net, bool honor_freezes, bool net_byte_order );

// Update the property tree from the FGNetCtrls/FG_DDS_Ctrls structure.
template<typename T>
void FGCtrls2Props( T *net, bool honor_freezes, bool net_byte_order );

#endif // _FG_NATIVE_CTRLS_HXX


