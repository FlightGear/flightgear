// viewer.cxx -- class for managing a viewer in the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//
// Copyright (C) 1997 - 2000  Curtis L. Olson  - curt@flightgear.org
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


#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/constants.h>

#include "viewer.hxx"


// Constructor
FGViewer::FGViewer( void ):
    fov(55.0),
    view_offset(0.0),
    goal_view_offset(0.0)
{
    sgSetVec3( pilot_offset, 0.0, 0.0, 0.0 );
    sgdZeroVec3(geod_view_pos);
    sgdZeroVec3(abs_view_pos);
    sea_level_radius = SG_EQUATORIAL_RADIUS_M; 
    //a reasonable guess for init, so that the math doesn't blow up
}


// Update the view parameters
void FGViewer::update() {
    SG_LOG( SG_VIEW, SG_ALERT, "Shouldn't ever see this" );
}


// Destructor
FGViewer::~FGViewer( void ) {
}
