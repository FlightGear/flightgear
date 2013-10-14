// igc.hxx -- International Glider Commission (IGC) protocol class
//
// Written by Thorsten Brehm, started October 2013.
//
// Copyright (C) 2013 Thorsten Brehm - brehmt (at) gmail com
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _FG_IGC_HXX
#define _FG_IGC_HXX

#include <simgear/compiler.h>
#include <FDM/flightProperties.hxx>
#include "protocol.hxx"

class IGCProtocol : public FGProtocol
{
    char buf[FG_MAX_MSG_SIZE];
    int length;

    bool gen_Hrecords();

public:
    IGCProtocol();
    ~IGCProtocol();

    bool gen_message();
    bool parse_message();

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();

    FlightProperties fdm;
};

#endif // _FG_IGC_HXX
