// mkrbeacons.hxx -- marker beacon management class
//
// Written by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_MKRBEACON_HXX
#define _FG_MKRBEACON_HXX


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>

#include "nav.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);


class FGMkrBeacon {

public:

    enum fgMkrBeacType {
	NOBEACON = 0,
	INNER,
	MIDDLE,
	OUTER   
    };

private:

    double lon;
    double lat;
    double elev;
    fgMkrBeacType type;

    double x, y, z;

public:

    FGMkrBeacon();
    FGMkrBeacon( double _lon, double _lat, double _elev, fgMkrBeacType _type );
    ~FGMkrBeacon();

    inline double get_elev() const { return elev; }
    inline fgMkrBeacType get_type() const { return type; }
    inline double get_x() const { return x; }
    inline double get_y() const { return y; }
    inline double get_z() const { return z; }
   
};


class FGMarkerBeacons {

    // convenience types
    typedef vector < FGMkrBeacon > beacon_list_type;
    typedef beacon_list_type::iterator beacon_list_iterator;
    typedef beacon_list_type::const_iterator beacon_list_const_iterator;

    typedef map < int, beacon_list_type > beacon_map_type;
    typedef beacon_map_type::iterator beacon_map_iterator;
    typedef beacon_map_type::const_iterator beacon_map_const_iterator;

    beacon_map_type beacon_map;

    // real add a marker beacon
    bool real_add( const int master_index, const FGMkrBeacon& b );

public:

    FGMarkerBeacons();
    ~FGMarkerBeacons();

    // initialize the structures
    bool init();

    // add a marker beacon
    bool add( double lon, double lat, double elev,
	      FGMkrBeacon::fgMkrBeacType type );

    // returns marker beacon type if we are over a marker beacon, NOBEACON
    // otherwise
    FGMkrBeacon::fgMkrBeacType query( double lon, double lat, double elev );
};


#endif // _FG_MKRBEACON_HXX
