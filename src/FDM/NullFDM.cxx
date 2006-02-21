// NullFDM.hxx -- a do-nothing flight model, used as a placeholder if the
//                action is externally driven.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "NullFDM.hxx"


FGNullFDM::FGNullFDM( double dt ) {
//     set_delta_t( dt );
}


FGNullFDM::~FGNullFDM() {
}


// Initialize the NullFDM flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGNullFDM::init() {
    //do init common to all the FDM's		
    common_init();
    // cout << "FGNullFDM::init()" << endl;
    set_inited( true );
}


// Run an iteration of the EOM.  This is a NOP here because the flight
// model values are getting filled in elsewhere (most likely from some
// external source.)
void FGNullFDM::update( double dt ) {
    // cout << "FGNullFDM::update()" << endl;
    // That is just to trigger ground level computations
    _updateGeodeticPosition(get_Latitude(), get_Longitude(), get_Altitude());
}
