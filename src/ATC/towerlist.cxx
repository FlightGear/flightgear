// towerlist.cxx -- ATC Tower data management class
//
// Written by David Luff, started March 2002.
// Based on navlist.cxx by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>

#include "towerlist.hxx"

FGTowerList *current_towerlist;

// Constructor
FGTowerList::FGTowerList( void ) {
}


// Destructor
FGTowerList::~FGTowerList( void ) {
}


// load the navaids and build the map
bool FGTowerList::init( SGPath path ) {
    return true;
}

