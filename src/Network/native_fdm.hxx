// native_fdm.hxx -- FGFS "Native" flight dynamics protocal class
//
// Written by Curtis Olson, started September 2001.
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


#ifndef _FG_NATIVE_FDM_HXX
#define _FG_NATIVE_FDM_HXX


#include <simgear/compiler.h>

#include <simgear/timing/timestamp.hxx>

#include "protocol.hxx"
#include "net_fdm.hxx"
#if FG_HAVE_DDS
#include "DDS/dds_fdm.h"
#else
using FG_DDS_FDM = FGNetFDM;
#endif


class FGNativeFDM : public FGProtocol {

    union {
        FG_DDS_FDM dds;
        FGNetFDM net;
    } fdm;
    
public:

    FGNativeFDM() = default;
    ~FGNativeFDM() = default;

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
};

#endif // _FG_NATIVE_FDM_HXX
