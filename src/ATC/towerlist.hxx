// towerlist.hxx -- ATC Tower data management class
//
// Written by David Luff, started March 2002.
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


#ifndef _FG_TOWERLIST_HXX
#define _FG_TOWERLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>
#include <string>

#include "tower.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);
SG_USING_STD(string);


class FGTowerList {

    // convenience types
    typedef vector < FGTower > tower_list_type;
    typedef tower_list_type::iterator tower_list_iterator;
    typedef tower_list_type::const_iterator tower_list_const_iterator;

    // Map containing FGTower keyed by frequency.
    // A vector of FGTower is kept at each node since some may share frequency
    typedef map < int, tower_list_type > tower_map_type;
    typedef tower_map_type::iterator tower_map_iterator;
    typedef tower_map_type::const_iterator tower_map_const_iterator;

    tower_map_type towerlist;

public:

    FGTowerList();
    ~FGTowerList();

    // load all atis and build the map
    bool init( SGPath path );

};


extern FGTowerList *current_towerlist;


#endif // _FG_TOWERLIST_HXX
