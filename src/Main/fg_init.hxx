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


#ifdef HAVE_CONFIG
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/timing/sg_time.hxx>

#include STL_STRING

#include <Airports/simple.hxx>

SG_USING_STD(string);


// Read in configuration (file and command line) and just set fg_root
bool fgInitFGRoot ( int argc, char **argv );


// Return the current base package version
string fgBasePackageVersion();


// Read in configuration (file and command line)
bool fgInitConfig ( int argc, char **argv );


// Initialize the localization
SGPropertyNode *fgInitLocale(const char *language);


// General house keeping initializations
bool fgInitGeneral ( void );


// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
bool fgInitSubsystems( void );


// Reset
void fgReInitSubsystems( void );


// find basic airport location info from airport database
bool fgFindAirportID( const string& id, FGAirport *a );

// Set pos given an airport id
bool fgSetPosFromAirportID( const string& id );

// Set tower position given an airport id
bool fgSetTowerPosFromAirportID( const string& id, double hdg );

// Set position and heading given an airport id and heading (degrees)
bool fgSetPosFromAirportIDandHdg( const string& id, double tgt_hdg );

//find altitude given glideslope and offset distance or offset distance
//given glideslope and altitude
void fgSetPosFromGlideSlope(void);

// Initialize various time dependent systems (lighting, sun position, etc.)
// returns a new instance of the SGTime class
SGTime *fgInitTime();

#endif // _FG_INIT_HXX



