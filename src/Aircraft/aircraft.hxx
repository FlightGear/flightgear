//*************************************************************************
// aircraft.hxx -- define shared aircraft parameters
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//*************************************************************************/


#ifndef _AIRCRAFT_HXX
#define _AIRCRAFT_HXX
                                 
class FGControls;
class SGPropertyNode;

// Define a structure containing all the parameters for an aircraft
typedef struct{
    FGControls *controls;
} fgAIRCRAFT ;


// current_aircraft contains all the parameters of the aircraft
// currently being operated.
extern fgAIRCRAFT current_aircraft;


// Initialize an Aircraft structure
void fgAircraftInit( void );


// Display various parameters to stdout
void fgAircraftOutputCurrent(fgAIRCRAFT *a);


// Read the list of available aircraft into to property tree
void fgReadAircraft(void);
bool fgLoadAircraft (const SGPropertyNode * arg);

#endif // _AIRCRAFT_HXX

