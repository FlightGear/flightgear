//
// fg_init.hxx -- Flight Gear top level initialization routines
//
// Written by Curtis Olson, started August 1997.
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


#ifndef _FG_INIT_HXX
#define _FG_INIT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


// Read in configuration (file and command line)
bool fgInitConfig ( int argc, char **argv );


// General house keeping initializations
bool fgInitGeneral ( void );


// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
bool fgInitSubsystems( void );


// Reset
void fgReInitSubsystems( void );


// Set current_options lon/lat given an airport id
bool fgSetPosFromAirportID( const string& id );

// Set current_options lon/lat given an airport id and heading (degrees)
bool fgSetPosFromAirportIDandHdg( const string& id, double tgt_hdg );


#endif // _FG_INIT_H


