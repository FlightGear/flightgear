// NullFDM.hxx -- a do-nothing flight model, used as a placeholder if the
//                action is externally driven.
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999 - 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _NULLFDM_HXX
#define _NULLFDM_HXX


#include "flight.hxx"


class FGNullFDM: public FGInterface {

public:
    FGNullFDM( double dt );
    ~FGNullFDM();

    // reset flight params to a specific position 
    void init();

    // update position based on inputs, positions, velocities, etc.
    void update( double dt );
};


#endif // _NULLFDM_HXX
