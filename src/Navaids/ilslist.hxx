// ilslist.hxx -- ils management class
//
// Written by Curtis Olson, started April 2000.
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


#ifndef _FG_ILSLIST_HXX
#define _FG_ILSLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>

#include "ils.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);


class FGILSList {

    // convenience types
    typedef vector < FGILS* > ils_list_type;
    typedef ils_list_type::iterator ils_list_iterator;
    typedef ils_list_type::const_iterator ils_list_const_iterator;

    // typedef map < int, ils_list_type, less<int> > ils_map_type;
    typedef map < int, ils_list_type > ils_map_type;
    typedef ils_map_type::iterator ils_map_iterator;
    typedef ils_map_type::const_iterator ils_map_const_iterator;

    ils_map_type ilslist;

public:

    FGILSList();
    ~FGILSList();

    // load the navaids and build the map
    bool init( SGPath path );

    // query the database for the specified frequency, lon and lat are
    // in degrees, elev is in meters
    // bool query( double lon, double lat, double elev, double freq, FGILS *i );

    // Query the database for the specified frequency.  It is assumed
    // that there will be multiple stations with matching frequencies
    // so a position must be specified.  Lon and lat are in degrees,
    // elev is in meters.
    FGILS *findByFreq( double freq, double lon, double lat, double elev );

};


#endif // _FG_ILSLIST_HXX
