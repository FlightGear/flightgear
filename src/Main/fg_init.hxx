//
// fg_init.hxx -- Flight Gear top level initialization routines
//
// Written by Curtis Olson, started August 1997.
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


#ifndef _FG_INIT_HXX
#define _FG_INIT_HXX

#include <string>
#include <simgear/misc/sg_path.hxx>

// forward decls
class SGPropertyNode;

// Return the current base package version
std::string fgBasePackageVersion();

bool fgInitHome();

// Read in configuration (file and command line)
int fgInitConfig ( int argc, char **argv, bool reinit );

void fgInitAircraftPaths(bool reinit);

int fgInitAircraft(bool reinit);

// log various settings / configuration state
void fgOutputSettings();

// Initialize the localization
SGPropertyNode *fgInitLocale(const char *language);

// Init navaids and waypoints
bool fgInitNav ();


// General house keeping initializations
bool fgInitGeneral ();


// Create all the subsystems needed by the sim
void fgCreateSubsystems(bool duringReset);

// called after the subsystems have been bound and initialised,
// to peform final init
void fgPostInitSubsystems();

// Re-position: when only location is changing, we can do considerably
// less work than a full re-init.
void fgStartReposition();

void fgStartNewReset();

#endif // _FG_INIT_HXX



