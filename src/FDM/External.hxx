// External.hxx -- interface to the "External"-ly driven flight model
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _EXTERNAL_HXX
#define _EXTERNAL_HXX


#include "flight.hxx"


class FGExternal: public FGInterface {

public:
    FGExternal::FGExternal( double dt );
    FGExternal::~FGExternal();

    // reset flight params to a specific position 
    void init();

    // update position based on inputs, positions, velocities, etc.
    void update( int multiloop );
};


#endif // _EXTERNAL_HXX
