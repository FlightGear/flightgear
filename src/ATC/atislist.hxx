// atislist.hxx -- atis management class
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


#ifndef _FG_ATISLIST_HXX
#define _FG_ATISLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>

#include "atis.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);


class FGATISList {

    // convenience types
    typedef vector < FGATIS > atis_list_type;
    typedef atis_list_type::iterator atis_list_iterator;
    typedef atis_list_type::const_iterator atis_list_const_iterator;

    // typedef map < int, atis_list_type, less<int> > atis_map_type;
    typedef map < int, atis_list_type > atis_map_type;
    typedef atis_map_type::iterator atis_map_iterator;
    typedef atis_map_type::const_iterator atis_map_const_iterator;

    atis_map_type atislist;

public:

    FGATISList();
    ~FGATISList();

    // load all atis and build the map
    bool init( SGPath path );

    // query the database for the specified frequency, lon and lat are
    // in degrees, elev is in meters
    bool query( double lon, double lat, double elev, double freq, FGATIS *a );
};


extern FGATISList *current_atislist;


#endif // _FG_ATISLIST_HXX
