// UFO.hxx -- interface to the "UFO" flight model
//
// Written by Curtis Olson, started October 1999.
// Slightly modified from MagicCarpet.hxx by Jonathan Polley, April 2002
//
// Copyright (C) 1999-2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _UFO_HXX
#define _UFO_HXX


#include "flight.hxx"


class FGUFO: public FGInterface {
    double Throttle;
    double Aileron;
    double Elevator;
    double Elevator_Trim;
    double Rudder;
public:
    FGUFO( double dt );
    ~FGUFO();

    // reset flight params to a specific position 
    void init();

    // update position based on inputs, positions, velocities, etc.
    void update( double dt );

};


#endif // _UFO_HXX
