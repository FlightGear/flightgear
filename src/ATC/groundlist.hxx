// groundlist.hxx -- ATC Ground data management class
//
// Written by David Luff, started November 2002.
// Based on navlist.hxx by Curtis Olson, started April 2000.
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


#ifndef _FG_GROUNDLIST_HXX
#define _FG_GROUNDLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>
#include <string>

#include "ground.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);
SG_USING_STD(string);


class FGGroundList {

    // convenience types
    typedef vector < FGGround > ground_list_type;
    typedef ground_list_type::iterator ground_list_iterator;
    typedef ground_list_type::const_iterator ground_list_const_iterator;

    // Map containing FGGround keyed by frequency.
    // A vector of FGGround is kept at each node since some may share frequency
    typedef map < int, ground_list_type > ground_map_type;
    typedef ground_map_type::iterator ground_map_iterator;
    typedef ground_map_type::const_iterator ground_map_const_iterator;

    ground_map_type groundlist;

public:

    FGGroundList();
    ~FGGroundList();

    // load all atis and build the map
    bool init( SGPath path );

    // query the database for the specified frequency, lon and lat are
    // in degrees, elev is in meters
    bool query( double lon, double lat, double elev, double freq, FGGround *g );

};


extern FGGroundList *current_groundlist;


#endif // _FG_GROUNDLIST_HXX
