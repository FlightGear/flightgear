// aircraft.cxx -- various aircraft routines
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#include <stdio.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>

#include "aircraft.hxx"

// This is a record containing all the info for the aircraft currently
// being operated
fgAIRCRAFT current_aircraft;


// Initialize an Aircraft structure
void fgAircraftInit( void ) {
    FG_LOG( FG_AIRCRAFT, FG_INFO, "Initializing Aircraft structure" );

    current_aircraft.fdm_state   = cur_fdm_state;
    current_aircraft.controls = &controls;
}


// Display various parameters to stdout
void fgAircraftOutputCurrent(fgAIRCRAFT *a) {
    FGInterface *f;

    f = a->fdm_state;

    FG_LOG( FG_FLIGHT, FG_DEBUG,
	    "Pos = ("
	    << (f->get_Longitude() * 3600.0 * SGD_RADIANS_TO_DEGREES) << "," 
	    << (f->get_Latitude()  * 3600.0 * SGD_RADIANS_TO_DEGREES) << ","
	    << f->get_Altitude() 
	    << ")  (Phi,Theta,Psi)=("
	    << f->get_Phi() << "," 
	    << f->get_Theta() << "," 
	    << f->get_Psi() << ")" );

    FG_LOG( FG_FLIGHT, FG_DEBUG,
	    "Kts = " << f->get_V_equiv_kts() 
	    << "  Elev = " << controls.get_elevator() 
	    << "  Aileron = " << controls.get_aileron() 
	    << "  Rudder = " << controls.get_rudder() 
	    << "  Power = " << controls.get_throttle( 0 ) );
}


