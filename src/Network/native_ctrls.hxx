// native_ctrls.hxx -- FGFS "Native" controls I/O class
//
// Written by Curtis Olson, started July 2001.
//
// Copyright (C) 2001  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_NATIVE_CTRLS_HXX
#define _FG_NATIVE_CTRLS_HXX


#include <simgear/compiler.h>

#include STL_STRING

#include <Controls/controls.hxx>

#include "protocol.hxx"
#include "raw_ctrls.hxx"

SG_USING_STD(string);


class FGNativeCtrls : public FGProtocol {

    FGRawCtrls raw_ctrls;
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


#endif // _FG_NATIVE_CTRLS_HXX


