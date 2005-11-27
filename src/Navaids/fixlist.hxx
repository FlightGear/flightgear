// fixlist.hxx -- fix list management class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_FIXLIST_HXX
#define _FG_FIXLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>
#include STL_STRING

#include "fix.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);
SG_USING_STD(string);

// TODO - fix names may be globally non-unique.  Allow for this.
typedef map < string, FGFix > fix_map_type;
typedef fix_map_type::iterator fix_map_iterator;
typedef fix_map_type::const_iterator fix_map_const_iterator;

class FGFixList {

    fix_map_type fixlist;

public:

    FGFixList();
    ~FGFixList();

    // load the navaids and build the map
    bool init( SGPath path );

    // query the database for the specified fix
    bool query( const string& ident, FGFix *f );
	
    // Find fix of requested type with closest exact or following ident 
    // (by ACSII values) to that supplied (ie. a lower-bound lookup).
    // Supplying true for exact forces only exact matches to be returned (similar to above function)
    // Returns NULL if no match found.
    const FGFix* findFirstByIdent( const string& ident, bool exact = false );

    // query the database for the specified fix, lon and lat are
    // in degrees, elev is in meters
    bool query_and_offset( const string& ident, double lon, double lat,
                           double elev, FGFix *f, double *heading,
                           double *dist );
						   
    // Return a pointer to the master fixlist
    inline const fix_map_type* getFixList() { return(&fixlist); }
};


#endif // _FG_FIXLIST_HXX
