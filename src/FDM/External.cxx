// External.cxx -- interface to the "External"-ly driven flight model
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


#include "External.hxx"


FGExternal::FGExternal( double dt ) {
    set_delta_t( dt );
}


FGExternal::~FGExternal() {
}


// Initialize the External flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGExternal::init() {
    // cout << "FGExternal::init()" << endl;

    // Explicitly call the superclass's
    // init method first.
    common_init();
}


// Run an iteration of the EOM.  This is essentially a NOP here
// because these values are getting filled in elsewhere based on
// external input.
void FGExternal::update( int multiloop ) {
    // cout << "FGExternal::update()" << endl;

    // double time_step = (1.0 / fgGetInt("/sim/model-hz"))
    //                     * multiloop;
}
