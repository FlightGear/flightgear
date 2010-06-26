// AV400Sim.hxx -- Garmin 400 series protocal class.  This AV400Sim
// protocol generates the set of "simulator" commands a garmin 400
// series gps would expect as input in simulator mode.  The AV400
// protocol generates the set of commands that a garmin 400 series gps
// would emit.
//
// Written by Curtis Olson, started Januar 2009.
//
// Copyright (C) 2009  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_AV400SIM_HXX
#define _FG_AV400SIM_HXX


#include <simgear/compiler.h>

#include <string>

#include "protocol.hxx"

class FlightProperties;

class FGAV400Sim : public FGProtocol {

    char buf[ FG_MAX_MSG_SIZE ];
    int length;
    FlightProperties* fdm;
    
public:

    FGAV400Sim();
    ~FGAV400Sim();

    bool gen_message();
    bool parse_message();
 
    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
};


#endif // _FG_AV400SIM_HXX
