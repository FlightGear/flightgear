// external.cxx -- externally driven flight model
//
// Written by Curtis Olson, started January 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@flightgear.org
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
// (Log is kept at end of this file)


#include <math.h>

#include "external.hxx"

#include <FDM/flight.hxx>
#include <Include/fg_constants.h>


// reset flight params to a specific position
void fgExternalInit( FGInterface &f ) {
}


// update position based on inputs, positions, velocities, etc.
void fgExternalUpdate( FGInterface& f, int multiloop ) {

}


// $Log$
// Revision 1.5  1999/02/05 21:29:03  curt
// Modifications to incorporate Jon S. Berndts flight model code.
//
// Revision 1.4  1999/02/01 21:33:32  curt
// Renamed FlightGear/Simulator/Flight to FlightGear/Simulator/FDM since
// Jon accepted my offer to do this and thought it was a good idea.
//
// Revision 1.3  1999/01/19 17:52:11  curt
// Working on being able to extrapolate a new position and orientation
// based on a position, orientation, and time offset.
//
// Revision 1.2  1998/12/05 15:54:13  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.1  1998/12/04 01:28:49  curt
// Initial revision.
//
