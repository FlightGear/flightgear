// controls.cxx -- defines a standard interface to all flight sim controls
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


#include "controls.hxx"


FGControls controls;


// Constructor
FGControls::FGControls() :
    aileron( 0.0 ),
    elevator( 0.0 ),
    elevator_trim( 1.969572E-03 ),
    rudder( 0.0 )
{
    for ( int engine = 0; engine < MAX_ENGINES; engine++ ) {
	throttle[engine] = 0.0;
    }

    for ( int wheel = 0; wheel < MAX_WHEELS; wheel++ ) {
        brake[wheel] = 0.0;
    }
}


// Destructor
FGControls::~FGControls() {
}


