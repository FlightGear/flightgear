// approachlist.cxx -- Approach data management class
//
// Written by Alexander Kappes, started March 2002.
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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "approachlist.hxx"


FGApproachList *current_approachlist;


// Constructor
FGApproachList::FGApproachList( void ) {
}


// Destructor
FGApproachList::~FGApproachList( void ) {
}


// load the approach data and build the map
bool FGApproachList::init( SGPath path ) {
    return true;
}

