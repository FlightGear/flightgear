// navdb.hxx -- top level navaids management routines
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_NAVDB_HXX
#define _FG_NAVDB_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

// #include <map>
// #include <vector>
// #include STL_STRING

#include "navlist.hxx"
#include "fixlist.hxx"

// SG_USING_STD(map);
// SG_USING_STD(vector);
// SG_USING_STD(string);


// load and initialize the navigational databases
bool fgNavDBInit( FGNavList *navlist, FGNavList *loclist, FGNavList *gslist,
                  FGNavList *dmelist, FGNavList *mkrbeacons );


#endif // _FG_NAVDB_HXX
