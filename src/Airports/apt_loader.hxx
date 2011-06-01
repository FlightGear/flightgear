// apt_loader.hxx -- a front end loader of the apt.dat file.  This loader
//                   populates the runway and basic classes.
//
// Written by Curtis Olson, started December 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _FG_APT_LOADER_HXX
#define _FG_APT_LOADER_HXX

#include <simgear/compiler.h>

#include <string>

// Load the airport data base from the specified aptdb file.  The
// metar file is used to mark the airports as having metar available
// or not.

bool fgAirportDBLoad( const std::string &aptdb_file, 
        const std::string &metar_file );

#endif // _FG_APT_LOADER_HXX
