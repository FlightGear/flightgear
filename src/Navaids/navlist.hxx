// navlist.hxx -- navaids management class
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


#ifndef _FG_NAVLIST_HXX
#define _FG_NAVLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>
#include STL_STRING

#include "nav.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);
SG_USING_STD(string);

class FGNavList {

    // convenience types
    typedef vector < FGNav* > nav_list_type;
    typedef nav_list_type::iterator nav_list_iterator;
    typedef nav_list_type::const_iterator nav_list_const_iterator;

    typedef map < int, nav_list_type > nav_map_type;
    typedef nav_map_type::iterator nav_map_iterator;
    typedef nav_map_type::const_iterator nav_map_const_iterator;

    typedef map < string, nav_list_type > nav_ident_map_type;
	
    nav_map_type navaids;
    nav_ident_map_type ident_navaids;
	
    // Given a point and a list of stations, return the closest one to
    // the specified point.
    FGNav *findNavFromList( const Point3D &aircraft, 
                            const nav_list_type &stations );
	
public:

    FGNavList();
    ~FGNavList();

    // load the navaids and build the map
    bool init( SGPath path );

    // Query the database for the specified frequency.  It is assumed
    // that there will be multiple stations with matching frequencies
    // so a position must be specified.  Lon and lat are in degrees,
    // elev is in meters.
    FGNav *findByFreq( double freq, double lon, double lat, double elev );

    // locate closest item in the DB matching the requested ident
    FGNav *findByIdent( const char* ident, const double lon, const double lat );

    // Given an Ident and optional freqency, return the first matching
    // station.
    FGNav *findByIdentAndFreq( const char* ident, const double freq = 0.0 );
};


#endif // _FG_NAVLIST_HXX
